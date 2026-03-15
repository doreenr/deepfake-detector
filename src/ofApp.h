#pragma once

#include "ofMain.h"
#include "FaceTracker.h"
#include "BlinkAnalyzer.h"
#include "JitterAnalyzer.h"
#include <map>

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;

private:
    ofVideoGrabber cam;
    ofPixels videoPixels;
    ofTexture videoTexture;
    FaceTracker tracker;

    // one blink and jitter analyzer per face ID
    map<int, BlinkAnalyzer> blinkAnalyzers;
    map<int, JitterAnalyzer> jitterAnalyzers;
};
