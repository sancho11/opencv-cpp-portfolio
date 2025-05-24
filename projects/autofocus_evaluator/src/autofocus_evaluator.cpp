/**
 * @file autofocus_evaluator.cpp
 * @brief Evaluates frame sharpness (focus) in a video using multiple metrics.
 *
 * This tool:
 *  1. Opens a video file or default DATA_DIR/../data/focus-test.mp4.
 *  2. Computes four focus measures on each frame or ROI:
 *     - Variance of Laplacian (absolute)
 *     - Sum of Modified Laplacian
 *     - Local variance
 *     - Variance of gradient magnitude
 *  3. Tracks timing per-frame and identifies the frame with maximum focus for each metric.
 *  4. Reports the best frame IDs and average computation times.
 *  5. Displays a concatenated image of the best frames side-by-side.
 *
 * Usage:
 *   ./autofocus_evaluator [video_file] [top left X] [top left Y] [width] [height]
 *
 * If video_file is not provided, defaults to DATA_DIR/../data/focus-test.mp4.
 * If ROI parameters are omitted, uses the full frame.
 */

#include <chrono>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "config.pch"  // Defines DATA_DIR macro

namespace fs = std::filesystem;

static const fs::path kDefaultVideo = fs::path(DATA_DIR) / "../data/focus-test.mp4";

/**
 * @brief Displays an image in a resizable window.
 * @param image Image to display
 * @param windowName Optional window name (default "Image")
 */
void displayImage(const cv::Mat& image, const std::string& windowName = "Image") {
  cv::namedWindow(windowName, cv::WINDOW_NORMAL);
  cv::resizeWindow(windowName, cv::Size(1200, 900));
  cv::imshow(windowName, image);
  cv::waitKey(0);
  cv::destroyWindow(windowName);
}

/**
 * @brief Compute variance of the Laplacian on V channel.
 * @param image BGR image
 * @return Sum of squared Laplacian responses
 */
double varAbsLaplacian(const cv::Mat& image) {
  cv::Mat hsv, v;
  cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
  std::vector<cv::Mat> channels;
  cv::split(hsv, channels);
  v = channels[2];

  cv::Mat lap;
  cv::Laplacian(v, lap, CV_32F, 3, 1.0 / (255.0 * 3 * 2));
  cv::Mat sq;
  cv::pow(lap, 2.0, sq);
  return cv::sum(sq)[0];
}

/**
 * @brief Compute local variance on V channel using sliding window.
 * @param image BGR image
 * @param ksize Window size (default 3x3)
 * @return Variance of local variances
 */
double varLocal(const cv::Mat& image, int ksize = 3) {
  cv::Mat L_result;

  int wy = 3;
  int wx = 3;
  cv::Mat mean_kernel = cv::Mat::ones(cv::Size(wx, wy), CV_32F);

  cv::Mat HSV;
  std::vector<cv::Mat> HSV_Ch;
  cv::cvtColor(image, HSV, cv::COLOR_BGR2HSV);
  split(HSV, HSV_Ch);

  cv::Mat Vch_normalized;
  HSV_Ch[2].convertTo(Vch_normalized, CV_32F, 1.0 / 255.0);

  cv::Mat mean_filtered_image;
  cv::filter2D(Vch_normalized, mean_filtered_image, CV_32F, mean_kernel, cv::Point(-1, -1), 0.0,
               cv::BORDER_DEFAULT);
  mean_filtered_image = mean_filtered_image / (wy * wx);

  cv::Mat Vch_normalized_borderExpanded;
  cv::copyMakeBorder(Vch_normalized, Vch_normalized_borderExpanded, 0, wy, 0, wx,
                     cv::BORDER_DEFAULT);
  cv::Mat lvk(Vch_normalized.size(), CV_32F);
  for (int m = 0; m < image.cols; m++) {
    for (int n = 0; n < image.rows; n++) {
      float lvk_mn = 0;
      float fbark_mn = mean_filtered_image.at<float>(cv::Point(m, n));
      for (int i = 0; i < wx; i++) {
        for (int j = 0; j < wy; j++) {
          float fk_minj = Vch_normalized_borderExpanded.at<float>(cv::Point(m + i, n + j));
          float fk_fbark_diff = fk_minj - fbark_mn;
          lvk_mn += cv::pow(fk_fbark_diff, 2.0);
        }
      }
      lvk.at<float>(cv::Point(m, n)) = lvk_mn;
    }
  }
  lvk = lvk / (wx * wy);
  float mean_lvk = cv::sum(lvk)[0] / (lvk.cols * lvk.rows);
  cv::Mat lvk_mean_lvk_diff = lvk - mean_lvk;
  cv::Mat lvk_mean_lvk_diff_pow2;
  cv::pow(lvk_mean_lvk_diff, 2.0, lvk_mean_lvk_diff_pow2);
  float fmVk = cv::sum(lvk_mean_lvk_diff_pow2)[0] / (lvk.cols * lvk.rows);
  return fmVk;
}

/**
 * @brief Compute variance of gradient magnitude on V channel.
 * @param image BGR image
 * @return Sum of squared gradient magnitudes
 */
double varGradMagnitude(const cv::Mat& image) {
  cv::Mat hsv;
  cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
  cv::Mat v;
  cv::extractChannel(hsv, v, 2);
  v.convertTo(v, CV_32F, 1.0 / 255.0);

  cv::Mat gx, gy;
  cv::Sobel(v, gx, CV_32F, 1, 0, 3);
  cv::Sobel(v, gy, CV_32F, 0, 1, 3);

  cv::Mat mag2 = gx.mul(gx) + gy.mul(gy);
  return cv::sum(mag2)[0];
}

