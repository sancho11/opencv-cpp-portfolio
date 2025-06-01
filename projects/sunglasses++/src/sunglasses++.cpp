/**
 * @file sunglasses_plus_plus.cpp
 * @brief Interactive face filter application: overlays sunglasses, mustache, and visual effects on detected faces.
 *
 * This program:
 *   1. Loads images (webcam or static) and preloads accessory graphics (sunglasses, mustache, effects).
 *   2. Detects faces using Haar cascades and tracks them across frames with MultiTracker.
 *   3. Overlays sunglasses (with reflection and alpha blending) and mustache on each detected face.
 *   4. Applies additional visual effects on the glasses (Sobel-based scratch/flare) if selected.
 *   5. Allows user to tweak parameters via trackbars in real-time.
 *
 * Usage:
 *   ./sunglasses_plus_plus
 *
 * Controls (Options window):
 *   - Source: Select input (Webcam or one of the static images)
 *   - Glasses Image: Choose sunglasses style
 *   - Reflection Contrast: Adjust contrast of reflection texture [0..100]
 *   - Glasses Alpha: Adjust frame opacity of glasses [0..100]
 *   - Glasses Effect: Choose scratch/flare effect image
 *   - Effect Intensity: Control strength of effect [0..100]
 *   - Mustache Option: Choose mustache style
 *
 * Press ESC in the Input window to exit.
 */

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include "config.pch"

// Window names
const char* kOptionsWindow = "Options";
const char* kInputWindow   = "Input";

// Paths & Models
static const std::filesystem::path kDataDir     = DATA_DIR;
static const std::string kFaceModel = "models/haarcascade_frontalface_default.xml";
static constexpr int K_FRAMES_DETECTION = 20;  ///< force redetection every K frames

// Input sources and asset lists
static const std::vector<std::string> input_sources = {
    "Webcam", "musk.jpg", "face1.png", "face2.png", "face3.png", "face4.png"
};
static const std::string              glasses        = "sunglassRGB.png";
static const std::vector<std::string> glasses_images = {
    "none", "glasses1.png", "glasses2.png", "glasses5.png",
    "glasses4.png", "glasses3.png", "glasses6.png", "glasses7.png"
};
static const std::vector<std::string> effects_images = {
    "none", "effect1.png", "effect2.png", "effect3.png", "effect4.png"
};
static const std::vector<std::string> mustache_images = {
    "none", "mustache1.jpg", "mustache2.jpg", "mustache2.jpg",
    "mustache3.jpg", "mustache4.jpg", "mustache5.jpg", "mustache6.jpg",
    "mustache7.jpg", "mustache8.jpg", "mustache9.jpg", "mustache10.jpg",
    "mustache11.jpg", "mustache12.jpg", "mustache13.jpg", "mustache14.jpg"
};

// Global trackbar parameters (updated in callback)
static int g_srcIdx             = 0;  ///< index into input_sources
static int g_glassesImgIdx      = 0;  ///< index into glasses_images
static int g_reflectionContrast = 0;  ///< 0..100
static int g_glassesAlpha       = 0;  ///< 0..100
static int g_effectImgIdx       = 0;  ///< index into effects_images
static int g_effectIntensity    = 0;  ///< 0..100
static int g_mustacheOption     = 0;  ///< index into mustache_images

/**
 * @brief Trackbar callback: updates the referenced integer.
 */
void onTrackbar(int value, void* userdata) { *static_cast<int*>(userdata) = value; }

/**
 * @brief Convert face bounding boxes (Rect2d) to Rect (int).
 */
static std::vector<cv::Rect> rect2dToRect(const std::vector<cv::Rect2d>& vec2d) {
    std::vector<cv::Rect> rects;
    rects.reserve(vec2d.size());
    for (const auto& r2d : vec2d) {
        rects.emplace_back(r2d);
    }
    return rects;
}

/**
 * @brief Detect faces in a frame using Haar cascade (returns vector of bounding boxes).
 */
std::vector<cv::Rect> detectFaces(
    cv::CascadeClassifier& faceC,
    const cv::Mat& frame
) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);
    std::vector<cv::Rect> faces;
    faceC.detectMultiScale(
        gray, faces,
        1.1, 3,
        0 | cv::CASCADE_SCALE_IMAGE,
        cv::Size(100, 100)
    );
    return faces;
}

