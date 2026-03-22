#include "ColourAnalyzer.h"
#include <iostream>

ColourAnalyzer::ColourAnalyzer() {
    reset();
}

void ColourAnalyzer::reset() {
    score = 0.5f;
    currentBhattacharyyaDist = 0.0f;
}

void ColourAnalyzer::update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) {
    if (landmarks.size() < 468 || frame.empty()) 
        return;

    // extract inner mask's silhouette from mesh grid
    std::vector<cv::Point> hull;
    for (int index : faceSilhouetteIndices) {
        hull.push_back(cv::Point(landmarks[index].x, landmarks[index].y));
    }

    // rasterize inner face mask
    cv::Mat innerMask = cv::Mat::zeros(frame.size(), CV_8UC1);
    cv::fillPoly(innerMask, std::vector<std::vector<cv::Point>>{hull}, cv::Scalar(255));

    // calculate outer skin ring thickness based on interocular distance
    float iod = glm::distance(landmarks[33], landmarks[362]); 
    int ringThickness = std::max(3, (int)(iod * 0.4f)); // 40% of eye distance
    
    // create outer skin ring
    cv::Mat dilatedMask;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ringThickness, ringThickness));
    cv::dilate(innerMask, dilatedMask, kernel);
    
    // subtract inner face from dilated mask to get only the ring
    cv::Mat outerMask = dilatedMask - innerMask; 

    // apply skin colour thresholding to outer ring
    cv::Mat ycrcbFrame;
    cv::cvtColor(frame, ycrcbFrame, cv::COLOR_BGR2YCrCb);
    cv::Mat skinThresholdMask;

    // standard openCV YCrCb skin ranges: Y(0-255), Cr(133-173), Cb(77-127)
    cv::inRange(ycrcbFrame, cv::Scalar(0, 133, 77), cv::Scalar(255, 173, 127), skinThresholdMask);
    
    // intersect geometric ring with skin pixels
    cv::Mat skinOnly;
    cv::bitwise_and(outerMask, skinThresholdMask, skinOnly);

    // fall back to raw geometric ring if pixels too few after filtering
    int skinPixelCount = cv::countNonZero(outerMask);

    if (skinPixelCount > 500) {
        outerMask = skinOnly;
    } else {
        std::cout << "WARNING: Skin filter starved. Falling back to raw ring." << std::endl;
    }

    // calculate histograms (Cb and Cr channels)
    int channels[] = {1, 2};
    int histSize[] = {32, 32};
    float crRanges[] = {0, 256};
    float cbRanges[] = {0, 256};
    const float* ranges[] = {crRanges, cbRanges};

    cv::Mat histInner, histOuter;
    
    cv::calcHist(&ycrcbFrame, 1, channels, innerMask, histInner, 2, histSize, ranges, true, false);
    cv::calcHist(&ycrcbFrame, 1, channels, outerMask, histOuter, 2, histSize, ranges, true, false);

    cv::normalize(histInner, histInner, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
    cv::normalize(histOuter, histOuter, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

    // Bhattacharyya Distance
    currentBhattacharyyaDist = cv::compareHist(histInner, histOuter, cv::HISTCMP_BHATTACHARYYA);

    calculateScore();
}

void ColourAnalyzer::calculateScore() {
    if (ofGetElapsedTimef() < 4.0f) { score = 0.5f; return; }

    std::cout << "Bhattacharyya Dist: " << currentBhattacharyyaDist << std::endl;

    float thresSafe = 0.92f;
    float thresFake = 0.98f;
    float targetScore = 0.5f;

    if (currentBhattacharyyaDist < thresSafe) { // real
        targetScore = 1.0f; 
    } 
    else if (currentBhattacharyyaDist < thresFake) { // uncertain, map distance to score
        targetScore = ofMap(currentBhattacharyyaDist, thresSafe, thresFake, 1.0f, 0.0f);
    } 
    else { // fake
        targetScore = 0.0f;
    }

    score = ofLerp(score, targetScore, 0.1f); // smooth score
}