#include "ofApp.h"
#include "ofAppGLFWWindow.h"

// ─────────────────────────────────────────────────────────────────────────────
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
        { "Blink Analysis", 0.0f, false },
        { "Landmark Jitter Analysis", 0.0f, false },
        { "FFT Analysis",         0.0f, false },
        { "Colour Histogram Analysis",         0.0f, false },
    };
}

// ─────────────────────────────────────────────────────────────────────────────
void ofApp::update() {
    bool hasNewPixels = false;

    if (currentMode == SourceMode::CAMERA) {
        cam.update();
        if (cam.isInitialized() && cam.isFrameNew()) {
            videoPixels  = cam.getPixels();
            hasNewPixels = true;
        }
    }
    else if (currentMode == SourceMode::VIDEO) {
        videoPlayer.update();
        if (videoPlayer.isInitialized() && videoPlayer.isFrameNew()) {
            videoPixels  = videoPlayer.getPixels();
            videoPixels.setImageType(OF_IMAGE_COLOR);
            hasNewPixels = true;
        }
    }
    else if (currentMode == SourceMode::IMAGE) {
        // Static image – push pixels every frame so the tracker keeps running.
        if (loadedImage.isAllocated()) {
            videoPixels  = loadedImage.getPixels();
            hasNewPixels = true;
        }
    }

    if (hasNewPixels) {
        videoTexture.loadData(videoPixels);
        tracker.update(videoPixels);

        // Run analysis on each detected face
        for (auto& face : tracker.getFaces()) {
            blinkAnalyzers[face.id].update(face.landmarks, cv::Mat());
            jitterAnalyzers[face.id].update(face.landmarks, cv::Mat());
            fftAnalyzers[face.id].update(face.cropped);
            colourAnalyzers[face.id].update(face.landmarks, ofxCv::toCv(videoPixels));
        }

        // Wire scores into the sidebar – use face 0 if present
        const vector<string> labels = {
            "Blink analysis", 
            "Landmark jitter analysis", 
            "Spatial FFT analysis", 
            "Histogram colour analysis"
        };

        bool faceFound = ! tracker.getFaces().empty();

        for (int i = 0; i < signalScores.size(); i++) {
            signalScores[i].label = labels[i];
            signalScores[i].active = faceFound;

            if (faceFound) {
                int id = tracker.getFaces()[0].id;
                
                if (i == 0)
                    signalScores[i].score = blinkAnalyzers[id].getScore();
                else if (i == 1)
                    signalScores[i].score = jitterAnalyzers[id].getScore();
                else if (i == 2)
                    signalScores[i].score = fftAnalyzers[id].getScore();
                else if (i == 3)
                    signalScores[i].score = colourAnalyzers[id].getScore();
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
void ofApp::draw() {
    float sbW   = gui.getSidebarWidth();
    float areaX = sbW;
    float areaW = ofGetWidth()  - sbW;
    float areaH = ofGetHeight();

    float weights[] = {0.3f, 0.3f, 0.1f, 0.3f}; // blink, jitter, FFT, histogram


    // ── 1. Letterboxed video feed ─────────────────────────────────────
    // Use the actual frame dimensions so any source aspect ratio letterboxes
    // correctly (not just 1280x720 webcam feeds).
    float srcW = (videoTexture.getWidth()  > 0) ? videoTexture.getWidth()  : 1280.0f;
    float srcH = (videoTexture.getHeight() > 0) ? videoTexture.getHeight() : 720.0f;
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

    auto& faces = tracker.getFaces();
    for (auto& face : faces) {
        float b = blinkAnalyzers.count(face.id)  ? blinkAnalyzers[face.id].getScore()  : 0.5f;
        float j = jitterAnalyzers.count(face.id) ? jitterAnalyzers[face.id].getScore() : 0.5f;
        float f = fftAnalyzers.count(face.id)    ? fftAnalyzers[face.id].getScore()    : 0.5f;
        float c = colourAnalyzers.count(face.id) ? colourAnalyzers[face.id].getScore() : 0.5f;

        float masterScore = (b * weights[0] + j * weights[1] + f * weights[2] + c * weights[3]);
        float smoothedScore = smoothedComposite * 0.85f + masterScore * 0.15f;

        // color the label and bbox by score: green=real, yellow: uncertain, red=fake
        ofColor statusColour = ofColor(255, 50, 50);
        
        if (ofGetElapsedTimef() < 4.0f) 
            statusColour = ofColor(255, 255, 255);
        else if (masterScore >= 0.7f) 
            statusColour = ofColor(0, 255, 0);
        else if (masterScore >= 0.45f)
            statusColour = ofColor(255, 255, 0);


        // draw bounding box
        ofNoFill();
        ofSetColor(statusColour);
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

        // draw labels with analyzer info
        // float labelX = face.bbox.x * sx;
        // float labelY = face.bbox.y * sy;

        // ofSetColor(statusColour);
        // ofDrawBitmapString("Face " + ofToString(face.id) + " | Overall Score: " + ofToString(masterScore, 2), labelX, labelY - 40);

        // if (bIt != blinkAnalyzers.end()) {
        //     ofDrawBitmapString("BLINK | EAR: " + ofToString(bIt->second.getEAR(), 2)
        //                      + " BPM: " + ofToString(bIt->second.getBPM(), 2)
        //                      + " Score: " + ofToString(bScore, 2),
        //                      labelX, labelY - 25);
        // }

        // if (jIt != jitterAnalyzers.end()) {
        //     ofDrawBitmapString("JITTER| Var: " + ofToString(jIt->second.getVariance(), 2)
        //                      + " Jump: "+ ofToString(jIt->second.getMaxJump(), 1) 
        //                      + " Score: " + ofToString(jScore, 2),
        //                      labelX, labelY - 10);
        // }
    }

    // ── 3. Video-area HUD ─────────────────────────────────────────────
    ofSetColor(255);
    hudText(hudFont, sourceModeLabel(), drawX + 24, drawY + 42);

    // HUD
    ofSetColor(255);
    hudText(hudFont, "Faces detected: " + ofToString(tracker.count()),
            drawX + 24, drawY + 82);

    // FPS – bottom-right corner
    ofSetColor(120);
    string fps  = "FPS: " + ofToString((int)ofGetFrameRate());
    float  fpsW = hudFont.isLoaded() ? hudFont.stringWidth(fps) * 0.5f : fps.size() * 8.0f;
    hudText(hudFont, fps, ofGetWidth() - fpsW - 24, ofGetHeight() - 20);

    // ── 4. Sidebar (always on top) ────────────────────────────────────
    // Composite: blink score if active, otherwise 0.5 placeholder
    // count average for all signalScores
    // NEW: add lower weight for FFT algo
    float composite = 0.0f;
    float totalWeight = 0.0f;

    for (int i = 0; i < (int)signalScores.size(); i++) {
        if (signalScores[i].active) {
            composite += signalScores[i].score * weights[i];
            totalWeight += weights[i];
        }
    }
    // added smoothing
    composite = (totalWeight > 0.0f) ? (composite / totalWeight) : 0.5f;
    smoothedComposite = smoothedComposite * 0.85f + composite * 0.15f;
    gui.draw(signalScores, smoothedComposite);
}

// ─────────────────────────────────────────────────────────────────────────────
void ofApp::exit() {
    tracker.exit();
}

// ─────────────────────────────────────────────────────────────────────────────
void ofApp::keyPressed(int key) {

    // ── C: switch back to webcam ──────────────────────────────────────
    if (key == 'c' || key == 'C') {
        if (videoPlayer.isLoaded()) videoPlayer.stop();
        currentMode = SourceMode::CAMERA;
        resetTracker();
        return;
    }

    // ── U: upload a video or image file ──────────────────────────────
    if (key == 'u' || key == 'U') {
        ofFileDialogResult result = ofSystemLoadDialog("Select video or image");
        if (!result.bSuccess) return;

        string path = result.getPath();
        string ext  = ofToLower(ofFilePath::getFileExt(path));

        if (ext == "mp4" || ext == "mov" || ext == "avi") {
            if (videoPlayer.isLoaded()) videoPlayer.stop();
            videoPlayer.load(path);
            videoPlayer.setLoopState(OF_LOOP_NORMAL);
            videoPlayer.play();
            currentMode = SourceMode::VIDEO;
            resetTracker();
        }
        else if (ext == "jpg" || ext == "jpeg" || ext == "png") {
            loadedImage.load(path);
            loadedImage.setImageType(OF_IMAGE_COLOR);
            currentMode = SourceMode::IMAGE;
            resetTracker();
        }
        else {
            ofLogWarning("ofApp") << "Unsupported file type: " << ext;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ofApp::mousePressed(int x, int y, int button) {
    if (button != OF_MOUSE_BUTTON_LEFT) return;

    switch (gui.hitTest(x, y)) {
        case GUIButton::UPLOAD:
            // Reuse the same logic as pressing U
            ofApp::keyPressed('u');
            break;
        case GUIButton::WEBCAM:
            // Reuse the same logic as pressing C
            ofApp::keyPressed('c');
            break;
        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void ofApp::resetTracker() {
    // Do NOT call tracker.exit() here — that invokes PyShutdown(), which
    // permanently kills the Python interpreter for the lifetime of the process.
    // Calling setup() again on the existing tracker is enough to flush stale
    // state and re-initialise the MediaPipe pipeline.
    // clears also the analyzers
    tracker.setup();
    blinkAnalyzers.clear();
    jitterAnalyzers.clear();
    fftAnalyzers.clear();
    setup();
}

string ofApp::sourceModeLabel() const {
    switch (currentMode) {
        case SourceMode::CAMERA: return "Webcam";
        case SourceMode::VIDEO:  return "Video – " + ofFilePath::getBaseName(videoPlayer.getMoviePath());
        case SourceMode::IMAGE:  return "Image – " + ofFilePath::getBaseName(loadedImage.getPixels().isAllocated() ? "file" : "");
        default:                 return "";
    }
}
