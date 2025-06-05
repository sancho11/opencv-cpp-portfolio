/**
 * @file document_scanner.cpp
 * @brief Document detection and perspective correction using homography.
 *
 * This program:
 *  1. Iterates over a list of sample images.
 *  2. Converts each image to HSV and extracts the V channel (intensity).
 *  3. Applies Gaussian blur, then Canny‐based edge detection, followed by morphological closing and dilation to produce a clean edge mask.
 *  4. Finds the largest quadrilateral contour in that mask (assumed to be the document).
 *  5. Sorts the four corner points in top‐left, top‐right, bottom‐left, bottom‐right order.
 *  6. Computes a homography mapping those corners to a fronto‐parallel rectangle.
 *  7. Warps the original image to that rectangle, yielding a perspective‐corrected “scanned” image.
 *  8. Displays the original and the corrected images side‐by‐side for visual verification.
 *
 * Note: All intermediate windows are resizable (1200×900) for inspection. The final side‐by‐side result is shown in “Scanned vs. Original.”
 */

 #include <cstdlib>
 #include <filesystem>
 #include <iostream>
 #include <vector>
 #include <cmath>
 
 #include <opencv2/opencv.hpp>
 #include "config.pch"  // Defines DATA_DIR macro
 
 //--------------------------------------------------------------------------------------
 // Configuration constants
 //--------------------------------------------------------------------------------------
 
 // Base data directory (from config.pch)
 static const std::filesystem::path kDataDir = DATA_DIR;
 
 // List of input image paths (relative to DATA_DIR)
 static const std::vector<std::string> kInputRelativePaths = {
  "../data/doc1.jpg",
  "../data/doc2.jpg",
  "../data/doc3.jpg",
  "../data/doc4.jpg",
  "../data/doc5.jpg",
  "../data/doc6.jpg"
};

// Gaussian blur parameters for low‐pass filtering on V channel
static constexpr int    kGaussianKernelSize = 17;   ///< must be odd (e.g., 17×17)
static constexpr double kGaussianSigma      = 3.3;  ///< standard deviation for Gaussian

// Canny edge‐detector thresholds
static constexpr double kCannyThresholdLow  = 25;
static constexpr double kCannyThresholdHigh = 40;
static constexpr int    kCannyAperture      = 3;    ///< the Sobel operator aperture size
static constexpr bool   kCannyL2Grad        = false;///< whether to use L2 gradient norm
static constexpr double kPerimeterEps       = 0.1;

// Morphological post‐processing parameters
static constexpr int MorphKSize  = 3;   ///< structuring element size (3×3)
static constexpr int kCloseSteps = 10;  ///< number of iterations for morphological close
static constexpr int kDilateSteps= 6;   ///< number of iterations for dilation

//--------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------

