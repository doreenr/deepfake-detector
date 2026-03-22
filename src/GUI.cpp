//
//  GUI.cpp
//  deepfake-detector
//

#include "GUI.h"
#include "ofAppGLFWWindow.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
static const ofColor BG_SIDEBAR   (18,  20,  26);
static const ofColor BG_CARD      (30,  33,  42);
static const ofColor BG_CARD_DIM  (24,  27,  34);
static const ofColor COL_DIVIDER  (50,  55,  68);
static const ofColor COL_TITLE    (240, 240, 245);
static const ofColor COL_SUBTITLE (160, 165, 178);
static const ofColor COL_LABEL    (200, 205, 215);
static const ofColor COL_DIM      (90,  95,  110);

static const ofColor COL_GREEN      (40,  210,  80);
static const ofColor COL_YELLOW     (230, 190,  20);
static const ofColor COL_RED        (220,  40,  40);
static const ofColor COL_GREEN_DIM  (10,   55,  20);
static const ofColor COL_YELLOW_DIM (60,   50,  10);
static const ofColor COL_RED_DIM    (60,   15,  15);

// ─────────────────────────────────────────────────────────────────────────────
// Font helpers
// ─────────────────────────────────────────────────────────────────────────────

// Draw text with its baseline at (x, y).
// If the font failed to load we fall back to ofDrawBitmapString so the UI
// stays functional even with a missing font file.
void GUI::txt(const ofTrueTypeFont& f, const string& s, float x, float y) const {
    if (f.isLoaded()) {
        // Font is loaded at 2× size for Retina sharpness.
        // Scale down by 0.5 around the draw origin so it renders
        // at the correct logical size in the layout.
        ofPushMatrix();
        ofTranslate(x, y);
        ofScale(0.5f, 0.5f);
        f.drawString(s, 0, 0);
        ofPopMatrix();
    } else {
        ofDrawBitmapString(s, x, y);
    }
}