/**
 * @brief Decide whether to run full face detection (Haar) this frame.
 */
static bool decideWhetherToRedetect(
    int frameCount,
    int K_FRAMES_DETECTION,
    bool trackersInited,
    const cv::Ptr<cv::legacy::MultiTracker>& multiTracker,
    const cv::Mat& frame
) {
    if (!trackersInited) return true;
    if (frameCount >= K_FRAMES_DETECTION) return true;
    const auto vec2d = multiTracker->getObjects();
    for (const auto& r2d : vec2d) {
        cv::Rect box(static_cast<int>(r2d.x), static_cast<int>(r2d.y),
                     static_cast<int>(r2d.width), static_cast<int>(r2d.height));
        cv::Rect bounds(0, 0, frame.cols, frame.rows);
        if ((box & bounds).area() == 0) return true;
    }
    return false;
}


/**
 * @brief Initialize/recreate MultiTracker from fresh face detections.
 */
static void initializeTrackers(
    const std::vector<cv::Rect>& faces,
    const cv::Mat& frame,
    cv::Ptr<cv::legacy::MultiTracker>& multiTracker
) {
    multiTracker = cv::legacy::MultiTracker::create();
    for (const auto& det : faces) {
        auto tracker = cv::legacy::TrackerMedianFlow::create();
        multiTracker->add(tracker, frame, det);
    }
}


/**
 * @brief Update trackers on current frame (MultiTracker modifies boxes internally).
 */
static void updateTrackers(cv::Mat& frame,
    cv::Ptr<cv::legacy::MultiTracker>& multiTracker) {
multiTracker->update(frame);
}

/**
 * @brief Overlay sunglasses on detected faces with reflection and alpha blending.
 */
