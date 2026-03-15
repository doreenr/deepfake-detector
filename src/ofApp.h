#pragma once

#include "ofMain.h"
#include "FaceTracker.h"

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
};