/**
* @brief Loads an image from disk, exiting on failure.
* @param path   Full filesystem path to the image.
* @param flags  OpenCV imread flags (default: IMREAD_COLOR).
* @return       Loaded cv::Mat.
*/
static cv::Mat loadImageOrExit(const std::filesystem::path& path, int flags = cv::IMREAD_COLOR) {
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
static void saveImageOrExit(const std::filesystem::path& path,
                          const cv::Mat& image,
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
static void showImage(const std::string& windowName, const cv::Mat& image) {
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
* @brief Computes a cleaned edge mask from the input intensity image.
*
* Steps:
*  1. Applies Gaussian blur to the V channel to suppress high‐frequency noise.
*  2. Runs Canny edge detector on the blurred image.
*  3. Performs morphological closing (cross‐shaped kernel) to close small gaps.
*  4. Performs dilation to thicken and connect edges.
*
* @param graySrc  Grayscale or single‐channel intensity image (CV_8U).
* @return         Binary edge mask (CV_8U) with white edges on black background.
*/
static cv::Mat computeEdgeMask(const cv::Mat& graySrc) {
  cv::Mat blurred;
  cv::GaussianBlur(graySrc, blurred,
                   cv::Size(kGaussianKernelSize, kGaussianKernelSize),
                   kGaussianSigma, kGaussianSigma);
  // Optional: uncomment to inspect blurred result
  // showImage("Blurred V Channel", blurred);

  cv::Mat edges;
  cv::Canny(blurred, edges,
            kCannyThresholdLow, kCannyThresholdHigh,
            kCannyAperture, kCannyL2Grad);

  // Morphological closing (CROSS) to close small gaps in edges
  cv::Mat kernel = cv::getStructuringElement(
      cv::MorphShapes::MORPH_CROSS,
      cv::Size(MorphKSize, MorphKSize)
  );
  cv::morphologyEx(edges, edges,
                   cv::MorphTypes::MORPH_CLOSE,
                   kernel, cv::Point(-1, -1), kCloseSteps);

  // Dilate to thicken edges
  cv::morphologyEx(edges, edges,
                   cv::MorphTypes::MORPH_DILATE,
                   kernel, cv::Point(-1, -1), kDilateSteps);

  return edges;
}

/**
* @brief Finds the largest quadrilateral contour in a binary edge mask.
*
* Scans all external contours, approximates each with polygonal curve,
* and retains the one with exactly 4 vertices and maximum area.
*
* @param edges   Binary edge map (CV_8U).
* @return        A vector of 4 cv::Point in arbitrary order if found; otherwise empty.
*/
static std::vector<cv::Point> findLargestQuad(const cv::Mat& edges) {
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(edges, contours,
                   cv::RetrievalModes::RETR_EXTERNAL,
                   cv::ContourApproximationModes::CHAIN_APPROX_SIMPLE);

  double maxArea    = 0.0;
  std::vector<cv::Point> bestQuad;

  for (const auto& c : contours) {
      double area = cv::contourArea(c);
      if (area <= maxArea) {
          continue;  // skip smaller ones
      }
      double perimeter = cv::arcLength(c, true);
      std::vector<cv::Point> approx;
      cv::approxPolyDP(c, approx,
                       kPerimeterEps * perimeter,
                       true);
      if (approx.size() == 4) {
          // found a quadrilateral larger than any before
          maxArea = area;
          bestQuad = approx;
      }
  }
  return bestQuad;  // may be empty if no 4‐vertex contour was found
}

/**
* @brief Orders four corner points in top-left, top-right, bottom-left, bottom-right order.
*
* Assumes `pts` has exactly 4 points. Sorts by y‐coordinate to split into two “top” and two “bottom” points,
* then sorts each pair by x‐coordinate.
*
* @param pts   Input vector of 4 points (unordered).
* @return      Ordered vector of 4 points: [TL, TR, BL, BR]. If size ≠ 4, returns empty.
*/
static std::vector<cv::Point> sortCorners(const std::vector<cv::Point>& pts) {
  if (pts.size() != 4) {
      return {};  // invalid input
  }
  // Copy and sort by y ascending
  std::vector<cv::Point> sorted = pts;
  std::sort(sorted.begin(), sorted.end(),
            [](const cv::Point& a, const cv::Point& b) {
                return a.y < b.y;
            });
  // Top two are sorted[0], sorted[1]; bottom two are sorted[2], sorted[3]
  std::vector<cv::Point> top{ sorted[0], sorted[1] };
  std::vector<cv::Point> bot{ sorted[2], sorted[3] };
  // Sort top by x ascending → top-left, top-right
  if (top[0].x > top[1].x) std::swap(top[0], top[1]);
  // Sort bottom by x ascending → bottom-left, bottom-right
  if (bot[0].x > bot[1].x) std::swap(bot[0], bot[1]);
  // Return in TL, TR, BL, BR order
  return { top[0], top[1], bot[0], bot[1] };
}

/**
* @brief Computes a size (width × height) for the “scanned” document based on the sorted corners.
*
* Measures the maximum horizontal and vertical distances between corresponding corner pairs:
*  - height = max(distance(TL, BL), distance(TR, BR))
*  - width  = max(distance(TL, TR), distance(BL, BR))
*
* @param sortedCorners  Vector of 4 points ordered [TL, TR, BL, BR].
* @return               cv::Size(width, height) for output warp. If input size ≠ 4, returns zero size.
*/
static cv::Size computeDocumentSize(const std::vector<cv::Point>& sortedCorners) {
  if (sortedCorners.size() != 4) {
      std::cerr << "ERROR: computeDocumentSize requires exactly 4 ordered corners.\n";
      return {};
  }
  auto dist = [](const cv::Point& p1, const cv::Point& p2) {
      double dx = static_cast<double>(p2.x - p1.x);
      double dy = static_cast<double>(p2.y - p1.y);
      return std::sqrt(dx*dx + dy*dy);
  };
  // Top‐left to bottom‐left, and top‐right to bottom‐right
  double h1 = dist(sortedCorners[0], sortedCorners[2]);
  double h2 = dist(sortedCorners[1], sortedCorners[3]);
  double maxH = std::max(h1, h2);
  // Top‐left to top‐right, and bottom‐left to bottom‐right
  double w1 = dist(sortedCorners[0], sortedCorners[1]);
  double w2 = dist(sortedCorners[2], sortedCorners[3]);
  double maxW = std::max(w1, w2);
  return cv::Size(static_cast<int>(std::round(maxW)),
                  static_cast<int>(std::round(maxH)));
}

/**
* @brief Computes a 3×3 homography mapping four document corners onto a fronto‐parallel rectangle.
*
* @param sortedCorners  Four points in [TL, TR, BL, BR] order.
* @param docSize        Desired output size (width, height).
* @return               3×3 homography matrix (CV_64F). Empty Mat if input invalid.
*/
static cv::Mat computeHomography(const std::vector<cv::Point>& sortedCorners,
                               const cv::Size& docSize) {
  if (sortedCorners.size() != 4 || docSize.width == 0 || docSize.height == 0) {
      return {};
  }
  // Source points (float)
  std::vector<cv::Point2f> srcPts(4);
  for (int i = 0; i < 4; ++i) {
      srcPts[i] = { static_cast<float>(sortedCorners[i].x),
                    static_cast<float>(sortedCorners[i].y) };
  }
  // Destination points: (0,0), (W,0), (0,H), (W,H)
  std::vector<cv::Point2f> dstPts = {
      { 0.0f,             0.0f            },  // TL
      { static_cast<float>(docSize.width), 0.0f },  // TR
      { 0.0f,             static_cast<float>(docSize.height) },  // BL
      { static_cast<float>(docSize.width), static_cast<float>(docSize.height) }  // BR
  };
  // Use getPerspectiveTransform (4‐point correspondences)
  return cv::getPerspectiveTransform(srcPts, dstPts);
}

//--------------------------------------------------------------------------------------
// Main program
//--------------------------------------------------------------------------------------

int main() {
  // Iterate over each sample document image
  for (const auto& relPath : kInputRelativePaths) {
      std::filesystem::path fullPath = kDataDir / relPath;
      cv::Mat colorImg = loadImageOrExit(fullPath, cv::IMREAD_COLOR);

      // Step 1: Convert BGR → HSV and extract V channel (intensity)
      cv::Mat hsv, vChannel;
      cv::cvtColor(colorImg, hsv, cv::COLOR_BGR2HSV);
      cv::extractChannel(hsv, vChannel, 2);  // V is channel index 2

      // Step 2: Compute clean edge mask using Canny + morphology
      cv::Mat edgeMask = computeEdgeMask(vChannel);
      //showImage("Clean Edge Mask", edgeMask);

      // Step 3: Find the largest quadrilateral contour (document boundary)
      auto quad = findLargestQuad(edgeMask);
      if (quad.size() != 4) {
          std::cerr << "WARNING: No quadrilateral detected in " << fullPath << "\n";
          continue;
      }

      // Step 4: Sort corner points to [TL, TR, BL, BR]
      auto sortedCorners = sortCorners(quad);

      // Step 5: Compute output document size
      cv::Size docSize = computeDocumentSize(sortedCorners);
      if (docSize.width == 0 || docSize.height == 0) {
          std::cerr << "WARNING: Invalid document size for " << fullPath << "\n";
          continue;
      }

      // Step 6: Compute homography matrix to warp to a fronto‐parallel rectangle
      cv::Mat H = computeHomography(sortedCorners, docSize);
      if (H.empty()) {
          std::cerr << "ERROR: Homography computation failed for " << fullPath << "\n";
          continue;
      }

      // Step 7: Warp the original color image
      cv::Mat warped;
      cv::warpPerspective(colorImg, warped, H, docSize);

      // Step 8: Show side‐by‐side result (original | scanned)
      cv::Mat resizedWarped;
      cv::resize(warped, resizedWarped, colorImg.size(), 0, 0, cv::INTER_LINEAR);

      cv::Mat sideBySide;
      cv::hconcat(colorImg, resizedWarped, sideBySide);

      showImage("Original vs. Scanned", sideBySide);

      // (Optional) Save the scanned output to disk
      std::filesystem::path outputPath = fullPath.parent_path() / ("scanned_" + fullPath.filename().string());
      saveImageOrExit(outputPath, warped);
  }

  return EXIT_SUCCESS;
}