static void applyGlasses(
    cv::Mat& frame,
    const cv::Mat& glassesIMG,
    const std::vector<cv::Mat>& glasses_mats,
    int glassesIdx,
    int reflectionContrast,
    int glassesAlpha,
    const std::vector<cv::Rect>& faces,
    const std::vector<cv::Mat>& effects_mats,
    int effectIdx,
    int effectIntensity
) {
    if (glassesIdx <= 0) return;  // "none" selected
    cv::Mat baseGlasses = glassesIMG;
    // Create binary masks for frame and lens
    cv::Mat maskFrameBase, maskWholeBase;
    maskFrameBase  = cv::Mat::zeros(baseGlasses.size(), CV_8UC1);
    maskWholeBase  = cv::Mat::zeros(baseGlasses.size(), CV_8UC1);
    cv::inRange(baseGlasses, cv::Scalar(0,0,55),  cv::Scalar(255,255,254), maskFrameBase);
    cv::inRange(baseGlasses, cv::Scalar(0,0,0),   cv::Scalar(254,254,254), maskWholeBase);
    // Prepare reflection texture
    cv::Mat reflection = glasses_mats[glassesIdx];
    double alphaC = 0.5 + (reflectionContrast / 100.0)*2.0;
    double betaC  =    -(reflectionContrast / 100.0)*128;
    cv::Mat reflectionC;
    reflection.convertTo(reflectionC, -1, alphaC, betaC);

    for (const auto& face : faces) {
        int fw = face.width, fh = face.height;
        double aspect = static_cast<double>(baseGlasses.cols) / baseGlasses.rows;
        int outW = fw;
        int outH = static_cast<int>(fw / aspect);
        // Resize glasses and masks
        cv::Mat gRes, mFrameRes, mWholeRes;
        cv::resize(baseGlasses,    gRes,       {outW, outH}, 0, 0, cv::INTER_LINEAR);
        cv::resize(maskFrameBase,  mFrameRes,  {outW, outH}, 0, 0, cv::INTER_NEAREST);
        cv::resize(maskWholeBase,  mWholeRes,  {outW, outH}, 0, 0, cv::INTER_NEAREST);
        // Lens mask = whole - frame
        cv::Mat mLensRes;
        cv::subtract(mWholeRes, mFrameRes, mLensRes);
        // Build reflection texture concatenating left + right
        cv::Mat leftR  = reflectionC(cv::Range::all(), cv::Range(0, reflectionC.cols*3/4));
        cv::Mat rightR = reflectionC(cv::Range::all(), cv::Range(reflectionC.cols/4, reflectionC.cols));
        cv::Mat concatR;
        cv::hconcat(leftR, rightR, concatR);
        cv::Mat reflTex;
        cv::resize(concatR, reflTex, {outW, outH}, 0, 0, cv::INTER_LINEAR);
        // Overlay reflection onto glasses
        cv::Mat gWithRef = gRes.clone();
        reflTex.copyTo(gWithRef, mLensRes);
        // Build alpha channel: 255 for frame, alpha*255 for lens, 0 elsewhere
        cv::Mat alphaCh(outH, outW, CV_8UC1, cv::Scalar(0));
        uchar aVal = static_cast<uchar>(255.0 * glassesAlpha / 100.0);
        alphaCh.setTo(aVal, mLensRes);
        alphaCh.setTo(255, mFrameRes);
        // Define insertion position
        int xOff = face.x + (fw - outW)/2;
        int yOff = face.y + static_cast<int>(fh * 0.41) - outH/2;
        xOff = std::clamp(xOff, 0, frame.cols - outW);
        yOff = std::clamp(yOff, 0, frame.rows - outH);
        cv::Rect roiRect{xOff, yOff, outW, outH};
        cv::Mat roi = frame(roiRect);
        // Convert to float [0..1] for blending
        cv::Mat alphaF, alpha3;
        alphaCh.convertTo(alphaF, CV_32FC1, 1.0/255.0);
        cv::cvtColor(alphaF, alpha3, cv::COLOR_GRAY2BGR);  // CV_32FC3
        cv::Mat gF, roiF;
        gWithRef.convertTo(gF, CV_32FC3, 1.0/255.0);
        roi.convertTo(roiF, CV_32FC3, 1.0/255.0);
        cv::Mat blended = gF.mul(alpha3) + roiF.mul(cv::Scalar::all(1.0) - alpha3);
        // Apply optional effect
        if (effectIdx != 0) {
            cv::Mat effect = effects_mats[effectIdx];
            cv::Mat grayE; cv::cvtColor(effect, grayE, cv::COLOR_BGR2GRAY);
            cv::Mat sx, sy;
            cv::Sobel(grayE, sx, CV_32F, 1, 0, 3);
            cv::Sobel(grayE, sy, CV_32F, 0, 1, 3);
            cv::Mat mag;
            cv::magnitude(sx, sy, mag);
            cv::Mat magNorm;
            cv::normalize(mag, magNorm, 0, 255, cv::NORM_MINMAX, CV_8U);
            cv::Mat magRes;
            cv::resize(magNorm, magRes, blended.size());
            cv::Mat maskE;
            magRes.convertTo(maskE, CV_32F, 1.0/255.0);
            maskE = maskE.mul(alphaF);
            cv::threshold(maskE, maskE, 1.0f, 1.0f, cv::THRESH_TRUNC);
            cv::Mat hsv;
            cv::cvtColor(blended, hsv, cv::COLOR_BGR2HSV);
            std::vector<cv::Mat> chans(3);
            cv::split(hsv, chans);
            float coefS = 1.0f - effectIntensity/100.0f;
            float coefV = 1.0f + 30.0f * (effectIntensity/100.0f);
            cv::Mat sMod = chans[1].mul(1.0f - maskE) + chans[1].mul(coefS).mul(maskE);
            cv::Mat vMod = chans[2].mul(1.0f - maskE) + chans[2].mul(coefV).mul(maskE);
            cv::threshold(sMod, sMod, 1.0f, 1.0f, cv::THRESH_TRUNC);
            cv::threshold(vMod, vMod, 1.0f, 1.0f, cv::THRESH_TRUNC);
            sMod.copyTo(chans[1]);
            vMod.copyTo(chans[2]);
            cv::Mat hsvMod;
            cv::merge(chans, hsvMod);
            cv::cvtColor(hsvMod, blended, cv::COLOR_HSV2BGR);
        }
        // Write back to ROI
        blended.convertTo(roi, CV_8UC3, 255.0);
    }
}


/**
 * @brief Overlay mustache on detected faces by locating the red marker under the nose.
 */
