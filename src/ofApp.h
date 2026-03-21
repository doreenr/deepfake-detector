#pragma once

#include "ofMain.h"
#include "FaceTracker.h"
#include "GUI.h"
#include "BlinkAnalyzer.h"
#include "JitterAnalyzer.h"
#include "FFTAnalyzer.h"
#include <map>

// ── Source mode ───────────────────────────────────────────────────────────────
enum class SourceMode { CAMERA, VIDEO, IMAGE };

class ofApp : public ofBaseApp {
public:
    void setup()              override;
    void update()             override;
    void draw()               override;
    void exit()               override;
    void keyPressed(int key)  override;
    void mousePressed(int x, int y, int button) override;

private:
    // ── video sources ─────────────────────────────────────────────────
    ofVideoGrabber cam;
    ofVideoPlayer  videoPlayer;
    ofImage        loadedImage;
    SourceMode     currentMode = SourceMode::CAMERA;

    // ── shared frame data ─────────────────────────────────────────────
    ofPixels       videoPixels;
    ofTexture      videoTexture;

    // ── subsystems ────────────────────────────────────────────────────
    FaceTracker    tracker;
    GUI            gui;

    // ── blink analysis – one analyzer per face ID ─────────────────────
    map<int, BlinkAnalyzer> blinkAnalyzers;
    
    // ── jitter analysis – one analyzer per face ID ─────────────────────
    map<int, JitterAnalyzer> jitterAnalyzers;
    
    // ── FFT spatial analysis ─────────────────────
    map<int, FFTAnalyzer> fftAnalyzers;

    // ── HUD fonts ─────────────────────────────────────────────────────
    ofTrueTypeFont hudFont;
    ofTrueTypeFont hudFontSemi;

    // ── signal scores fed into the sidebar ────────────────────────────
    vector<SignalScore> signalScores;

    // ── helpers ───────────────────────────────────────────────────────
    void resetTracker();
    string sourceModeLabel() const;
};
