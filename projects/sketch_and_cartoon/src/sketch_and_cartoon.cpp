/**
 * @file blemish_cartoon_sketch.cpp
 * @brief Generates a cartoonified and pencil‐sketch version of an input image.
 *
 * This program:
 *  1. Loads an input image from disk.
 *  2. Computes a pencil‐sketch mask by high‐pass filtering the V channel in HSV.
 *  3. Applies a bilateral filter to smooth colors while preserving edges.
 *  4. Merges the sketch mask with the smoothed color image to produce a cartoon effect.
 *  5. Displays both results in resizable windows.
 *  6. Saves the cartoon and sketch images back to disk.
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <vector>

#include "config.pch"

//--------------------------------------------------------------------------------------
// Configuration constants
//--------------------------------------------------------------------------------------

static const std::string kDataDir = DATA_DIR;  ///< Base data directory (from config.pch)
static const std::string kInputRelativePath = "../data/face.png";  ///< Relative path under DATA_DIR
static const std::string kOutputCartoonRelPath =
    "../data/resultCartoon.png";  ///< Relative output for cartoon
static const std::string kOutputSketchRelPath =
    "../data/resultSketch.png";                                      ///< Relative output for sketch
static const std::string kWindowCartoonName = "Cartoonified Image";  ///< Window title for cartoon
static const std::string kWindowSketchName = "Pencil Sketch Image";  ///< Window title for sketch

static constexpr int kGaussianKernelSize = 17;        ///< Kernel size for Gaussian blur (odd)
static constexpr double kGaussianSigma = 12.0;        ///< Sigma value for Gaussian blur
static constexpr int kSketchThreshold = 4;            ///< Threshold delta for sketch binarization
static constexpr int kBilateralDiameter = -1;         ///< Diameter for bilateral filter (auto)
static constexpr double kBilateralSigmaColor = 60.0;  ///< SigmaColor for bilateral filter
static constexpr double kBilateralSigmaSpace = 30.0;  ///< SigmaSpace for bilateral filter

//--------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------

/**
 * @brief Loads an image from disk, exiting on failure.
 * @param path   Full filesystem path to the image.
 * @param flags  OpenCV imread flags (default: IMREAD_UNCHANGED).
 * @return       Loaded cv::Mat.
 */
cv::Mat loadImageOrExit(const std::filesystem::path& path, int flags = cv::IMREAD_UNCHANGED) {
  cv::Mat img = cv::imread(path.string(), flags);
  if (img.empty()) {
    std::cerr << "ERROR: Could not load image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return img;
}

/**
 * @brief Saves an image to disk, exiting on failure.
 * @param path    Full filesystem path for output.
 * @param image   Image to write.
 * @param params  Optional imwrite parameters.
 */
void saveImageOrExit(const std::filesystem::path& path, const cv::Mat& image,
                     const std::vector<int>& params = {}) {
  if (!cv::imwrite(path.string(), image, params)) {
    std::cerr << "ERROR: Could not save image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

/**
 * @brief Displays an image in a resizable window until any key is pressed.
 * @param windowName  Title for the window.
 * @param image       Image to display.
 */
void showImage(const std::string& windowName, const cv::Mat& image) {
  cv::namedWindow(windowName, cv::WINDOW_NORMAL);
  cv::resizeWindow(windowName, 1200, 900);
  cv::imshow(windowName, image);
  cv::waitKey(0);
  cv::destroyWindow(windowName);
}

//--------------------------------------------------------------------------------------
// Core image‐processing functions
//--------------------------------------------------------------------------------------

/**
 * @brief Computes a binary pencil‐sketch mask from the V channel (HSV) of the image.
 *
 * Steps:
 *  1. Convert BGR → HSV and extract V channel.
 *  2. Gaussian‐blur V to obtain low‐frequency content.
 *  3. Subtract: high = blurredV − V to isolate dark strokes.
 *  4. Threshold with THRESH_BINARY_INV to produce white-on-black strokes.
 *
 * @param src   Input BGR image.
 * @return      Binary sketch mask (CV_8U).
 */
cv::Mat computeSketchMask(const cv::Mat& src) {
  // Convert to HSV and extract V channel
  cv::Mat hsv;
  cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
  std::vector<cv::Mat> channels;
  cv::split(hsv, channels);
  const cv::Mat& vChannel = channels[2];

  // Low‐pass (Gaussian blur)
  cv::Mat blurred;
  cv::GaussianBlur(vChannel, blurred, cv::Size(kGaussianKernelSize, kGaussianKernelSize),
                   kGaussianSigma, kGaussianSigma);

  // High‐pass approximation: blurred − original
  cv::Mat highPass = blurred - vChannel;

  // Binarize: dark strokes become white
  cv::Mat mask;
  cv::threshold(highPass, mask, kSketchThreshold, 255, cv::THRESH_BINARY_INV);

  return mask;
}

/**
 * @brief Produces a pencil‐sketch image from the source.
 * @param src   Input BGR image.
 * @return      Grayscale-looking sketch image.
 */
cv::Mat pencilSketch(const cv::Mat& src) {
  // The mask is white where strokes are desired
  return computeSketchMask(src);
}

/**
 * @brief Produces a cartoonified image by blending a sketch mask over smoothed color.
 *
 * Steps:
 *  1. Smooth colors with a bilateral filter.
 *  2. Invert sketch mask so strokes mask out the background.
 *  3. Apply mask to set non‐stroke regions to black in the smoothed color image.
 *
 * @param src   Input BGR image.
 * @return      Cartoonified BGR image.
 */
cv::Mat cartoonify(const cv::Mat& src) {
  // 1) Color smoothing
  cv::Mat smoothColor;
  cv::bilateralFilter(src, smoothColor, kBilateralDiameter, kBilateralSigmaColor,
                      kBilateralSigmaSpace);

  // 2) Sketch mask (white = strokes)
  cv::Mat sketchMask = computeSketchMask(src);

  // 3) Invert mask so strokes are 0, background 255
  cv::Mat invMask;
  cv::bitwise_not(sketchMask, invMask);

  // 4) Black-out background in the smoothColor image
  smoothColor.setTo(cv::Scalar(0, 0, 0), invMask);

  return smoothColor;
}

//--------------------------------------------------------------------------------------
// Main entry point
//--------------------------------------------------------------------------------------

int main() {
  // Build full input/output paths
  std::filesystem::path inputPath = std::filesystem::path{kDataDir} / kInputRelativePath;
  std::filesystem::path cartoonPath = std::filesystem::path{kDataDir} / kOutputCartoonRelPath;
  std::filesystem::path sketchPath = std::filesystem::path{kDataDir} / kOutputSketchRelPath;

  // Load source image
  cv::Mat src = loadImageOrExit(inputPath, cv::IMREAD_COLOR);

  // Generate outputs
  cv::Mat sketch = pencilSketch(src);
  cv::Mat cartoon = cartoonify(src);

  // Display results
  showImage(kWindowSketchName, sketch);
  showImage(kWindowCartoonName, cartoon);

  // Save to disk
  saveImageOrExit(sketchPath, sketch);
  saveImageOrExit(cartoonPath, cartoon);

  std::cout << "Saved pencil sketch to: " << sketchPath << "\n"
            << "Saved cartoon image to: " << cartoonPath << std::endl;

  return EXIT_SUCCESS;
}