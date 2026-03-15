#pragma once

#include "ofMain.h"
#include "FaceTracker.h"
#include "GUI.h"

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw()   override;
    void exit()   override;

private:
    ofVideoGrabber cam;
    ofPixels       videoPixels;
    ofTexture      videoTexture;
    FaceTracker    tracker;
    GUI            gui;

    // Font for the video-area HUD (same family as the sidebar)
    ofTrueTypeFont hudFont;
    ofTrueTypeFont hudFontSemi;

    // Individual signal scores fed into the GUI.
    vector<SignalScore> signalScores;
};
