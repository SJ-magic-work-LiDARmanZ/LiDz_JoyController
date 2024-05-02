/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include <ofxGui.h>

#include "ofxNetwork.h"
#include "ofxMidi.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
class ParamToSend{
private:
	bool b_set_ = false;
	int id_ = 0;
	const int kNumIds_;
	
public:
	ParamToSend(int kNumIds, int init_id)
	: kNumIds_(kNumIds)
	{
		Set(init_id);
	}
	
	void Set(int id){
		if( (0 <= id) && (id < kNumIds_) )	id_ = id;
		else								id_ = 0;
		
		b_set_ = true;
	}
	
	bool IsSet()	{ return b_set_; }
	void Clear()	{ b_set_ = false; }
	int GetId()		{ return id_; }
};

/**************************************************
**************************************************/
class ofApp : public ofBaseApp, public ofxMidiListener{
	/****************************************
	****************************************/
	enum{
		kWindowWidth	= 300,
		kWindowHeight	= 300,
	};
	
	enum class BoostLed{
		Normal,
		FadeToNormal,
		Strobe,
		Half,
		Full,
		Off,
	};
	
	BoostLed boost_led_ = BoostLed::Normal;
	bool b_set_boost_led_ = false;
	
	/********************
	********************/
	ofxPanel gui_;
	ofxGuiGroup Group_misc;
		ofxToggle gui_b_print_udp_message_;
		ofxToggle gui_b_print_midi_;
		
	/********************
	********************/
	ofImage img_screen_design_;
	
	ofxUDPManager udp_send_cam_pos_;
	ofxUDPManager udp_send_boost_led_;
	ofxUDPManager udp_send_boost_info_to_SendLiveData;
	
	ParamToSend param_cam_speed_id;
	ParamToSend param_cam_pos_id;
	
	/********************
	■std::queue
		https://cpprefjp.github.io/reference/queue/queue.html
	********************/
	ofxMidiIn midi_in_;
	const std::size_t kMaxMessages = 10;			// max number of messages to keep track of
	
	// std::vector<ofxMidiMessage> midiMessages;	// received messages
	std::queue<ofxMidiMessage> midi_messages_;		// received messages
	
	
	ofxMidiOut midi_out_;
	
	/****************************************
	****************************************/
	void SetupUdp();
	void PrepAndSendUdp_CamPos();
	void PrepAndSendUdp_BoostLed();
	
	void SetupMidiIn();
	void newMidiMessage(ofxMidiMessage &message) override;
	void ReceiveMidi_MainThread();
	void ExtractParamFromMidiMessage();
	
	void SetupMidiOut();
	void UpdateMidiPad();
	void SendMidi_AllLedOff();
	
public:
	/****************************************
	****************************************/
	ofApp();
	~ofApp();
	
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

	void keyPressed(int key) override;
	void keyReleased(int key) override;
	void mouseMoved(int x, int y ) override;
	void mouseDragged(int x, int y, int button) override;
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
	void mouseEntered(int x, int y) override;
	void mouseExited(int x, int y) override;
	void windowResized(int w, int h) override;
	void dragEvent(ofDragInfo dragInfo) override;
	void gotMessage(ofMessage msg) override;
	
};
