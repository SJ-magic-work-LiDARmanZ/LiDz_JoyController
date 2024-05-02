/************************************************************
************************************************************/
#include "ofApp.h"


/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp()
: param_cam_speed_id(4, 1)
, param_cam_pos_id(27, 16)
{
}

/******************************
******************************/
ofApp::~ofApp(){
}

/******************************
******************************/
void ofApp::setup(){
	/********************
	********************/
	ofSetWindowTitle("LiDz_JoyController");
	
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetWindowShape(kWindowWidth, kWindowHeight);
	ofSetEscapeQuitsApp(false);
	
	ofEnableAntiAliasing();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA); // OF_BLENDMODE_DISABLED, OF_BLENDMODE_ALPHA, OF_BLENDMODE_ADD
	
	/********************
	********************/
	img_screen_design_.load("ScreenDesign/ScreenDesign.png");
	
	/********************
	********************/
	SetupUdp();
	
	/********************
	********************/
	SetupMidiIn();
	SetupMidiOut();
	
    midi_out_.sendControlChange(1, 0, 0); // Reset LED
	
	UpdateMidiPad();
	
	/********************
	********************/
	gui_.setup("param", "gui.xml", 16, 16);
	
	Group_misc.setup("misc");
		Group_misc.add(gui_b_print_udp_message_.setup("print_udp_msg", false));
		Group_misc.add(gui_b_print_midi_.setup("print_midi", false));
	gui_.add(&Group_misc);
}

/******************************
******************************/
void ofApp::SetupMidiIn(){
	/********************
	********************/
	// ofSetLogLevel(OF_LOG_VERBOSE);
	
	/********************
	********************/
	midi_in_.listInPorts();
	
	// midi_in_.openPort(1); // by number
	midi_in_.openPort("Launchpad Mini MK3 LPMiniMK3 MIDI Out");
	if( midi_in_.isOpen() )	{ printf("> midi_in_ open : ok\n"); }
	else					{ printf("> midi_in_ open : ng\n"); }
	
	// don't ignore sysex, timing, & active sense messages,
	// these are ignored by default
	midi_in_.ignoreTypes(false, false, false);

	// add ofApp as a listener and enable direct message handling
	// comment this to use queued message handling
	midi_in_.addListener(this);

	/********************
	********************/
	// print received messages to the console
	// midi_in_.setVerbose(true);
}

/******************************
******************************/
void ofApp::SetupMidiOut(){
	midi_out_.listOutPorts();
	
	midi_out_.openPort("Launchpad Mini MK3 LPMiniMK3 MIDI In");
	if( midi_out_.isOpen() )	{ printf("> midi_out_ open : ok\n"); }
	else					{ printf("> midi_out_ open : ng\n"); }
}

/******************************
******************************/
void ofApp::SetupUdp(){
	/********************
	********************/
	{
		ofxUDPSettings settings;
		settings.sendTo("127.0.0.1", 12350);
		// settings.sendTo("10.0.0.10", 12345);
		settings.blocking = false;
		
		udp_send_cam_pos_.Setup(settings);
	}
	{
		ofxUDPSettings settings;
		settings.sendTo("127.0.0.1", 12349);
		// settings.sendTo("10.0.0.10", 12345);
		settings.blocking = false;
		
		udp_send_boost_led_.Setup(settings);
	}
	{
		ofxUDPSettings settings;
		settings.sendTo("127.0.0.1", 12351);
		settings.blocking = false;
		
		udp_send_boost_info_to_SendLiveData.Setup(settings);
	}
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	ReceiveMidi_MainThread();
	ExtractParamFromMidiMessage();
	
	/********************
	********************/
	PrepAndSendUdp_CamPos();
	PrepAndSendUdp_BoostLed();
}



/******************************
******************************/
void ofApp::SendMidi_AllLedOff(){
	/********************
	********************/
	for(int i = 0; i <= 33; i++){
		midi_out_.sendNoteOff(1, i, 127);
	}
}