static void applyMustache(
    cv::Mat& frame,
    const std::vector<cv::Mat>& mustache_mats,
    int mustacheIdx,
    const std::vector<cv::Rect>& faces
) {
    if (mustacheIdx <= 0) return;
    cv::Mat mustacheRaw = mustache_mats[mustacheIdx].clone();
    // Build base mask: pixels not "almost white" => part of mustache
    cv::Mat baseMask;
    cv::inRange(mustacheRaw, cv::Scalar(0,0,0), cv::Scalar(100,100,100), baseMask);
    // Detect red marker (~ under the nose)
    cv::Mat maskRed;
    cv::inRange(mustacheRaw, cv::Scalar(0,0,100), cv::Scalar(80,80,255), maskRed);
    cv::Moments m = cv::moments(maskRed, true);
    cv::Point redDot;
    if (m.m00 > 0) {
        redDot = {static_cast<int>(m.m10/m.m00), static_cast<int>(m.m01/m.m00)};
        // Remove red dot from mask
        cv::circle(mustacheRaw, redDot, 2, cv::Scalar(0), -1);
        cv::circle(baseMask, redDot, 2, cv::Scalar(255), -1);
    } else {
        redDot = {mustacheRaw.cols/2, mustacheRaw.rows/2};
    }
    for (const auto& face : faces) {
        int fw = face.width, fh = face.height;
        int outW = static_cast<int>(fw * 0.6);
        double aspect = static_cast<double>(mustacheRaw.cols) / mustacheRaw.rows;
        int outH = static_cast<int>(outW / aspect);
        cv::Mat msRes, mskRes;
        cv::resize(mustacheRaw, msRes, {outW, outH}, 0, 0, cv::INTER_LINEAR);
        cv::resize(baseMask, mskRes, {outW, outH}, 0, 0, cv::INTER_NEAREST);
        float scaleX = static_cast<float>(outW) / mustacheRaw.cols;
        float scaleY = static_cast<float>(outH) / mustacheRaw.rows;
        cv::Point rdRes{static_cast<int>(redDot.x*scaleX), static_cast<int>(redDot.y*scaleY)};
        int xOff = face.x + (fw - outW)/2;
        int yOff = face.y + static_cast<int>(fh * 0.65) - rdRes.y;
        xOff = std::clamp(xOff, 0, frame.cols - outW);
        yOff = std::clamp(yOff, 0, frame.rows - outH);
        cv::Rect roiRect{xOff, yOff, outW, outH};
        cv::Mat roi = frame(roiRect);
        cv::Mat mask3c;
        cv::cvtColor(mskRes, mask3c, cv::COLOR_GRAY2BGR);
        cv::Mat msF, roiF, maskF;
        msRes.convertTo(msF,  CV_32FC3, 1.0/255.0);
        roi.convertTo(roiF,   CV_32FC3, 1.0/255.0);
        mask3c.convertTo(maskF, CV_32FC3, 1.0/255.0);
        cv::Mat blended = msF.mul(maskF) + roiF.mul(cv::Scalar::all(1.0) - maskF);
        blended.convertTo(roi, CV_8UC3, 255.0);
    }
}


