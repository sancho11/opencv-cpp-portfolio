/**
 * @file interactive_scaler.cpp
 * @brief Interactive image scaling using OpenCV sliders (trackbars).
 *
 * Loads an image, shows UI to scale up/down in real time,
 * and allows saving the scaled result.
 *
 * Usage:
 *   ./interactive_image_scaler [input_image] [output_image]
 *
 * If no input is provided, defaults to DATA_DIR + "/truth.png".
 * If no output is provided, saves to "scaled_output.png" in current directory when pressing 's'.
 */

#include <filesystem>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "config.pch"  // defines DATA_DIR as a string literal

// Global constants and variables
static const std::filesystem::path kDefaultInput =
    std::filesystem::path(DATA_DIR) / "../data/truth.png";
static const int kMaxScale = 100;  ///< Trackbar max value for scale percentage
static const int kMaxType = 1;     ///< 0: Scale up, 1: Scale down

static cv::Mat originalImage;  ///< The loaded original image
static cv::Mat currentScaled;  ///< The most recent scaled image
static std::string windowName = "Resize Image";
static int scalePercent = 0;              ///< Trackbar position (0..kMaxScale)
static int scaleType = 0;                 ///< Trackbar position (0 or 1)
static std::filesystem::path outputPath;  ///< Where to save when user presses 's'

/**
 * @brief Loads an image and exits if it fails.
 * @param path   Filesystem path to load
 * @param flags  imread flags (default: IMREAD_UNCHANGED)
 * @return cv::Mat loaded image
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
 * @brief Saves an image and exits if it fails.
 * @param path    Destination path
 * @param image   cv::Mat to write
 * @param params  Optional imwrite parameters
 */
void saveImageOrExit(const std::filesystem::path& path, const cv::Mat& image,
                     const std::vector<int>& params = {}) {
  if (!cv::imwrite(path.string(), image, params)) {
    std::cerr << "ERROR: Failed to save image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "Saved scaled image to: " << path << std::endl;
}

/**
 * @brief Callback for trackbar events: rescales and displays the image.
 * @param, void*  Unused
 */
void onScaleChange(int, void*) {
  // Determine scale factor: up or down
  double factor =
      (scaleType == 0) ? (1.0 + scalePercent / 100.0) : std::max(0.01, 1.0 - scalePercent / 100.0);

  // Resize with INTER_LINEAR interpolation
  cv::resize(originalImage, currentScaled, cv::Size(), factor, factor, cv::INTER_LINEAR);
  cv::imshow(windowName, currentScaled);
}

int main(int argc, char* argv[]) {
  // Determine input and output paths
  std::filesystem::path inputPath = (argc > 1 ? argv[1] : kDefaultInput);
  outputPath =
      (argc > 2 ? std::filesystem::path(argv[2]) : std::filesystem::path("scaled_output.png"));

  // Load image
  originalImage = loadImageOrExit(inputPath);

  // Create display window and sliders
  cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
  cv::createTrackbar("Scale (%)", windowName, &scalePercent, kMaxScale, onScaleChange);
  cv::createTrackbar("Mode (0=up,1=down)", windowName, &scaleType, kMaxType, onScaleChange);

  // Initial display
  onScaleChange(0, nullptr);
  std::cout << "Press 's' to save, ESC to exit." << std::endl;

  // Event loop
  while (true) {
    int key = cv::waitKey(20);
    if (key == 27) {  // ESC
      break;
    }
    if (key == 's' || key == 'S') {
      saveImageOrExit(outputPath, currentScaled, {cv::IMWRITE_PNG_COMPRESSION, 3});
    }
  }
  return 0;
}