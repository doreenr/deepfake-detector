#include "JitterAnalyzer.h"

void JitterAnalyzer::reset() {
    distanceHistory.clear();
    score = 0.5f;
    previousPosition = glm::vec2(0, 0);
    previousVelocity = glm::vec2(0, 0);
    maxJump = 0.0f;
    currentVariance = 0.0f;
}

void JitterAnalyzer::update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) {
    if (landmarks.size() < 468) return;

    glm::vec2 currentNosePos = landmarks[4];
    
    // calculate Interocular Distance (IOD) for Scale Normalization
    glm::vec2 leftEyeOuter = landmarks[33];
    glm::vec2 rightEyeOuter = landmarks[362];
    float iod = glm::distance(leftEyeOuter, rightEyeOuter);
    if (iod < 1.0f) 
        iod = 1.0f;

    // kinematic tracking (velocity & acceleration)
    if (previousPosition != glm::vec2(0, 0)) {
        glm::vec2 currentVelocity = currentNosePos - previousPosition;
        
        if (previousVelocity != glm::vec2(0, 0)) {
            glm::vec2 currentAcceleration = currentVelocity - previousVelocity;

            // normalized Jitter Magnitude
            float jitterMagnitude = glm::length(currentAcceleration) / iod;
            distanceHistory.push_back(jitterMagnitude);

            if (distanceHistory.size() > maxHistory) {
                distanceHistory.pop_front();
            }

            // calculate variance and max jump once buffer is full
            if (distanceHistory.size() == maxHistory) {
                float sum = 0.0f;
                maxJump = 0.0f; // reset

                for (float d : distanceHistory) {
                    sum += d;
                    if (d > maxJump) {
                        maxJump = d; // tracks largest movement between consecutive frames
                    }
                }
                float mean = sum / maxHistory;

                float variance = 0.0f;
                for (float d : distanceHistory) {
                    variance += std::pow(d - mean, 2.0f);
                }
                variance /= maxHistory;

                currentVariance = variance;
                calculateScore();
            }
        }
        previousVelocity = currentVelocity; 
    }
    previousPosition = currentNosePos;
}

void JitterAnalyzer::calculateScore() {

    std::cout << "Var: " << currentVariance << " | MaxJump: " << maxJump << std::endl;
    if (ofGetElapsedTimef() < 4.0f) { score = 0.5f; return; }
    
    // Variance score
    float varScore = 1.0f;
    if (currentVariance < 0.00008f) {
        varScore = 0.1f; // too smoothed (AI temporal smoothing)
    } else if (currentVariance < 0.0001f) { // suspiciously still
        varScore = ofMap(currentVariance, 0.00008f, 0.0001f, 0.1f, 0.5f, true);
    } else if (currentVariance > 0.05f) { // flickering (AI noise)
        varScore = 0.2f;
    } else if (currentVariance > 0.01f) { // suspiciously erratic
        varScore = ofMap(currentVariance, 0.01f, 0.02f, 0.1f, 0.5f, true);
    }

    // Max jump score
    float jumpScore = 1.0f;
    if (maxJump > 0.5f) {
        jumpScore = 0.1f; // big teleport (>50% of face width shift in 1 frame)
    } else if (maxJump > 0.1f) {
        jumpScore = 0.4f; // sharp change
    } else if (maxJump < 0.01f) {
        jumpScore = 0.2f; // frozen image
    }

    // weighted sum of variance and max jump
    score = ofLerp(score, varScore * 0.6f + jumpScore * 0.4f, 0.1f); // smooth score
}