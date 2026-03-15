#include "FaceTracker.h"
#include "ofxCv.h"

namespace mp = ofx::MediaPipe;

void FaceTracker::setup() {
    mpTracker = make_shared<mp::FaceTracker>();
    mp::FaceTracker::FaceSettings settings;
    settings.maxNum = 2;
    settings.minDetectionConfidence = 0.25f;
    settings.minPresenceConfidence = 0.25f;
    settings.minTrackingConfidence = 0.25f;
    settings.runningMode = mp::Tracker::MODE_VIDEO;
    settings.outputFaceBlendshapes = true;
    mpTracker->setup(settings);
}

cv::Rect FaceTracker::computeBBox(const vector<glm::vec2>& landmarks,
                                   int imgW, int imgH) {
    float minX = imgW, minY = imgH, maxX = 0, maxY = 0;
    for (auto& pt : landmarks) {
        minX = std::min(minX, pt.x);
        minY = std::min(minY, pt.y);
        maxX = std::max(maxX, pt.x);
        maxY = std::max(maxY, pt.y);
    }
    float padX = (maxX - minX) * 0.1f;
    float padY = (maxY - minY) * 0.1f;
    int x = std::max(0, (int)(minX - padX));
    int y = std::max(0, (int)(minY - padY));
    int w = std::min(imgW - x, (int)(maxX - minX + padX * 2));
    int h = std::min(imgH - y, (int)(maxY - minY + padY * 2));
    return cv::Rect(x, y, w, h);
}

void FaceTracker::update(ofPixels& pixels) {
    mpTracker->process(pixels);

    faces.clear();

    auto& mpFaces = mpTracker->getFaces();
    int imgW = pixels.getWidth();
    int imgH = pixels.getHeight();

    for (auto& mpFace : mpFaces) {
        if (!mpFace || mpFace->keypoints.size() < 468) continue;

        Face f;
        f.id = mpFace->ID;

        // convert 3d keypoints to 2d landmarks scaled to image size
        for (auto& kp : mpFace->keypoints) {
            f.landmarks.push_back(glm::vec2(kp.pos.x, kp.pos.y));
        }

        f.bbox = computeBBox(f.landmarks, imgW, imgH);

        // crop the face region from the frame
        cv::Mat frame = ofxCv::toCv(pixels);
        cv::Rect safe = f.bbox & cv::Rect(0, 0, imgW, imgH);
        if (safe.area() > 100) {
            f.cropped = frame(safe).clone();
        }

        faces.push_back(f);
    }
}

void FaceTracker::exit() {
    mpTracker.reset();
    mp::Tracker::PyShutdown();
}
