#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxMediaPipeFaceTracker.h"

struct Face {
    int id;
    cv::Rect bbox;
    vector<glm::vec2> landmarks;  // 468+ points from MediaPipe face mesh
    cv::Mat cropped;              // BGR face crop
    
    // jitter variables
    float jitterScore = 0.0f;
    int jitterStatus = 0;
};

class FaceTracker {
public:
    void setup();
    void update(ofPixels& pixels);
    void exit();

    int count() const { return faces.size(); }
    const vector<Face>& getFaces() const { return faces; }

private:
    shared_ptr<ofx::MediaPipe::FaceTracker> mpTracker;
    vector<Face> faces;

    cv::Rect computeBBox(const vector<glm::vec2>& landmarks, int imgW, int imgH);
};