/**
 * @brief Compute sum of modified Laplacian (Lovlac) on V channel.
 * @param image BGR image
 * @return Sum of absolute directional Laplacians
 */
double sumModifiedLaplacian(const cv::Mat& image) {
  cv::Mat L_x_result;
  cv::Mat L_y_result;

  int kSize = 3;
  cv::Mat HSV;
  std::vector<cv::Mat> HSV_Ch;
  cv::cvtColor(image, HSV, cv::COLOR_BGR2HSV);
  split(HSV, HSV_Ch);

  cv::Mat kernelx = cv::Mat::zeros(cv::Size(kSize, 1), CV_32F);
  kernelx.at<float>(cv::Point(0, 0)) = -1.0;
  kernelx.at<float>(cv::Point(1, 0)) = 2.0;
  kernelx.at<float>(cv::Point(2, 0)) = -1.0;

  cv::Mat kernely = cv::Mat::zeros(cv::Size(1, kSize), CV_32F);
  kernely.at<float>(cv::Point(0, 0)) = -1.0;
  kernely.at<float>(cv::Point(0, 1)) = 2.0;
  kernely.at<float>(cv::Point(0, 2)) = -1.0;

  cv::Mat Vch_normalized;
  HSV_Ch[2].convertTo(Vch_normalized, CV_32F, 1.0 / 255.0);

  cv::filter2D(Vch_normalized, L_x_result, CV_32F, kernelx, cv::Point(-1, -1), 0.0,
               cv::BORDER_DEFAULT);
  cv::filter2D(Vch_normalized, L_y_result, CV_32F, kernely, cv::Point(-1, -1), 0.0,
               cv::BORDER_DEFAULT);

  cv::Mat L_x_result_abs, L_y_result_abs;
  L_x_result_abs = cv::abs(L_x_result);
  L_y_result_abs = cv::abs(L_y_result);
  double sum_abs_Lx, sum_abs_Ly;
  sum_abs_Lx = cv::sum(L_x_result_abs)[0];
  sum_abs_Ly = cv::sum(L_y_result_abs)[0];
  double var = (sum_abs_Lx + sum_abs_Ly);

  return var;
}

int main(int argc, char* argv[]) {
  // Open video
  fs::path videoPath = (argc > 1 ? argv[1] : kDefaultVideo);
  cv::VideoCapture cap(videoPath.string());
  if (!cap.isOpened()) {
    std::cerr << "ERROR: Cannot open video: " << videoPath << std::endl;
    return EXIT_FAILURE;
  }

  // Optional ROI args
  int x = 0, y = 0, w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH),
      h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  if (argc >= 6) {
    x = std::stoi(argv[2]);
    y = std::stoi(argv[3]);
    w = std::stoi(argv[4]);
    h = std::stoi(argv[5]);
  }
  cv::Rect roi(x, y, w, h);

  int totalFrames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
  std::cout << "Total frames: " << totalFrames << std::endl;

  // Timing accumulators
  double t1 = 0, t2 = 0, t3 = 0, t4 = 0;
  // Best values & frames
  double best1 = 0, best2 = 0, best3 = 0, best4 = 0;
  // Frame #
  int fbest1 = 0, fbest2 = 0, fbest3 = 0, fbest4 = 0;

  cv::Mat f1, f2, f3, f4;
  int idx = 0;

  cv::Mat frame;
  while (cap.read(frame)) {
    cv::Mat crop = frame(roi);
    auto start = std::chrono::high_resolution_clock::now();
    double v1 = varAbsLaplacian(crop);
    t1 += (std::chrono::high_resolution_clock::now() - start).count() * 1e-6;
    start = std::chrono::high_resolution_clock::now();
    double v2 = sumModifiedLaplacian(crop);
    t2 += (std::chrono::high_resolution_clock::now() - start).count() * 1e-6;
    start = std::chrono::high_resolution_clock::now();
    double v3 = varLocal(crop);
    t3 += (std::chrono::high_resolution_clock::now() - start).count() * 1e-6;
    start = std::chrono::high_resolution_clock::now();
    double v4 = varGradMagnitude(crop);
    t4 += (std::chrono::high_resolution_clock::now() - start).count() * 1e-6;

    if (v1 > best1) {
      best1 = v1;
      f1 = frame.clone();
      fbest1 = idx;
    }
    if (v2 > best2) {
      best2 = v2;
      f2 = frame.clone();
      fbest2 = idx;
    }
    if (v3 > best3) {
      best3 = v3;
      f3 = frame.clone();
      fbest3 = idx;
    }
    if (v4 > best4) {
      best4 = v4;
      f4 = frame.clone();
      fbest4 = idx;
    }
    idx++;
  }

  // Report
  std::cout << "========================================" << std::endl;
  std::cout << "Best frames (1..4): " << best1 << ", " << best2 << ", " << best3 << ", " << best4
            << std::endl;
  std::cout << "FrameNumber (1..4): " << best1 << ", " << best2 << ", " << best3 << ", " << best4
            << std::endl;
  std::cout << "Avg time per frame (ms):\n"
            << " varAbsLaplacian:     " << (t1 / idx) << "\n"
            << " sumModifiedLaplacian:" << (t2 / idx) << "\n"
            << " varLocal:             " << (t3 / idx) << "\n"
            << " varGradMagnitude:    " << (t4 / idx) << std::endl;

  // Concatenate best frames 2x2
  cv::Mat top, bottom, all;
  cv::hconcat(f1, f2, top);
  cv::hconcat(f3, f4, bottom);
  cv::vconcat(top, bottom, all);
  displayImage(all, "Best Focus Frames");

  return EXIT_SUCCESS;
}
