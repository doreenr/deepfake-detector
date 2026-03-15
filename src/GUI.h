//
//  GUI.h
//  deepfake-detector
//

#pragma once
#include "ofMain.h"

enum class AuthenticityLevel { AUTHENTIC, UNCERTAIN, FAKE };

// Per-algorithm signal score for the sidebar
struct SignalScore {
    string label;   // e.g. "Blink Analysis"
    float  score;   // 0.0–1.0
    bool   active;  // false = placeholder / not yet implemented
};

class GUI {
public:
    void setup();
    void draw(const vector<SignalScore>& scores, float composite);
    float getSidebarWidth() const { return sidebarW; }

private:
    // ── sub-draw helpers ─────────────────────────────────────────────
    void drawBackground();
    void drawDivider(float y);
    void drawSourceButtons(float& cursorY);
    void drawTrackingScores(const vector<SignalScore>& scores, float& cursorY);
    void drawAuthenticitySection(float composite, float startY);
    void drawHorizontalTrafficLight(float cx, float cy, AuthenticityLevel level);
    void drawSignalRow(const SignalScore& s, float x, float y, float w);

    // ── fonts ─────────────────────────────────────────────────────────
    // All loaded from bin/data/ in setup():
    //   IBMPlexSans-Regular.ttf   → fontReg  (labels, body text)
    //   IBMPlexSans-SemiBold.ttf  → fontSemi (section titles, button text)
    //   IBMPlexSans-Bold.ttf      → fontBold (app title)
    //   IBMPlexSans-Bold.ttf      → fontLg   (large score number)
    ofTrueTypeFont fontReg;
    ofTrueTypeFont fontSemi;
    ofTrueTypeFont fontBold;
    ofTrueTypeFont fontTitle;  // larger bold font for the app title
    ofTrueTypeFont fontLg;

    // Draw string with baseline at (x, y); falls back to bitmap if not loaded.
    void txt(const ofTrueTypeFont& f, const string& s, float x, float y) const;
    // Pixel width of a string in the given font.
    float tw(const ofTrueTypeFont& f, const string& s) const;

    // ── layout constants ─────────────────────────────────────────────
    float sidebarW   = 760.0f;
    float padX       = 40.0f;
    float padY       = 48.0f;
    float btnH       = 88.0f;
    float btnGap     = 20.0f;
    float rowH       = 108.0f;
    float sectionGap = 40.0f;

    // ── misc helpers ─────────────────────────────────────────────────
    AuthenticityLevel scoreToLevel(float score) const;
    void drawRoundRect(float x, float y, float w, float h, float r,
                       ofColor fill, float alpha = 255.0f);
    void drawBar(float x, float y, float w, float h,
                 float value, ofColor barColor);
};
