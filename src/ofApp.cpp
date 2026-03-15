#include "ofApp.h"
#include "ofAppGLFWWindow.h"

void ofApp::setup() {
    ofSetFrameRate(30);
    ofSetWindowTitle("Deepfake Detector");
    ofBackground(30, 30, 35);

    cam.setDeviceID(0);
    cam.setDesiredFrameRate(30);
    cam.setup(1280, 720);

    tracker.setup();
    gui.setup();

    // HUD fonts – same family as the sidebar
    hudFont    .load("IBMPlexSans-Regular.ttf",  48, true, true);
    hudFontSemi.load("IBMPlexSans-SemiBold.ttf", 52, true, true);

    // Placeholder signal scores
    signalScores = {
        { "Blink analysis", 0.0f, false },
        { "Algo 2",         0.0f, false },
        { "Algo 3",         0.0f, false },
    };
}

void ofApp::update() {
    cam.update();
    if (cam.isInitialized() && cam.isFrameNew()) {
        videoPixels = cam.getPixels();
        tracker.update(videoPixels);
        videoTexture.loadData(videoPixels);
    }

    // TODO: replace with real detector outputs, e.g.:
    // signalScores[0].score  = blinkDetector.getScore();
    // signalScores[0].active = true;
}

// Draws text at logical position (x,y). Font is loaded at 2× for Retina
// sharpness; we scale down by 0.5 so it renders at the correct logical size.
static void hudText(const ofTrueTypeFont& f, const string& s, float x, float y) {
    if (f.isLoaded()) {
        ofPushMatrix();
        ofTranslate(x, y);
        ofScale(0.5f, 0.5f);
        f.drawString(s, 0, 0);
        ofPopMatrix();
    } else {
        ofDrawBitmapString(s, x, y);
    }
}

void ofApp::draw() {
    float sbW   = gui.getSidebarWidth();
    float areaX = sbW;
    float areaW = ofGetWidth()  - sbW;
    float areaH = ofGetHeight();

    // ── 1. Letterboxed video feed ─────────────────────────────────────
    const float srcW = 1280.0f, srcH = 720.0f;
    float scale = std::min(areaW / srcW, areaH / srcH);
    float drawW = srcW * scale;
    float drawH = srcH * scale;
    float drawX = areaX + (areaW - drawW) * 0.5f;
    float drawY =         (areaH - drawH) * 0.5f;

    ofSetColor(255);
    if (videoTexture.getWidth() > 0)
        videoTexture.draw(drawX, drawY, drawW, drawH);
    else
        cam.draw(drawX, drawY, drawW, drawH);

    // ── 2. Face overlays ──────────────────────────────────────────────
    float sx = drawW / srcW;
    float sy = drawH / srcH;

    for (auto& face : tracker.getFaces()) {
        ofNoFill();
        ofSetColor(0, 255, 0);
        ofSetLineWidth(2);
        ofDrawRectangle(drawX + face.bbox.x * sx,
                        drawY + face.bbox.y * sy,
                        face.bbox.width  * sx,
                        face.bbox.height * sy);

        ofFill();
        ofSetColor(0, 200, 255, 150);
        for (auto& pt : face.landmarks)
            ofDrawCircle(drawX + pt.x * sx, drawY + pt.y * sy, 1.5f);

        ofSetColor(255);
        hudText(hudFont, "Face " + ofToString(face.id),
                drawX + face.bbox.x * sx,
                drawY + face.bbox.y * sy - 6);
    }

    // ── 3. Video-area HUD ─────────────────────────────────────────────
    ofSetColor(255);
    hudText(hudFont, "Webcam", drawX + 24, drawY + 42);

    ofSetColor(255);
    hudText(hudFont, "Faces detected: " + ofToString(tracker.count()),
            drawX + 24, drawY + 82);

    // FPS – bottom-right corner
    ofSetColor(120);
    string fps = "FPS: " + ofToString((int)ofGetFrameRate());
    float  fpsW = hudFont.isLoaded() ? hudFont.stringWidth(fps) * 0.5f : fps.size() * 8.0f;
    hudText(hudFont, fps, ofGetWidth() - fpsW - 24, ofGetHeight() - 20);

    // ── 4. Sidebar (always on top) ────────────────────────────────────
    float composite = 0.5f; // TODO: replace with real weighted score
    gui.draw(signalScores, composite);
}

void ofApp::exit() {
    tracker.exit();
}
