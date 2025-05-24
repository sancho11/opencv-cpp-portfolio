/**
 * @file qrcode_detection.cpp
 * @brief Reads an input image, detects and decodes any QR code present, draws a green bounding box
 * around the detected QR code, displays the annotated image in a resizable window, prints the
 * decoded text to the console, and saves the result to disk.
 *
 */

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/opencv.hpp>

#include "config.pch"
std::string datadir = std::string(DATA_DIR);

/**
 * @brief Loads an image from disk and checks for errors.
 * @param path File path
 * @param flags Imread flags (default: IMREAD_UNCHANGED)
 * @return Loaded cv::Mat
 */
cv::Mat loadImage(const std::string& path, int flags = cv::IMREAD_UNCHANGED) {
  cv::Mat img = cv::imread(datadir + "/" + path, flags);
  if (img.empty()) {
    std::cerr << "Error: Unable to load image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return img;
}

/**
 * @brief Saves an image to disk (under the same data directory) and checks for errors.
 * @param path  Relative file path (e.g. "output/result.png")
 * @param img   The cv::Mat image to write
 * @param params Optional vector of imwrite parameters (e.g. compression level)
 */
void saveImage(const std::string& path, const cv::Mat& img,
               const std::vector<int>& params = std::vector<int>{}) {
  // Assumes you have defined somewhere:
  //   const std::string datadir = "../data";
  const std::string fullPath = datadir + "/" + path;
  if (!cv::imwrite(fullPath, img, params)) {
    std::cerr << "Error: Unable to save image: " << fullPath << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

int main() {
  // #Step 1: Read Image and store it in variable img
  cv::Mat IDCard = loadImage("../data/IDCard.jpg");

  // Checking witdh and heigth
  std::cout << IDCard.size().height << " " << IDCard.size().width << std::endl;

  // #Step 2: Detect QR Code in the Image
  cv::Mat bbox, rectifiedImage;

  // Creating a QRCodeDetector Object
  cv::QRCodeDetector qrDecoder = cv::QRCodeDetector();

  // Detect QR Code in the Image
  // Output should be stored in opencvData
  ///
  std::vector<cv::Point> vertices;
  std::string opencvData = qrDecoder.detectAndDecode(IDCard, vertices);

  // Check if a QR Code has been detected
  if (opencvData.length() > 0)
    std::cout << "QR Code Detected" << std::endl;
  else
    std::cout << "QR Code NOT Detected" << std::endl;

  // #Step 3: Draw bounding box around the detected QR Code
  int n = bbox.rows;
  cv::Mat annotated_IDCard = IDCard.clone();
  cv::Point prevp = vertices[3];
  for (int i = 0; i < 4; i++) {
    cv::line(annotated_IDCard, prevp, vertices[i], cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    prevp = vertices[i];
  }
  /// Show result
  cv::namedWindow("Recognized QR", cv::WINDOW_NORMAL);
  cv::resizeWindow("Recognized QR", 1200, 600);
  cv::imshow("Recognized QR", annotated_IDCard);
  cv::waitKey(0);

  // #Step 4: Print the Decoded Text
  //  Since we have already detected and decoded the QR Code
  //  using qrDecoder.detectAndDecode, we will directly
  //  use the decoded text we obtained at that step (opencvData)

  std::cout << "QR Code Detected!" << std::endl;
  ///
  std::cout << "Decoded Data: " << opencvData << std::endl;

  // Step 5: Save and display the result image
  // Write the result image
  saveImage("../data/QRCodeAnnotated.jpg", annotated_IDCard);
}