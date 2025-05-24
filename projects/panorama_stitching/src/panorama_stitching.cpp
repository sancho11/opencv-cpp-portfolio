/**
 * @file panorama_stitching.cpp
 * @brief Loads a series of images from a data directory, stitches them into a panorama,
 *        and saves the result to disk.
 *
 * This program:
 *  1. Scans a specified directory for image files with a given extension.
 *  2. Reads and validates each image.
 *  3. Uses OpenCV's Stitcher to stitch the images into a single panorama.
 *  4. Writes the stitched panorama to the output file.
 *  5. Reports any errors encountered during processing.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "config.pch"

static const std::string kDataDir = DATA_DIR;       ///< Base data directory (defined in config.pch)
static const std::string kInputSubdir = "scene";    ///< Subdirectory under data directory to scan
static const std::string kFileExtension = ".jpeg";  ///< Target image file extension
static const std::string kOutputFilename = "panorama.jpg";  ///< Output panorama filename

/**
 * @brief Loads an image from disk, exiting on failure.
 * @param path Full path to the image file.
 * @param flags Imread flags (default: IMREAD_UNCHANGED).
 * @return Loaded cv::Mat image.
 */
cv::Mat loadImageOrExit(const std::filesystem::path& path, int flags = cv::IMREAD_UNCHANGED) {
  cv::Mat image = cv::imread(path.string(), flags);
  if (image.empty()) {
    std::cerr << "ERROR: Failed to load image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return image;
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
 * @brief Retrieves all regular files with a specified extension in a directory.
 * @param directory Path to the directory to scan.
 * @param extension File extension to filter by (including the dot), e.g. ".jpg".
 * @param outFiles Vector to receive the absolute paths of matching files.
 * @throws std::runtime_error If the directory does not exist or is not a directory.
 */
void collectImageFiles(const std::filesystem::path& directory, const std::string& extension,
                       std::vector<std::filesystem::path>& outFiles) {
  outFiles.clear();

  if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
    throw std::runtime_error("Invalid directory: " + directory.string());
  }

  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file()) {
      continue;  // Skip non-files
    }
    if (entry.path().extension() == extension) {
      outFiles.emplace_back(std::filesystem::absolute(entry.path()));
    }
  }
}

int main() {
  // Build full input directory path
  std::filesystem::path inputDir = std::filesystem::path{kDataDir} / kInputSubdir;

  std::vector<std::filesystem::path> imageFiles;
  try {
    collectImageFiles(inputDir, kFileExtension, imageFiles);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (imageFiles.empty()) {
    std::cerr << "No images found in " << inputDir << " with extension " << kFileExtension
              << std::endl;
    return EXIT_FAILURE;
  }

  // Sort files to ensure consistent ordering
  std::sort(imageFiles.begin(), imageFiles.end());

  // Load valid images into a vector
  std::vector<cv::Mat> images;
  images.reserve(imageFiles.size());
  for (const auto& filePath : imageFiles) {
    cv::Mat img = cv::imread(filePath.string());
    if (img.empty()) {
      std::cerr << "WARNING: Skipping invalid image: " << filePath << std::endl;
      continue;
    }
    images.push_back(std::move(img));
  }

  if (images.size() < 2) {
    std::cerr << "ERROR: Need at least two valid images to stitch a panorama." << std::endl;
    return EXIT_FAILURE;
  }

  // Create panorama stitcher (PANORAMA mode for perspective images)
  cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);

  // Perform stitching
  cv::Mat panorama;
  cv::Stitcher::Status status = stitcher->stitch(images, panorama);

  if (status != cv::Stitcher::OK) {
    std::cerr << "ERROR: Panorama stitching failed (code " << static_cast<int>(status) << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Save the resulting panorama
  std::filesystem::path outputPath = std::filesystem::path{kDataDir} / kOutputFilename;
  saveImageOrExit(outputPath, panorama);

  std::cout << "Panorama successfully saved to: " << outputPath << std::endl;
  return EXIT_SUCCESS;
}