/******************************
******************************/
void ofApp::UpdateMidiPad(){
	SendMidi_AllLedOff();
	
	midi_out_.sendNoteOn(1, param_cam_pos_id.GetId() , 127);
	
	int ofs = 24;
	midi_out_.sendNoteOn(1, ofs + param_cam_speed_id.GetId() , 127);
	
	if(boost_led_ == BoostLed::Normal)				midi_out_.sendNoteOn(1, 32 , 127);
	else if(boost_led_ == BoostLed::FadeToNormal)	midi_out_.sendNoteOn(1, 32 , 127);
	else if(boost_led_ == BoostLed::Strobe)			midi_out_.sendNoteOn(1, 28 , 127);
	else if(boost_led_ == BoostLed::Half)			midi_out_.sendNoteOn(1, 29 , 127);
	else if(boost_led_ == BoostLed::Full)			midi_out_.sendNoteOn(1, 30 , 127);
	else if(boost_led_ == BoostLed::Off)			midi_out_.sendNoteOn(1, 31 , 127);
}

/******************************
******************************/
void ofApp::ReceiveMidi_MainThread(){
	if(midi_in_.hasWaitingMessages()) {
		ofxMidiMessage message;
		
		// add the latest message to the message queue
		while(midi_in_.getNextMessage(message)) {
			// midi_messages_.push_back(message);
			midi_messages_.push(message);
		}
		
		// remove any old messages if we have too many
		/*
		while(midi_messages_.size() > kMaxMessages) {
			midi_messages_.erase(midi_messages_.begin());
		}
		*/
		while(kMaxMessages < midi_messages_.size()) {
			midi_messages_.pop();
		}
	}
}

/******************************
******************************/
void ofApp::ExtractParamFromMidiMessage(){
	// for(unsigned int i = 0; i < midi_messages_.size(); ++i) {
	while( 0 < (int)midi_messages_.size() ){
		ofxMidiMessage message = midi_messages_.front();
		midi_messages_.pop();
		
		if(MIDI_SYSEX <= message.status){
			continue;
		}
		
		if( (message.status == MIDI_NOTE_ON) || (  message.status == MIDI_NOTE_OFF) ){
			bool b_note_on = false;
			if(message.velocity <= 0)	b_note_on = false;
			else						b_note_on = true;
			
			if(b_note_on){
				if( (0 <= message.pitch) && (message.pitch <= 17) ){
					if(gui_b_print_midi_){
						printf("set by midi : cam pos id : %d\n", message.pitch);
						fflush(stdout);
					}
					
					param_cam_pos_id.Set(message.pitch);
					UpdateMidiPad();
					
				}else if( (24 <= message.pitch) &&(message.pitch <= 27) ){
					if(gui_b_print_midi_){
						printf("set by midi : cam speed id : %d\n", message.pitch - 24);
						fflush(stdout);
					}
					
					param_cam_speed_id.Set( message.pitch - 24 );
					UpdateMidiPad();
					
				}else{
					if(gui_b_print_midi_){
						printf("set by midi : flash : %d\n", message.pitch);
						fflush(stdout);
					}
					
					switch(message.pitch){
					case 28:
						boost_led_ = BoostLed::Strobe;
						b_set_boost_led_ = true;
						break;
						
					case 29:
						boost_led_ = BoostLed::Half;
						b_set_boost_led_ = true;
						break;
						
					case 30:
						boost_led_ = BoostLed::Full;
						b_set_boost_led_ = true;
						break;
						
					case 31:
						boost_led_ = BoostLed::Off;
						b_set_boost_led_ = true;
						break;
						
					case 32:
						boost_led_ = BoostLed::Normal;
						b_set_boost_led_ = true;
						break;
						
					case 33:
						boost_led_ = BoostLed::FadeToNormal;
						b_set_boost_led_ = true;
						break;
					}
					
					UpdateMidiPad();
				}
			}
		}
	}
}



