/**
 * @file chroma_keying.cpp
 * @brief Interactive chroma‐key compositor for video streams using OpenCV.
 *
 * This program:
 *  1. Opens a video with a green‐screen subject.
 *  2. Lets the user click on the screen preview to sample the key color.
 *  3. Provides trackbars to adjust tolerance, mask softness, green‐spill correction, and
 * seamless‐clone mode.
 *  4. Generates a binary key mask in HSV space, softens it, corrects green spill, and composites
 * the subject over a background.
 *  5. Displays the result in real time and loops the video when it ends.
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "config.pch"

//----------------------------------------------------------------------------------
// Constants & Globals
//----------------------------------------------------------------------------------

const std::string kOptionsWindow = "Chroma Key Options";
const std::string kOutputWindow = "Chroma Key Output";
const std::filesystem::path kDataDir{DATA_DIR};

// Trackbar-controlled parameters
int g_keyR = 0;       ///< Sampled key color R component [0,255]
int g_keyG = 0;       ///< Sampled key color G component [0,255]
int g_keyB = 0;       ///< Sampled key color B component [0,255]
int g_tolerance = 0;  ///< Key color tolerance [%]
int g_softness = 0;   ///< Mask softness radius factor
int g_colorCast = 0;  ///< Green‐spill correction strength [%]
int g_seamless = 0;   ///< Use seamlessClone (1) or simple overlay (0)

cv::Mat g_currentFrame;  ///< Last captured frame for mouse sampling

//--------------------------------------------------------------------------------------
// Utility callbacks
//--------------------------------------------------------------------------------------

/**
 * @brief Mouse callback to sample the average BGR color in a 41×41 patch.
 */
void onMouseSampleColor(int event, int x, int y, int /*flags*/, void*) {
  if (event != cv::EVENT_LBUTTONDOWN) return;

  constexpr int radius = 20;
  int x0 = std::clamp(x - radius, 0, g_currentFrame.cols - 1);
  int y0 = std::clamp(y - radius, 0, g_currentFrame.rows - 1);
  int x1 = std::clamp(x + radius + 1, 0, g_currentFrame.cols);
  int y1 = std::clamp(y + radius + 1, 0, g_currentFrame.rows);

  cv::Mat patch = g_currentFrame(cv::Range(y0, y1), cv::Range(x0, x1));
  std::vector<cv::Mat> ch;
  cv::split(patch, ch);

  double sumB = cv::sum(ch[0])[0];
  double sumG = cv::sum(ch[1])[0];
  double sumR = cv::sum(ch[2])[0];
  int count = patch.rows * patch.cols;

  g_keyB = static_cast<int>(sumB / count);
  g_keyG = static_cast<int>(sumG / count);
  g_keyR = static_cast<int>(sumR / count);

  cv::setTrackbarPos("R", kOptionsWindow, g_keyR);
  cv::setTrackbarPos("G", kOptionsWindow, g_keyG);
  cv::setTrackbarPos("B", kOptionsWindow, g_keyB);
}

/**
 * @brief Generic trackbar callback to update an integer parameter.
 */
void onTrackbar(int value, void* userdata) { *static_cast<int*>(userdata) = value; }

//--------------------------------------------------------------------------------------
// Core processing functions
//--------------------------------------------------------------------------------------

/**
 * @brief Genera una máscara binaria donde los píxeles cercanos al color clave son blancos.
 * @param frame      Fotograma BGR de entrada.
 * @param keyBGR     Color clave muestreado en BGR.
 * @param tolerance  Tolerancia [%] para la selección del color.
 * @return           Máscara de un solo canal (CV_8U).
 */
cv::Mat createKeyMask(const cv::Mat& frame, const cv::Vec3b& keyBGR, int tolerance) {
  // Convertir el color clave BGR → HSV
  cv::Mat keyPix(1, 1, CV_8UC3, keyBGR);
  cv::cvtColor(keyPix, keyPix, cv::COLOR_BGR2HSV);
  cv::Vec3b keyHSV = keyPix.at<cv::Vec3b>(0, 0);

  // Convertir fotograma a HSV
  cv::Mat hsv;
  cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

  // Calc. de deltas en HSV según tolerancia
  double tol = tolerance / 100.0;
  int hDelta = static_cast<int>(180 * tol);
  int sDelta = static_cast<int>(255 * tol);
  int vDelta = static_cast<int>(255 * tol);

  // Calcular límites inferior y superior, primero en int
  int hLower = std::clamp<int>(static_cast<int>(keyHSV[0]) - hDelta, 0, 180);
  int sLower = std::clamp<int>(static_cast<int>(keyHSV[1]) - sDelta, 0, 255);
  int vLower = std::clamp<int>(static_cast<int>(keyHSV[2]) - vDelta, 0, 255);

  int hUpper = std::clamp<int>(static_cast<int>(keyHSV[0]) + hDelta, 0, 180);
  int sUpper = std::clamp<int>(static_cast<int>(keyHSV[1]) + sDelta, 0, 255);
  int vUpper = std::clamp<int>(static_cast<int>(keyHSV[2]) + vDelta, 0, 255);

  // Ahora construir cv::Scalar con paréntesis (evita warnings de lista inicializada)
  cv::Scalar lower(hLower, sLower, vLower);
  cv::Scalar upper(hUpper, sUpper, vUpper);

  // Generar máscara
  cv::Mat mask;
  cv::inRange(hsv, lower, upper, mask);
  return mask;
}

