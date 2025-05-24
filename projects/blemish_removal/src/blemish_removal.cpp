/**
 * @file blemish_removal.cpp
 * @brief Interactive blemish removal tool using OpenCV's seamless cloning.
 *
 * This program:
 *  1. Loads an input image containing blemishes.
 *  2. Opens a resizable window where the user can click on blemishes.
 *  3. For each click, selects an optimal source patch based on minimal texture variance.
 *  4. Applies seamless cloning to remove the blemish in-place.
 *  5. Allows the user to press 'C' to reset the image or 'Esc' to exit.
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>

#include "config.pch"

//--------------------------------------------------------------------------------------
// Configuration constants
//--------------------------------------------------------------------------------------

static const std::string kDataDir = DATA_DIR;  ///< Base data directory (from config.pch)
static const std::string kInputRelative = "../data/blemish.png";  ///< Relative path under DATA_DIR
static const std::string kOutputRelative = "../data/result.png";  ///< Relative path under DATA_DIR
static const std::string kWindowName = "Blemish Removal";  ///< Name of the interactive window
static constexpr int kDefaultRad = 20;                     ///< Radius of blemish removal patch

// Global image used by the mouse callback
static cv::Mat g_sourceImage;

/**
 * @brief Loads an image from disk, exiting on failure.
 * @param path      Full filesystem path to the image.
 * @param flags     OpenCV imread flags (default: IMREAD_UNCHANGED).
 * @return          Loaded cv::Mat.
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
 * @brief Computes the sum of squared Laplacian responses (texture variance) in a patch.
 * @param patch     Input image patch (BGR color).
 * @return          Scalar variance measure.
 */
double computePatchVariance(const cv::Mat& patch) {
  cv::Mat hsv, lap, lap2;
  std::vector<cv::Mat> channels;
  cv::cvtColor(patch, hsv, cv::COLOR_BGR2HSV);
  cv::split(hsv, channels);

  // Compute Laplacian on V channel to estimate texture
  cv::Laplacian(channels[2], lap, CV_32F, 3, 1.0 / (255.0 * 3 * 2), 0, cv::BORDER_DEFAULT);
  cv::pow(lap, 2.0, lap2);

  return cv::sum(lap2)[0];
}

/**
 * @brief Finds a nearby source patch with the lowest texture variance.
 * @param image     The full source image.
 * @param center    Center point of the blemish to remove.
 * @param radius    Radius of the blemish patch.
 * @return          Best-matching patch (same size as diameter×diameter).
 */
cv::Mat selectBestPatch(const cv::Mat& image, const cv::Point& center, int radius) {
  const int diameter = radius * 2 + 1;
  cv::Mat bestPatch;
  double minVariance = std::numeric_limits<double>::infinity();

  // Eight compass directions (unit vectors)
  static const std::vector<cv::Point2d> kDirs = {
      {1, 0},  {0.7071, 0.7071},   {0, 1},  {-0.7071, 0.7071},
      {-1, 0}, {-0.7071, -0.7071}, {0, -1}, {0.7071, -0.7071}};

  for (const auto& d : kDirs) {
    cv::Point srcCenter{static_cast<int>(center.x + d.x * radius * 2),
                        static_cast<int>(center.y + d.y * radius * 2)};

    // Define patch bounds
    int x0 = srcCenter.x - radius;
    int y0 = srcCenter.y - radius;
    int x1 = srcCenter.x + radius + 1;
    int y1 = srcCenter.y + radius + 1;

    // Skip if patch out of bounds
    if (x0 < 0 || y0 < 0 || x1 > image.cols || y1 > image.rows) continue;

    cv::Mat patch = image(cv::Range(y0, y1), cv::Range(x0, x1));
    double var = computePatchVariance(patch);

    if (var < minVariance) {
      minVariance = var;
      bestPatch = patch.clone();
    }
  }

  return bestPatch;
}

/**
 * @brief Mouse event callback: performs seamless cloning on left-click.
 */
void onMouse(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
  if (event != cv::EVENT_LBUTTONDOWN) return;

  cv::Point center{x, y};
  cv::Mat patch = selectBestPatch(g_sourceImage, center, kDefaultRad);
  if (patch.empty()) return;

  // Prepare binary mask for seamlessClone
  cv::Mat mask = cv::Mat::zeros(patch.size(), CV_8UC1);
  cv::circle(mask, cv::Point(kDefaultRad, kDefaultRad), kDefaultRad, cv::Scalar(255), cv::FILLED);

  cv::seamlessClone(patch, g_sourceImage, mask, center, g_sourceImage, cv::NORMAL_CLONE);
}

/**
 * @brief Launches the interactive blemish removal window.
 * @param image     Input image to process (will be modified in-place).
 */
void runBlemishRemoval(cv::Mat image) {
  g_sourceImage = image.clone();
  cv::namedWindow(kWindowName, cv::WINDOW_NORMAL);
  cv::resizeWindow(kWindowName, 1200, 900);
  cv::setMouseCallback(kWindowName, onMouse);

  std::cout << "Instructions:\n"
            << " - Left-click to remove blemish.\n"
            << " - Press 'C' to reset image.\n"
            << " - Press 'Esc' to exit.\n";

  cv::Mat original = image.clone();
  while (true) {
    cv::imshow(kWindowName, g_sourceImage);
    int key = cv::waitKey(20) & 0xFF;
    if (key == 27)  // Esc
      break;
    if (key == 'c' || key == 'C') {
      original.copyTo(g_sourceImage);
    }
  }
  cv::destroyAllWindows();
}

int main() {
  // Construct full path and load image
  std::filesystem::path inputImagePath = std::filesystem::path{kDataDir} / kInputRelative;
  cv::Mat input = loadImageOrExit(inputImagePath, cv::IMREAD_COLOR);

  runBlemishRemoval(input);

  // Save image
  std::filesystem::path outputImagePath = std::filesystem::path{kDataDir} / kOutputRelative;
  saveImageOrExit(outputImagePath, g_sourceImage);
  return EXIT_SUCCESS;
}
