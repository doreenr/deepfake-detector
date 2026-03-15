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

        // run blink analysis on each detected face
        auto& faces = tracker.getFaces();
        for (auto& face : faces) {
            blinkAnalyzers[face.id].update(face.landmarks);
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
        // draw bounding box
        ofNoFill();
        ofSetColor(0, 255, 0);
        ofSetLineWidth(2);
        ofDrawRectangle(face.bbox.x * sx, face.bbox.y * sy,
                        face.bbox.width * sx, face.bbox.height * sy);

        // draw landmarks
        ofFill();
        ofSetColor(0, 200, 255, 150);
        for (auto& pt : face.landmarks) {
            ofDrawCircle(pt.x * sx, pt.y * sy, 1.5);
        }

        // blink analyzer info
        auto it = blinkAnalyzers.find(face.id);
        if (it != blinkAnalyzers.end()) {
            auto& ba = it->second;
            float score = ba.getScore();

            // color the label by score: green=real, red=fake
            if (score >= 0.7f) ofSetColor(0, 255, 0);
            else if (score >= 0.4f) ofSetColor(255, 255, 0);
            else ofSetColor(255, 50, 50);

            float labelX = face.bbox.x * sx;
            float labelY = face.bbox.y * sy - 8;

            ofDrawBitmapString("Face " + ofToString(face.id), labelX, labelY);
            ofDrawBitmapString("EAR: " + ofToString(ba.getEAR(), 2)
                             + "  BPM: " + ofToString(ba.getBPM(), 1)
                             + "  Score: " + ofToString(score, 2),
                             labelX, labelY + 14);
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
