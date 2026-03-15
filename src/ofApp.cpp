#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(30);
    ofSetWindowTitle("Deepfake Detector");
    ofBackground(30, 30, 35);

    cam.setDeviceID(0);
    cam.setDesiredFrameRate(30);
    cam.setup(1280, 720);

    tracker.setup();
}

void ofApp::update() {
    cam.update();
    if (cam.isInitialized() && cam.isFrameNew()) {
        videoPixels = cam.getPixels();
        tracker.update(videoPixels);
        videoTexture.loadData(videoPixels);

        // run blink and jitter analysis on each detected face
        auto& faces = tracker.getFaces();
        for (auto& face : faces) {
            blinkAnalyzers[face.id].update(face.landmarks);
            jitterAnalyzers[face.id].update(face.landmarks);
        }
    }
}

void ofApp::draw() {
    // draw camera feed
    ofSetColor(255);
    if (videoTexture.getWidth() > 0) {
        videoTexture.draw(0, 0, ofGetWidth(), ofGetHeight());
    } else {
        cam.draw(0, 0, ofGetWidth(), ofGetHeight());
    }

    // scale factor for drawing landmarks on top of video
    float sx = (float)ofGetWidth() / 1280.0f;
    float sy = (float)ofGetHeight() / 720.0f;

    auto& faces = tracker.getFaces();
    for (auto& face : faces) {
        float bScore = 0.5f;
        float jScore = 0.5f;

        // calculate scores
        auto bIt = blinkAnalyzers.find(face.id);
        if (bIt != blinkAnalyzers.end()) {
            bScore = bIt->second.getScore();
        }
        
        auto jIt = jitterAnalyzers.find(face.id);
        if (jIt != jitterAnalyzers.end()) {
            jScore = jIt->second.getScore();
        }

        float masterScore = (bScore + jScore) / 2.0f;

        // color the label and bbox by score: green=real, yellow: uncertain, red=fake
        ofColor statusColour = ofColor(255, 50, 50);
        if (masterScore >= 0.7f) statusColour = ofColor(0, 255, 0);
        else if (masterScore >= 0.4f) statusColour = ofColor(255, 255, 0);


        // draw bounding box      
        ofNoFill();
        ofSetColor(statusColour);
        ofSetLineWidth(2);
        ofDrawRectangle(face.bbox.x * sx, face.bbox.y * sy,
                        face.bbox.width * sx, face.bbox.height * sy);

        // draw landmarks
        ofFill();
        ofSetColor(0, 200, 255, 150);
        for (auto& pt : face.landmarks) {
            ofDrawCircle(pt.x * sx, pt.y * sy, 1.5);
        }

        // draw labels with analyzer info
        float labelX = face.bbox.x * sx;
        float labelY = face.bbox.y * sy;

        ofSetColor(statusColour);
        ofDrawBitmapString("Face " + ofToString(face.id) + " | Overall Score: " + ofToString(masterScore, 2), labelX, labelY - 40);

        if (bIt != blinkAnalyzers.end()) {
            ofDrawBitmapString("BLINK | EAR: " + ofToString(bIt->second.getEAR(), 2)
                             + " BPM: " + ofToString(bIt->second.getBPM(), 2)
                             + " Score: " + ofToString(bScore, 2),
                             labelX, labelY - 25);
        }

        if (jIt != jitterAnalyzers.end()) {
            ofDrawBitmapString("JITTER| Var: " + ofToString(jIt->second.getVariance(), 2)
                             + " Jump: "+ ofToString(jIt->second.getMaxJump(), 1) 
                             + " Score: " + ofToString(jScore, 2),
                             labelX, labelY - 10);
        }
    }

    // HUD
    ofSetColor(255);
    ofDrawBitmapString("DEEPFAKE DETECTOR", 10, 20);
    ofSetColor(150);
    ofDrawBitmapString("Faces: " + ofToString(tracker.count()), 10, 34);

    ofSetColor(100);
    ofDrawBitmapString("FPS: " + ofToString((int)ofGetFrameRate()),
                       ofGetWidth() - 70, ofGetHeight() - 10);
}

void ofApp::exit() {
    tracker.exit();
}
