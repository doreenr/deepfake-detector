//
//  FFTAnalyzer.cpp
//  deepfake-detector
//
//  Created by Doreen Reuchsel on 21.03.26.
//

#include "FFTAnalyzer.h"

void FFTAnalyzer::reset() {
    score = 0.5f;
}

float FFTAnalyzer::computeHighFreqRatio(const cv::Mat& gray) {
    // Pad to optimal DFT size for performance
    int m = cv::getOptimalDFTSize(gray.rows);
    int n = cv::getOptimalDFTSize(gray.cols);
    
    cv::Mat padded;
    cv::copyMakeBorder(gray, padded,
                       0, m - gray.rows,
                       0, n - gray.cols,
                       cv::BORDER_CONSTANT, cv::Scalar::all(0));
    
    // Convert to float and build complex input [real, imaginary=0]
    cv::Mat floatImg;
    padded.convertTo(floatImg, CV_32F);
    
    cv::Mat planes[] = { floatImg, cv::Mat::zeros(floatImg.size(), CV_32F) };
    cv::Mat complex;
    cv::merge(planes, 2, complex);
    
    cv::dft(complex, complex);
    
    // Compute magnitude spectrum
    cv::split(complex, planes);
    cv::Mat mag;
    cv::magnitude(planes[0], planes[1], mag);
    mag += cv::Scalar::all(1);
    cv::log(mag, mag);
    
    // Crop to even size and rearrange quadrants (shift zero-freq to center)
    mag = mag(cv::Rect(0, 0, mag.cols & -2, mag.rows & -2));
    int cx = mag.cols / 2;
    int cy = mag.rows / 2;
    
    cv::Mat q0(mag, cv::Rect(0,  0,  cx, cy));
    cv::Mat q1(mag, cv::Rect(cx, 0,  cx, cy));
    cv::Mat q2(mag, cv::Rect(0,  cy, cx, cy));
    cv::Mat q3(mag, cv::Rect(cx, cy, cx, cy));
    
    cv::Mat tmp;
    q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
    q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);
    
    // High-freq mask: ring from 40% to 100% of half-width
    cv::Mat lowMask  = cv::Mat::zeros(mag.size(), CV_8U);
    cv::Mat highMask = cv::Mat::zeros(mag.size(), CV_8U);
    float innerR = std::min(cx, cy) * 0.4f;
    float outerR = std::min(cx, cy) * 1.0f;
    cv::circle(highMask, {cx, cy}, (int)outerR, 255, -1);
    cv::circle(lowMask,  {cx, cy}, (int)innerR, 255, -1);
        cv::Mat ringMask = highMask - lowMask;

        double totalEnergy = cv::sum(mag)[0];
    cv::Mat ringMaskF;
    ringMask.convertTo(ringMaskF, CV_32F, 1.0 / 255.0);
    double highEnergy = cv::sum(mag.mul(ringMaskF))[0];
        if (totalEnergy < 1e-6) return 0.5f;
        return (float)(highEnergy / totalEnergy);
    }

    void FFTAnalyzer::update(const cv::Mat& crop) {
        if (crop.empty() || crop.rows < 32 || crop.cols < 32) return;

        cv::Mat gray;
        if (crop.channels() == 3)
            cv::cvtColor(crop, gray, cv::COLOR_BGR2GRAY);
        else
            gray = crop.clone();

        float ratio = computeHighFreqRatio(gray);

        if      (ratio < 0.45f) score = 0.3f;
        else if (ratio < 0.55f) score = 0.6f;
        else if (ratio < 0.72f) score = 1.0f;
        else if (ratio < 0.80f) score = 0.5f;
        else                    score = 0.2f;
    }
