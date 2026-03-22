#pragma once

#include "Analyzer.h"
#include <vector>

class ColourAnalyzer : public Analyzer {
    float currentBhattacharyyaDist = 0.0f;

    // MediaPipe indices that form the outer edge of the face
    const std::vector<int> faceSilhouetteIndices = {
        10, 338, 297, 332, 284, 251, 389, 356, 454, 323, 361, 288, 
        397, 365, 379, 378, 400, 377, 152, 148, 176, 149, 150, 136, 
        172, 58, 132, 93, 234, 127, 162, 21, 54, 103, 67, 109
    };

    void calculateScore();

public:
    ColourAnalyzer();
  ofImage innerVisual;
    ofImage outerVisual;  
    void update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) override;
    void reset() override;
};