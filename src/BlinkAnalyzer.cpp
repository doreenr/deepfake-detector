#include "BlinkAnalyzer.h"

// MediaPipe 468-point face mesh eye landmarks
// we need 6 points per eye for the EAR formula, mapped the same way
// as the classic dlib approach: outer corner, 2 upper, inner corner, 2 lower
//
// left eye:  33 (outer), 160 (upper1), 158 (upper2), 133 (inner), 153 (lower2), 144 (lower1)
// right eye: 362 (outer), 385 (upper1), 387 (upper2), 263 (inner), 373 (lower2), 380 (lower1)
//
// the nice thing about 468 points is we get much denser coverage around the
// eye contour, so EAR is a bit more stable than with dlib's 6 points per eye.

static const int L_EYE[] = {33, 160, 158, 133, 153, 144};
static const int R_EYE[] = {362, 385, 387, 263, 373, 380};

void BlinkAnalyzer::reset() {
    earHistory.clear();
    blinkTimes.clear();
    closedFrames = 0;
    wasClosed = false;
    currentEAR = 0;
    bpm = 0;
    score = 0.5f;
}

float BlinkAnalyzer::computeEAR(const vector<glm::vec2>& lm, bool left) {
    const int* idx = left ? L_EYE : R_EYE;

    // EAR = (|p2-p6| + |p3-p5|) / (2 * |p1-p4|)
    // from Soukupová & Čech 2016, adapted for MediaPipe indices
    float v1 = glm::distance(lm[idx[1]], lm[idx[5]]);
    float v2 = glm::distance(lm[idx[2]], lm[idx[4]]);
    float h  = glm::distance(lm[idx[0]], lm[idx[3]]);

    if (h < 0.001f) return 0.3f;
    return (v1 + v2) / (2.0f * h);
}

void BlinkAnalyzer::update(const vector<glm::vec2>& landmarks) {
    if (landmarks.size() < 468) return;

    float leftEAR  = computeEAR(landmarks, true);
    float rightEAR = computeEAR(landmarks, false);
    currentEAR = (leftEAR + rightEAR) / 2.0f;

    earHistory.push_back(currentEAR);
    if (earHistory.size() > 300) earHistory.pop_front();

    float now = ofGetElapsedTimef();

    // blink = EAR drops below 0.21 for at least 2 frames
    bool closed = (currentEAR < 0.21f);
    if (closed) {
        closedFrames++;
    } else {
        if (wasClosed && closedFrames >= 2) {
            blinkTimes.push_back(now);
        }
        closedFrames = 0;
    }
    wasClosed = closed;

    // throw out blinks older than 30 sec
    while (!blinkTimes.empty() && (now - blinkTimes.front()) > 30.0f) {
        blinkTimes.pop_front();
    }

    // wait a bit before scoring
    if (now < 4.0f) { score = 0.5f; return; }

    float window = std::min(now, 30.0f);
    bpm = (blinkTimes.size() / window) * 60.0f;

    // rate scoring: normal is ~12-25, zero is very sus
    float rateScore = 1.0f;
    if (bpm < 2.0f)       rateScore = 0.1f;
    else if (bpm < 8.0f)  rateScore = 0.4f;
    else if (bpm > 40.0f) rateScore = 0.3f;

    // regularity scoring: real blinks are irregular
    float regScore = 1.0f;
    if (blinkTimes.size() >= 4) {
        vector<float> gaps;
        for (size_t i = 1; i < blinkTimes.size(); i++) {
            gaps.push_back(blinkTimes[i] - blinkTimes[i-1]);
        }
        float sum = 0, sumSq = 0;
        for (float g : gaps) { sum += g; sumSq += g * g; }
        float mean = sum / gaps.size();
        float var  = (sumSq / gaps.size()) - (mean * mean);
        float cv   = (mean > 0) ? sqrt(max(0.0f, var)) / mean : 0;

        // cv > 0.5 is normal human variability
        // cv < 0.15 means almost metronomic = fake
        if (cv < 0.15f)      regScore = 0.2f;
        else if (cv < 0.3f)  regScore = 0.5f;
    }

    score = rateScore * 0.6f + regScore * 0.4f;
}