float GUI::tw(const ofTrueTypeFont& f, const string& s) const {
    if (f.isLoaded()) return f.stringWidth(s) * 0.5f;  // 2× font, halve the width
    return s.size() * 8.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::setup() {
    // ofTrueTypeFont::load(path, size, antialiased, fullCharSet, contours, simplify)
    // Paths are relative to bin/data/
    // Load at 2× point size to match Retina pixel density.
    // ofTrueTypeFont rasterises at load time, so the size must match the
    // actual pixel density — otherwise glyphs are upscaled and look blurry.
    // Load at 2× physical pixels for sharp Retina rendering.
    // We draw text with ofScale(0.5) so it appears at the correct logical size.
    fontReg  .load("IBMPlexSans-Regular.ttf",  48, true, true);
    fontSemi .load("IBMPlexSans-SemiBold.ttf", 52, true, true);
    fontBold .load("IBMPlexSans-Bold.ttf",     52, true, true);
    fontTitle.load("IBMPlexSans-Bold.ttf",     80, true, true);  // larger app title
    fontLg   .load("IBMPlexSans-Bold.ttf",     112, true, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main draw entry point
// ─────────────────────────────────────────────────────────────────────────────
void GUI::draw(const vector<SignalScore>& scores, float composite) {
    drawBackground();

    float cursorY = padY;

    // ── Header ────────────────────────────────────────────────────────
    cursorY += 46;
    ofSetColor(COL_TITLE);
    txt(fontTitle, "DeepFake Detector", padX, cursorY);
    cursorY += 100;

    // ── Source buttons ────────────────────────────────────────────────
    drawSourceButtons(cursorY);

    cursorY += 42;

    // ── Tracking scores ───────────────────────────────────────────────
    drawTrackingScores(scores, cursorY);

    cursorY += 42;

    // ── Authenticity section ──────────────────────────────────────────
    drawAuthenticitySection(composite, cursorY);
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawBackground() {
    ofSetColor(BG_SIDEBAR);
    ofDrawRectangle(0, 0, sidebarW, ofGetHeight());
    ofSetColor(COL_DIVIDER);
    ofDrawLine(sidebarW - 1, 0, sidebarW - 1, ofGetHeight());
}

void GUI::drawDivider(float y) {
    ofSetColor(COL_DIVIDER);
    ofDrawLine(padX, y, sidebarW - padX, y);
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawSourceButtons(float& cursorY) {
    float btnW = sidebarW - padX * 2;

    auto drawBtn = [&](const string& label, float y) {
        drawRoundRect(padX, y, btnW, btnH, 12, BG_CARD);
        ofSetColor(COL_LABEL);
        float labelW = tw(fontReg, label);
        txt(fontReg, label,
            padX + (btnW - labelW) * 0.5f,
            y + btnH * 0.5f + 10);
    };

    drawBtn("Upload video", cursorY);
    uploadBtnRect.set(padX, cursorY, btnW, btnH);
    cursorY += btnH + btnGap;

    drawBtn("Switch to webcam", cursorY);
    webcamBtnRect.set(padX, cursorY, btnW, btnH);
    cursorY += btnH + sectionGap;
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawTrackingScores(const vector<SignalScore>& scores, float& cursorY) {
    ofSetColor(COL_TITLE);
    txt(fontSemi, "Tracking scores", padX, cursorY + 26);
    cursorY += 56;

    float rowW = sidebarW - padX * 2;
    for (auto& s : scores) {
        drawSignalRow(s, padX, cursorY, rowW);
        cursorY += rowH + 6;
    }
    cursorY += sectionGap - 6;
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawSignalRow(const SignalScore& s, float x, float y, float w) {
    ofColor bg = s.active ? BG_CARD : BG_CARD_DIM;
    drawRoundRect(x, y, w, rowH, 12, bg);

    // Vertical divider splitting label column from score column
    float divX = x + w * 0.75f;
    ofSetColor(COL_DIVIDER);
    ofDrawLine(divX, y + 16, divX, y + rowH - 16);

    // Label — vertically centred in left column
    ofSetColor(s.active ? COL_LABEL : COL_DIM);
    txt(fontReg, s.label,
        x + 24,
        y + rowH * 0.5f + 10);

    // Score — vertically centred in right column
    ofSetColor(s.active ? COL_TITLE : COL_DIM);
    string val = ofToString(s.score, 2);
    float valW = tw(fontReg, val);
    float rightColCX = divX + (x + w - divX) * 0.5f;
    txt(fontReg, val,
        rightColCX - valW * 0.5f,
        y + rowH * 0.5f + 10);
}

// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawAuthenticitySection(float composite, float startY) {
    AuthenticityLevel level = scoreToLevel(composite);

    ofSetColor(COL_TITLE);
    txt(fontSemi, "Authenticity score", padX, startY + 26);
    float y = startY + 56;

    float cardW = sidebarW - padX * 2;
    float cardH = 144;
    drawRoundRect(padX, y, cardW, cardH, 16, BG_CARD);

    float tlCX = padX + 192;
    float tlCY = y + cardH * 0.5f;
    drawHorizontalTrafficLight(tlCX, tlCY, level);

    float divX = padX + cardW * 0.56f;
    ofSetColor(COL_DIVIDER);
    ofDrawLine(divX, y + 20, divX, y + cardH - 20);

    string scoreStr = ofToString(composite, 1);
    float  scoreW   = tw(fontLg, scoreStr);
    float  rightHalfCX = divX + (padX + cardW - divX) * 0.5f;
    ofSetColor(COL_TITLE);
    txt(fontLg, scoreStr, rightHalfCX - scoreW * 0.5f, tlCY + 20);

    y += cardH + 24;

    string label = (level == AuthenticityLevel::AUTHENTIC) ? "Result: authentic" :
                   (level == AuthenticityLevel::UNCERTAIN)  ? "Result: uncertain" :
                                                              "Result: fake";
    ofColor labelColor = (level == AuthenticityLevel::AUTHENTIC) ? COL_GREEN  :
                         (level == AuthenticityLevel::UNCERTAIN)  ? COL_YELLOW :
                                                                    COL_RED;
    ofSetColor(COL_TITLE);  // always white
    txt(fontReg, label, padX, y + 26);
}

void GUI::drawHorizontalTrafficLight(float cx, float cy, AuthenticityLevel level) {
    float bulbR   = 36;
    float spacing = 92;

    bool redOn   = (level == AuthenticityLevel::FAKE);
    bool yellOn  = (level == AuthenticityLevel::UNCERTAIN);
    bool greenOn = (level == AuthenticityLevel::AUTHENTIC);

    ofSetColor(redOn   ? COL_RED    : COL_RED_DIM);
    ofDrawCircle(cx - spacing, cy, bulbR);

    ofSetColor(yellOn  ? COL_YELLOW : COL_YELLOW_DIM);
    ofDrawCircle(cx, cy, bulbR);

    ofSetColor(greenOn ? COL_GREEN  : COL_GREEN_DIM);
    ofDrawCircle(cx + spacing, cy, bulbR);
}

// ─────────────────────────────────────────────────────────────────────────────
GUIButton GUI::hitTest(float x, float y) const {
    if (uploadBtnRect.inside(x, y)) return GUIButton::UPLOAD;
    if (webcamBtnRect.inside(x, y)) return GUIButton::WEBCAM;
    return GUIButton::NONE;
}

// ─────────────────────────────────────────────────────────────────────────────
AuthenticityLevel GUI::scoreToLevel(float score) const {
    if (ofGetElapsedTimef() < 4.0f) return AuthenticityLevel::UNCERTAIN;
    if (score >= 0.65f) return AuthenticityLevel::AUTHENTIC;
    if (score >= 0.45f || ofGetElapsedTimef() < 4.0f) return AuthenticityLevel::UNCERTAIN;
    return AuthenticityLevel::FAKE;
}

void GUI::drawRoundRect(float x, float y, float w, float h, float r,
                        ofColor fill, float alpha) {
    ofSetColor(fill.r, fill.g, fill.b, (int)alpha);
    ofDrawRectRounded(x, y, w, h, r);
}

void GUI::drawBar(float x, float y, float w, float h,
                  float value, ofColor barColor) {
    ofSetColor(COL_DIVIDER);
    ofDrawRectRounded(x, y, w, h, h * 0.5f);
    ofSetColor(barColor);
    float filled = ofClamp(value, 0, 1) * w;
    if (filled > 0)
        ofDrawRectRounded(x, y, filled, h, h * 0.5f);
}