int main() {
    // 1. Create windows
    cv::namedWindow(kOptionsWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kOptionsWindow, 600, 400);
    cv::namedWindow(kInputWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kInputWindow, 600, 400);
    // 2. Create trackbars
    cv::createTrackbar("Source",            kOptionsWindow, nullptr,
                        static_cast<int>(input_sources.size()-1), onTrackbar, &g_srcIdx);
    cv::createTrackbar("Glasses Image",     kOptionsWindow, nullptr,
                        static_cast<int>(glasses_images.size()-1), onTrackbar, &g_glassesImgIdx);
    cv::createTrackbar("Reflection Contrast", kOptionsWindow, nullptr,
                        100, onTrackbar, &g_reflectionContrast);
    cv::createTrackbar("Glasses Alpha",     kOptionsWindow, nullptr,
                        100, onTrackbar, &g_glassesAlpha);
    cv::createTrackbar("Glasses Effect",    kOptionsWindow, nullptr,
                        static_cast<int>(effects_images.size()-1), onTrackbar, &g_effectImgIdx);
    cv::createTrackbar("Effect Intensity",  kOptionsWindow, nullptr,
                        100, onTrackbar, &g_effectIntensity);
    cv::createTrackbar("Mustache Option",   kOptionsWindow, nullptr,
                        static_cast<int>(mustache_images.size()-1), onTrackbar, &g_mustacheOption);

    // 3. Load Haar cascade for face detection
    cv::CascadeClassifier faceC;
    if (!faceC.load((kDataDir / "../data" / kFaceModel).string())) {
        std::cerr << "ERROR: Could not load face cascade." << std::endl;
        return -1;
    }
    // 4. Preload static images
    std::vector<cv::Mat> staticImages(input_sources.size());
    for (size_t i = 1; i < input_sources.size(); ++i) {
        cv::Mat img = cv::imread((kDataDir / "../data" / input_sources[i]).string());
        if (img.empty()) {
            std::cerr << "Error: Could not preload '" << input_sources[i] << "'." << std::endl;
        }
        staticImages[i] = img;
    }
    // 5. Preload sunglasses image and variants
    cv::Mat baseGlasses = cv::imread((kDataDir / "../data" / "sunglassRGB.png").string());
    if (baseGlasses.empty()) {
        std::cerr << "Error: Could not preload base sunglasses." << std::endl;
    }
    std::vector<cv::Mat> glassesMats(glasses_images.size());
    for (size_t i = 1; i < glasses_images.size(); ++i) {
        cv::Mat gimg = cv::imread((kDataDir / "../data" / glasses_images[i]).string());
        if (gimg.empty()) {
            std::cerr << "Error: Could not preload '" << glasses_images[i] << "'." << std::endl;
        }
        glassesMats[i] = gimg;
    }
    // 6. Preload effects
    std::vector<cv::Mat> effectsMats(effects_images.size());
    for (size_t i = 1; i < effects_images.size(); ++i) {
        cv::Mat eimg = cv::imread((kDataDir / "../data" / effects_images[i]).string());
        if (eimg.empty()) {
            std::cerr << "Error: Could not preload '" << effects_images[i] << "'." << std::endl;
        }
        effectsMats[i] = eimg;
    }
    // 7. Preload mustache images
    std::vector<cv::Mat> mustacheMats(mustache_images.size());
    for (size_t i = 1; i < mustache_images.size(); ++i) {
        cv::Mat mimg = cv::imread((kDataDir / "../data" / mustache_images[i]).string());
        if (mimg.empty()) {
            std::cerr << "Error: Could not preload '" << mustache_images[i] << "'." << std::endl;
        }
        mustacheMats[i] = mimg;
    }
    // 8. Initialize webcam
    cv::VideoCapture cap(0);
    if (cap.isOpened()) {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    } else {
        std::cout << "Warning: Cannot open webcam; static images only." << std::endl;
    }
    // 9. Setup multi-tracker
    cv::Ptr<cv::legacy::MultiTracker> multiTracker = cv::legacy::MultiTracker::create();
    bool trackersInited = false;
    int frameCount = 0;
    std::vector<cv::Rect> faceBoxes;

    // 10. Main loop
    while (true) {
        cv::Mat frame;
        // 10a. Acquire frame based on source selection
        if (g_srcIdx == 0 && cap.isOpened()) {
            cap >> frame;
            if (frame.empty()) break;
            ++frameCount;
            bool needRedetect = decideWhetherToRedetect(frameCount, K_FRAMES_DETECTION,
                    trackersInited, multiTracker, frame);
            if (needRedetect) {
                faceBoxes = detectFaces(faceC, frame);
                if (!faceBoxes.empty()) {
                    initializeTrackers(faceBoxes, frame, multiTracker);
                    trackersInited = true;
                }
                frameCount = 0;
            }
            if (trackersInited) {
                updateTrackers(frame, multiTracker);
                faceBoxes = rect2dToRect(multiTracker->getObjects());
            }
        } else {
            frame = staticImages[g_srcIdx].clone();
            if (frame.empty()) break;
            faceBoxes = detectFaces(faceC, frame);
        }
        // 10b. Overlay accessories
        applyGlasses(frame, baseGlasses, glassesMats, g_glassesImgIdx,
                     g_reflectionContrast, g_glassesAlpha,
                     faceBoxes, effectsMats, g_effectImgIdx, g_effectIntensity);
        applyMustache(frame, mustacheMats, g_mustacheOption, faceBoxes);
        // 10c. Display result
        cv::imshow(kInputWindow, frame);
        int key = cv::waitKey(30) & 0xFF;
        if (key == 27) break;  // ESC to exit
    }
    return 0;
}