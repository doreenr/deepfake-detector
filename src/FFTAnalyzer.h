//
//  FFTAnalyzer.h
//  deepfake-detector
//
//  Created by Doreen Reuchsel on 21.03.26.

#pragma once

#include "ofMain.h"
#include "opencv2/opencv.hpp"

class FFTAnalyzer {
public:
    void update(const cv::Mat& crop);
    void reset();
    float getScore() const { return score; }

private:
    float score = 0.5f;
    float computeHighFreqRatio(const cv::Mat& gray);
};
//