/******************************
******************************/
void ofApp::PrepAndSendUdp_CamPos(){
	/********************
	********************/
	if( !param_cam_pos_id.IsSet() )	return;
	
	/********************
	********************/
	string str_message = "/CamWork,";
	
	char buf[100];
	snprintf( buf, std::size(buf), "%d,%d", param_cam_pos_id.GetId(), param_cam_speed_id.GetId() );
	str_message += buf;
	
	udp_send_cam_pos_.Send(str_message.c_str(), str_message.length());
	
	if(gui_b_print_udp_message_){
		printf("udp message = %s\n", str_message.c_str());
		fflush(stdout);
	}
	
	/********************
	********************/
	param_cam_pos_id.Clear();
	param_cam_speed_id.Clear();
}

/******************************
******************************/
void ofApp::PrepAndSendUdp_BoostLed(){
	/********************
	********************/
	if( !b_set_boost_led_ )	return;
	
	/********************
	********************/
	string str_message = "/BoostLed,";
	
	char buf[100];
	snprintf( buf, std::size(buf), "%d", (int)boost_led_ );
	str_message += buf;
	
	udp_send_boost_led_.Send(str_message.c_str(), str_message.length());
	udp_send_boost_info_to_SendLiveData.Send(str_message.c_str(), str_message.length());
	
	if(gui_b_print_udp_message_){
		printf("udp message = %s\n", str_message.c_str());
		fflush(stdout);
	}
	
	/********************
	********************/
	b_set_boost_led_ = false;
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	ofBackground(30);
	
	/********************
	********************/
	img_screen_design_.draw(0, 0, ofGetWidth(), ofGetHeight());
	
	/********************
	********************/
	gui_.draw();
}

/******************************
midi_in_ events
******************************/
void ofApp::newMidiMessage(ofxMidiMessage &message) {
	/*
	// add the latest message to the message queue
	midi_messages_.push_back(message);
	
	// remove any old messages if we have too many
	while(midi_messages_.size() > kMaxMessages) {
		midi_messages_.erase(midi_messages_.begin());
	}
	*/
	
	midi_messages_.push(message);
	
	while(kMaxMessages < midi_messages_.size()) {
		midi_messages_.pop();
	}
}

/******************************
******************************/
void ofApp::exit(){
	/********************
	********************/
	SendMidi_AllLedOff();
	
	/********************
	********************/
	printf("> Good-bye\n");
	fflush(stdout);
}

/******************************
	ParamToSend param_cam_speed_id(3);
	ParamToSend param_cam_pos_id(26);
******************************/
void ofApp::keyPressed(int key){
	switch(key){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			param_cam_pos_id.Set(key - '0');
			break;
			
		case 'q':
			param_cam_speed_id.Set(0);
			printf("param_cam_speed_id = %d\n", param_cam_speed_id.GetId());
			fflush(stdout);
			break;
			
		case 'w':
			param_cam_speed_id.Set(1);
			printf("param_cam_speed_id = %d\n", param_cam_speed_id.GetId());
			fflush(stdout);
			break;
			
		case 'e':
			param_cam_speed_id.Set(2);
			printf("param_cam_speed_id = %d\n", param_cam_speed_id.GetId());
			fflush(stdout);
			break;
			
		case 'r':
			param_cam_speed_id.Set(3);
			printf("param_cam_speed_id = %d\n", param_cam_speed_id.GetId());
			fflush(stdout);
			break;
	}
}

/******************************
******************************/
void ofApp::keyReleased(int key){

}

/******************************
******************************/
void ofApp::mouseMoved(int x, int y ){

}

/******************************
******************************/
void ofApp::mouseDragged(int x, int y, int button){

}

/******************************
******************************/
void ofApp::mousePressed(int x, int y, int button){

}

/******************************
******************************/
void ofApp::mouseReleased(int x, int y, int button){

}

/******************************
******************************/
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

/******************************
******************************/
void ofApp::mouseEntered(int x, int y){

}

/******************************
******************************/
void ofApp::mouseExited(int x, int y){

}

/******************************
******************************/
void ofApp::windowResized(int w, int h){

}

/******************************
******************************/
void ofApp::gotMessage(ofMessage msg){

}

/******************************
******************************/
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
