/**
 * @file feature_alignment.cpp
 * @brief Aligns color channels by matching ORB features and warping, then demonstrates the result.
 *
 * This tool:
 *  1. Loads a 3-channel image stored as three stacked grayscale regions.
 *  2. Splits it into blue, green, and red channels (top, middle, bottom).
 *  3. Detects ORB features and descriptors on each channel.
 *  4. Matches features (green↔blue, green↔red), filters by quality.
 *  5. Estimates homographies via RANSAC and warps blue and red onto green.
 *  6. Merges channels back into a color image and displays comparison.
 *
 * Usage:
 *   ./feature_alignment [input_image]
 *
 * If input_image is not provided, defaults to DATA_DIR/../data/emir.jpg
 */

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "config.pch"  // Defines DATA_DIR macro

static const std::filesystem::path kDefaultInput =
    std::filesystem::path(DATA_DIR) / "../data/emir.jpg";
static constexpr int kMaxFeatures = 20000;
static constexpr float kGoodMatchRate = 0.005f;

/**
 * @brief Load an image or exit on failure.
 * @param path Filesystem path to image
 * @param flags imread flags (default: unchanged)
 * @return Loaded cv::Mat
 */
cv::Mat loadImageOrExit(const std::filesystem::path& path, int flags = cv::IMREAD_GRAYSCALE) {
  cv::Mat img = cv::imread(path.string(), flags);
  if (img.empty()) {
    std::cerr << "ERROR: Failed to load image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return img;
}

/**
 * @brief Display a vector of images in a single window grid.
 * @param images List of cv::Mat to display
 * @param rows   Number of rows
 * @param cols   Number of columns
 * @param winName Window name
 */
void displayGrid(const std::vector<cv::Mat>& images, int rows, int cols,
                 const std::string& winName) {
  if (images.empty()) return;
  int cellW = images[0].cols;
  int cellH = images[0].rows;
  cv::Mat canvas(cellH * rows, cellW * cols, images[0].type());
  for (size_t i = 0; i < images.size(); ++i) {
    int r = i / cols;
    int c = i % cols;
    cv::Rect roi(c * cellW, r * cellH, images[i].cols, images[i].rows);
    images[i].copyTo(canvas(roi));
  }
  cv::namedWindow(winName, cv::WINDOW_NORMAL);
  cv::imshow(winName, canvas);
  cv::waitKey(0);
  cv::destroyWindow(winName);
}

/**
 * @brief Detect ORB keypoints and descriptors.
 * @param img Input image
 * @param keypoints Output keypoints
 * @param descriptors Output descriptors
 */
void detectAndCompute(const cv::Mat& img, std::vector<cv::KeyPoint>& keypoints,
                      cv::Mat& descriptors) {
  auto orb = cv::ORB::create(kMaxFeatures);
  orb->detectAndCompute(img, {}, keypoints, descriptors);
}

/**
 * @brief Match descriptors and keep the top percentage by score.
 * @param desc1 Descriptors of image 1 (query)
 * @param desc2 Descriptors of image 2 (train)
 * @param matches Output filtered matches
 */
void matchAndFilter(const cv::Mat& desc1, const cv::Mat& desc2, std::vector<cv::DMatch>& matches) {
  cv::Ptr<cv::DescriptorMatcher> matcher =
      cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE_HAMMING);
  matcher->match(desc1, desc2, matches);
  std::sort(matches.begin(), matches.end(),
            [](auto& a, auto& b) { return a.distance < b.distance; });
  int nGood = static_cast<int>(matches.size() * kGoodMatchRate);
  matches.resize(std::max(nGood, 1));
}

/**
 * @brief Compute homography from matched keypoints.
 * @param kp1 Keypoints of image 1 (query)
 * @param kp2 Keypoints of image 2 (train)
 * @param matches Filtered matches
 * @return 3x3 homography matrix
 */
cv::Mat computeHomography(const std::vector<cv::KeyPoint>& kp1,
                          const std::vector<cv::KeyPoint>& kp2,
                          const std::vector<cv::DMatch>& matches) {
  std::vector<cv::Point2f> pts1, pts2;
  pts1.reserve(matches.size());
  pts2.reserve(matches.size());
  for (const auto& m : matches) {
    pts1.push_back(kp1[m.queryIdx].pt);
    pts2.push_back(kp2[m.trainIdx].pt);
  }
  return cv::findHomography(pts2, pts1, cv::RANSAC);
}

int main(int argc, char* argv[]) {
  // Determine input path
  std::filesystem::path inputPath = (argc > 1) ? argv[1] : kDefaultInput;

  // Load grayscale image with three stacked channels
  cv::Mat stacked = loadImageOrExit(inputPath);

  int h = stacked.rows / 3;
  int w = stacked.cols;

  // Split into B, G, R
  cv::Mat blue = stacked(cv::Rect(0, 0, w, h));
  cv::Mat green = stacked(cv::Rect(0, h, w, h));
  cv::Mat red = stacked(cv::Rect(0, 2 * h, w, h));
  displayGrid({blue, green, red}, 1, 3, "Channels");

  // Detect and compute features
  std::vector<cv::KeyPoint> kpB, kpG, kpR;
  cv::Mat descB, descG, descR;
  detectAndCompute(blue, kpB, descB);
  detectAndCompute(green, kpG, descG);
  detectAndCompute(red, kpR, descR);

  // Draw keypoints
  cv::Mat imKB, imKG, imKR;
  cv::drawKeypoints(blue, kpB, imKB, cv::Scalar(255));
  cv::drawKeypoints(green, kpG, imKG, cv::Scalar(0, 255));
  cv::drawKeypoints(red, kpR, imKR, cv::Scalar(0, 0, 255));
  displayGrid({imKB, imKG, imKR}, 1, 3, "Keypoints");

  // Match and filter
  std::vector<cv::DMatch> matchesBG, matchesRG;
  matchAndFilter(descG, descB, matchesBG);
  matchAndFilter(descG, descR, matchesRG);

  // Draw matches
  cv::Mat mBG, mRG;
  cv::drawMatches(green, kpG, blue, kpB, matchesBG, mBG);
  cv::drawMatches(green, kpG, red, kpR, matchesRG, mRG);
  displayGrid({mBG, mRG}, 1, 2, "Matches G-B | G-R");

  // Compute homographies
  cv::Mat H_BtoG = computeHomography(kpG, kpB, matchesBG);
  cv::Mat H_RtoG = computeHomography(kpG, kpR, matchesRG);

  // Warp channels
  cv::Mat blueWarp, redWarp;
  cv::warpPerspective(blue, blueWarp, H_BtoG, cv::Size(w, h));
  cv::warpPerspective(red, redWarp, H_RtoG, cv::Size(w, h));
  displayGrid({blueWarp, redWarp}, 1, 2, "Warped Channels");

  // Merge and compare
  cv::Mat mergedOriginal;
  cv::merge(std::vector<cv::Mat>{blue, green, red}, mergedOriginal);
  cv::Mat mergedAligned;
  cv::merge(std::vector<cv::Mat>{blueWarp, green, redWarp}, mergedAligned);
  displayGrid({mergedOriginal, mergedAligned}, 1, 2, "Original vs Aligned");

  return EXIT_SUCCESS;
}