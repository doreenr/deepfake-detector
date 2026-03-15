#include "JitterAnalyzer.h"

void JitterAnalyzer::reset() {
    distanceHistory.clear();
    score = 0.5f;
}

void JitterAnalyzer::update(const std::vector<glm::vec2>& landmarks) {
    if (landmarks.size() < 468) return;

    glm::vec2 currentNosePos = landmarks[4];

    // calculate distance with previous frame
    if (distanceHistory.size() > 0 || previousPosition != glm::vec2(0,0)) {
        float dist = glm::distance(currentNosePos, previousPosition);
        distanceHistory.push_back(dist);

        if (distanceHistory.size() > maxHistory) {
            distanceHistory.pop_front();
        }

        // calculate variance once the buffer is full
        if (distanceHistory.size() == maxHistory) {
            float sum = 0.0f;

            for (float d : distanceHistory) {
                sum += d;
                if (d > maxJump) {
                    maxJump = d; // tracks largest movement
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

    previousPosition = currentNosePos;
}


void JitterAnalyzer::calculateScore() {
    if (ofGetElapsedTimef() < 4.0f) { score = 0.5f; return; }
    
    // variance score
    float varScore = 1.0f;
    if (currentVariance < 0.1f) {
        varScore = 0.1f; // too smoothed
    } else if (currentVariance < 0.5f) {
        varScore = 0.4f; // too still
    } else if (currentVariance > 15.0f) {
        varScore = 0.2f; // flickering
    } else if (currentVariance > 8.0f) {
        varScore = 0.5f; // too erratic
    }

    // max jump score
    float jumpScore = 1.0f;
    if (maxJump > 12.0f) {
        jumpScore = 0.1f; // big teleport
    } else if (maxJump > 8.0f) {
        jumpScore = 0.4f; // sharp change
    } else if (maxJump < 0.1f) {
        jumpScore = 0.2f; // frozen image
    }

    // weighted sum of variance and max jump
    score = (varScore * 0.6f) + (jumpScore * 0.4f);
}