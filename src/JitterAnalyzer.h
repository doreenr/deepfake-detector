#pragma once
#include "ofMain.h"
#include "Analyzer.h"

#include <map>
#include <deque>
#include <vector>

class JitterAnalyzer : public Analyzer {
    std::deque<float> distanceHistory;
    glm::vec2 previousPosition;
    glm::vec2 previousVelocity = glm::vec2(0,0);

    float currentVariance = 0.0f;
    float maxJump = 0.0f;

    int maxHistory = 30;
    float lowerThreshold = 0.1f;
    float upperThreshold = 10.0f;

public:
    void update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) override;
    void reset() override;
    void calculateScore();
    float getVariance() const { return currentVariance; }
    float getMaxJump() const { return maxJump; }
};