/**
 * @brief Converts a hard mask into a soft [0,1] floating‐point mask.
 * @param maskHard   8‐bit single‐channel binary mask.
 * @param softness   Softness factor (kernel radius = 2*softness+1).
 * @return           32‐bit single‐channel mask [0..1].
 */
cv::Mat softenMask(const cv::Mat& maskHard, int softness) {
  int ksize = 2 * softness + 1;
  cv::Mat inverted;
  cv::bitwise_not(maskHard, inverted);
  cv::GaussianBlur(inverted, inverted, cv::Size(ksize, ksize), 0);
  cv::Mat maskFloat;
  inverted.convertTo(maskFloat, CV_32F, 1.0 / 255.0);
  return maskFloat;
}

/**
 * @brief Saves an image to disk, exiting on failure.
 * @param path Full path where the image will be written.
 * @param image The cv::Mat image to save.
 * @param params Optional imwrite parameters (e.g., compression level).
 */
void saveImageOrExit(const std::filesystem::path& path, const cv::Mat& image,
                     const std::vector<int>& params = {}) {
  if (!cv::imwrite(path.string(), image, params)) {
    std::cerr << "ERROR: Failed to save image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

/**
 * @brief Attenuates green spill based on the soft mask.
 * @param frame         Original BGR frame.
 * @param maskSoft      Soft mask [0..1].
 * @param colorCastPct  Correction strength [%].
 * @return              Color‐cast–corrected BGR image.
 */
cv::Mat correctGreenSpill(const cv::Mat& frame, const cv::Mat& maskSoft, int colorCastPct) {
  cv::Mat frameF;
  frame.convertTo(frameF, CV_32F, 1.0 / 255.0);

  // Split channels
  std::vector<cv::Mat> ch(3);
  cv::split(frameF, ch);

  // Apply mask
  for (int i = 0; i < 3; ++i) ch[i] = ch[i].mul(maskSoft);

  // Calculate spill = max(B,R) - G, threshold at zero
  cv::Mat maxBR = cv::max(ch[0], ch[2]);
  cv::Mat spill;
  cv::subtract(ch[1], maxBR, spill);
  cv::threshold(spill, spill, 0.0f, 1.0f, cv::THRESH_TOZERO);

  // Subtract k*spill from G, redistribute half to B and R
  float k = colorCastPct / 100.0f;
  ch[1] = ch[1] - k * spill;
  cv::min(ch[1], 1.0f, ch[1]);
  cv::max(ch[1], 0.0f, ch[1]);

  ch[0] += 0.5f * k * spill;
  ch[2] += 0.5f * k * spill;

  // Merge and convert back to 8U
  cv::Mat corrected;
  cv::merge(ch, corrected);
  corrected.convertTo(corrected, CV_8U, 255.0);
  return corrected;
}

/**
 * @brief Composites the foreground over a background using soft mask or seamlessClone.
 * @param fg            Foreground BGR image (green‐spill corrected).
 * @param maskSoft      Soft mask [0..1].
 * @param bg            Background BGR image (same size as fg).
 * @param useSeamless   If true, use seamlessClone; otherwise simple blend.
 * @return              Composited BGR image.
 */
cv::Mat compositeFrame(const cv::Mat& fg, const cv::Mat& maskSoft, const cv::Mat& bg,
                       bool useSeamless) {
  if (!useSeamless) {
    // Simple soft blend
    cv::Mat bgF, fgF;
    bg.convertTo(bgF, CV_32F, 1.0 / 255.0);
    fg.convertTo(fgF, CV_32F, 1.0 / 255.0);

    cv::Mat invMask = 1.0f - maskSoft;
    std::vector<cv::Mat> bgCh(3), fgCh(3), outCh(3);
    cv::split(bgF, bgCh);
    cv::split(fgF, fgCh);

    for (int i = 0; i < 3; ++i) outCh[i] = bgCh[i].mul(invMask) + fgCh[i].mul(maskSoft);

    cv::Mat outF;
    cv::merge(outCh, outF);
    cv::Mat outU8;
    outF.convertTo(outU8, CV_8U, 255.0);
    return outU8;
  } else {
    // Seamless clone
    cv::Mat maskU8;
    maskSoft.convertTo(maskU8, CV_8U, 255.0);
    cv::Point center(bg.cols / 2, bg.rows / 2);
    cv::Mat output;
    cv::seamlessClone(fg, bg, maskU8, center, output, cv::NORMAL_CLONE);
    return output;
  }
}

//--------------------------------------------------------------------------------------
// Main processing loop
//--------------------------------------------------------------------------------------

/**
 * @brief Runs the interactive chroma‐key effect on a video file.
 * @param videoPath       Path to the green‐screen input video.
 * @param backgroundPath  Path to the replacement background image.
 */
void runChromaKey(const std::filesystem::path& videoPath,
                  const std::filesystem::path& backgroundPath) {
  // Open video
  cv::VideoCapture cap(videoPath.string(), cv::CAP_FFMPEG);
  if (!cap.isOpened()) {
    std::cerr << "ERROR: Cannot open video: " << videoPath << std::endl;
    return;
  }

  // Read first frame and load background
  cv::Mat frame;
  cap >> frame;
  if (frame.empty()) {
    std::cerr << "ERROR: Empty video: " << videoPath << std::endl;
    return;
  }

  cv::Mat bg = cv::imread(backgroundPath.string(), cv::IMREAD_COLOR);
  if (bg.empty()) {
    std::cerr << "ERROR: Cannot load background: " << backgroundPath << std::endl;
    return;
  }
  // Resize background to match frame size
  cv::resize(bg, bg, frame.size(), 0, 0, cv::INTER_AREA);

  // Create control window and trackbars
  cv::namedWindow(kOptionsWindow, cv::WINDOW_NORMAL);
  cv::resizeWindow(kOptionsWindow, 600, 400);
  cv::createTrackbar("R", kOptionsWindow, nullptr, 255, onTrackbar, &g_keyR);
  cv::createTrackbar("G", kOptionsWindow, nullptr, 255, onTrackbar, &g_keyG);
  cv::createTrackbar("B", kOptionsWindow, nullptr, 255, onTrackbar, &g_keyB);
  cv::createTrackbar("Tolerance", kOptionsWindow, nullptr, 100, onTrackbar, &g_tolerance);
  cv::createTrackbar("Softness", kOptionsWindow, nullptr, 10, onTrackbar, &g_softness);
  cv::createTrackbar("Color Cast", kOptionsWindow, nullptr, 100, onTrackbar, &g_colorCast);
  cv::createTrackbar("Seamless", kOptionsWindow, nullptr, 1, onTrackbar, &g_seamless);
  cv::setMouseCallback(kOptionsWindow, onMouseSampleColor);

  cv::namedWindow(kOutputWindow, cv::WINDOW_NORMAL);
  cv::resizeWindow(kOutputWindow, 640, 360);

  bool running = true;
  while (running) {
    // Loop frames
    if (frame.empty()) {
      // restart video
      cap.set(cv::CAP_PROP_POS_FRAMES, 0);
      cap >> frame;
      if (frame.empty()) break;
    }

    // Prepare for sampling
    g_currentFrame = frame.clone();
    cv::imshow(kOptionsWindow, frame);

    // Processing pipeline
    cv::Mat keyMask = createKeyMask(
        frame, {static_cast<uchar>(g_keyB), static_cast<uchar>(g_keyG), static_cast<uchar>(g_keyR)},
        g_tolerance);
    cv::Mat softMask = softenMask(keyMask, g_softness);
    cv::Mat fgCorrect = correctGreenSpill(frame, softMask, g_colorCast);
    cv::Mat composite = compositeFrame(fgCorrect, softMask, bg, g_seamless != 0);

    cv::imshow(kOutputWindow, composite);

    // Read next frame & handle key
    char c = static_cast<char>(cv::waitKey(25));
    if (c == 27)  // Esc
      running = false;
    auto dir = videoPath.parent_path();
    auto stem = videoPath.stem().string();
    std::string new_file_name = stem + ".example.jpg";
    saveImageOrExit(dir / new_file_name, composite);
    cap >> frame;
  }

  cv::destroyAllWindows();
}

//--------------------------------------------------------------------------------------
// Entry point
//--------------------------------------------------------------------------------------

int main() {
  // First demo
  runChromaKey(kDataDir / "../data" / "greenscreen-asteroid.mp4", kDataDir / "../data" / "IF1.jpg");

  // Reset parameters
  g_keyR = g_keyG = g_keyB = 0;
  g_tolerance = g_softness = g_colorCast = g_seamless = 0;

  // Second demo
  runChromaKey(kDataDir / "../data" / "greenscreen-demo.mp4",
               kDataDir / "../data" / "times-square.jpg");

  return EXIT_SUCCESS;
}