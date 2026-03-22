#pragma once

#include "ofMain.h"
#include "Analyzer.h"
#include <deque>

// tracks blink rate + regularity to detect fake faces.
// real people blink ~15-20x per minute with irregular gaps.
// deepfakes/photos usually don't blink at all, or blink too perfectly.

class BlinkAnalyzer : public Analyzer {
public:
    void update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) override;
    void reset() override;

    float getBPM() const { return bpm; }
    float getEAR() const { return currentEAR; }

private:
    float computeEAR(const vector<glm::vec2>& lm, bool left);

    deque<float> earHistory;
    deque<float> blinkTimes;
    int closedFrames = 0;
    bool wasClosed = false;
    float currentEAR = 0;
    float bpm = 0;
};
