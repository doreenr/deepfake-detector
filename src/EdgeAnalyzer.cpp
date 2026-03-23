#include "EdgeAnalyzer.h"

void EdgeAnalyzer::reset() {
    edgeRatio = 0.0f;
    score = 0.5f;
}

void EdgeAnalyzer::update(const std::vector<glm::vec2>& landmarks, const cv::Mat& frame) {
    if (landmarks.size() < 468 || frame.empty()) return;

    int w = frame.cols;
    int h = frame.rows;

    // build the face silhouette contour from landmarks
    std::vector<cv::Point> contour;
    for (int idx : silhouetteIdx) {
        int x = std::clamp((int)landmarks[idx].x, 0, w - 1);
        int y = std::clamp((int)landmarks[idx].y, 0, h - 1);
        contour.push_back(cv::Point(x, y));
    }
    if (contour.size() < 10) return;

    // create inner and outer masks by dilating/eroding the contour
    // the "strip" between them is the blending boundary region
    cv::Mat contourMask = cv::Mat::zeros(h, w, CV_8UC1);
    std::vector<std::vector<cv::Point>> contours = {contour};
    cv::drawContours(contourMask, contours, 0, 255, cv::FILLED);

    int stripWidth = std::max(8, (int)(cv::boundingRect(contour).width * 0.08f));
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                cv::Size(stripWidth, stripWidth));

    cv::Mat outerMask, innerMask;
    cv::dilate(contourMask, outerMask, kernel);
    cv::erode(contourMask, innerMask, kernel);

    // the boundary strip = outer minus inner
    cv::Mat stripMask = outerMask - innerMask;

    // run Canny on the grayscale frame
    cv::Mat gray;
    if (frame.channels() == 3)
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    else
        gray = frame;

    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    // count edge pixels in the boundary strip vs inside the face
    int stripEdges = cv::countNonZero(edges & stripMask);
    int stripArea  = cv::countNonZero(stripMask);

    int innerEdges = cv::countNonZero(edges & innerMask);
    int innerArea  = cv::countNonZero(innerMask);

    if (stripArea < 10 || innerArea < 10) return;

    float stripDensity = (float)stripEdges / stripArea;
    float innerDensity = (float)innerEdges / innerArea;

    // ratio of boundary edge density to interior edge density
    // real faces: boundary edges are similar to interior (ratio ~1.0-2.0)
    // deepfakes: boundary has way more edges (sharp blending line) or way fewer
    //            (over-smoothed blending), so ratio is either very high or very low
    edgeRatio = (innerDensity > 0.001f) ? stripDensity / innerDensity : stripDensity * 100.0f;

    calculateScore();
}

void EdgeAnalyzer::calculateScore() {
    // normal range: ratio between 0.5 and 4.0
    // real faces have edge activity at the boundary (jaw, hairline, hair contrast)
    // which is often higher than the interior — especially with strong lighting
    if (edgeRatio >= 0.5f && edgeRatio <= 4.0f) {
        score = 1.0f;
    }
    // slightly outside normal
    else if (edgeRatio >= 0.3f && edgeRatio < 0.5f) {
        score = 0.6f;  // boundary smoother than expected
    }
    else if (edgeRatio > 4.0f && edgeRatio <= 6.0f) {
        score = 0.6f;  // boundary sharper than expected but could be lighting
    }
    // clearly abnormal
    else if (edgeRatio < 0.3f) {
        score = 0.2f;  // over-smoothed blending line
    }
    else {
        score = 0.2f;  // very sharp blending artifact
    }
}
