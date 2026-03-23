#pragma once

#include "Analyzer.h"

// looks for edge artifacts along the face boundary.
// deepfakes often have unnatural transitions at the blending line
// between the generated face and the original background/neck.
// we run Canny on a strip around the face silhouette and compare
// edge density inside vs outside — a big mismatch is suspicious.

class EdgeAnalyzer : public Analyzer {
public:
    void update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) override;
    void reset() override;

    float getEdgeRatio() const { return edgeRatio; }

private:
    float edgeRatio = 0.0f;

    // MediaPipe face silhouette indices (outer boundary of the face)
    const std::vector<int> silhouetteIdx = {
        10, 338, 297, 332, 284, 251, 389, 356, 454, 323, 361, 288,
        397, 365, 379, 378, 400, 377, 152, 148, 176, 149, 150, 136,
        172, 58, 132, 93, 234, 127, 162, 21, 54, 103, 67, 109
    };

    void calculateScore();
};
