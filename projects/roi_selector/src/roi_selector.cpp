/**
 * @file roi_selector.cpp
 * @brief Interactive ROI selector: draw a rectangle on an image to save a cropped region.
 *
 * Usage:
 *   ./roi_selector [input_image] [output_image]
 *
 * Controls:
 *   - Drag left mouse button to draw ROI.
 *   - Press Enter to exit (saves ROI).
 *   - Press ESC to exit without saving.
 */

#include <filesystem>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "config.pch"  // Defines DATA_DIR macro

static cv::Scalar kRectColor(255, 255, 0);
static constexpr int kRectThickness = 2;
static const std::filesystem::path kDefaultInput =
    std::filesystem::path(DATA_DIR) / "../data/demo.png";
static const std::filesystem::path kDefaultOutput =
    std::filesystem::path(DATA_DIR) / "../data/face.png";

/**
 * @brief Loads an image or exits on failure.
 * @param path  Filesystem path to load
 * @param flags imread flags (default: unchanged)
 * @return Loaded cv::Mat
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
 * @brief Saves an image or exits on failure.
 * @param path   Destination path
 * @param image  cv::Mat to write
 * @param params Optional imwrite parameters
 */
void saveImageOrExit(const std::filesystem::path& path, const cv::Mat& image,
                     const std::vector<int>& params = {}) {
  if (!cv::imwrite(path.string(), image, params)) {
    std::cerr << "ERROR: Failed to save image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "Saved ROI to: " << path << std::endl;
}

/**
 * @brief Mouse callback state container.
 */
struct MouseState {
  cv::Mat image;                      ///< Original image
  cv::Mat display;                    ///< Current display image (with rectangle)
  cv::Point start_pt;                 ///< Mouse-down point
  cv::Point end_pt;                   ///< Mouse-up point
  bool drawing{false};                ///< True while dragging
  std::filesystem::path output_path;  ///< File to save cropped ROI
};

/**
 * @brief Mouse callback: handles drawing and ROI extraction.
 */
void onMouse(int event, int x, int y, int /*flags*/, void* userdata) {
  auto state = reinterpret_cast<MouseState*>(userdata);
  switch (event) {
    case cv::EVENT_LBUTTONDOWN:
      state->drawing = true;
      state->start_pt = {x, y};
      state->display = state->image.clone();
      break;

    case cv::EVENT_MOUSEMOVE:
      if (state->drawing) {
        state->display = state->image.clone();
        cv::rectangle(state->display, state->start_pt, {x, y}, kRectColor, kRectThickness);
      }
      break;

    case cv::EVENT_LBUTTONUP:
      state->drawing = false;
      state->end_pt = {x, y};

      // Normalize coordinates
      {
        int x1 = std::min(state->start_pt.x, state->end_pt.x);
        int x2 = std::max(state->start_pt.x, state->end_pt.x);
        int y1 = std::min(state->start_pt.y, state->end_pt.y);
        int y2 = std::max(state->start_pt.y, state->end_pt.y);
        state->start_pt = {x1, y1};
        state->end_pt = {x2, y2};
      }

      // Draw final rectangle
      state->display = state->image.clone();
      cv::rectangle(state->display, state->start_pt, state->end_pt, kRectColor, kRectThickness);

      // Crop and save ROI
      {
        cv::Rect roi(state->start_pt.x, state->start_pt.y, state->end_pt.x - state->start_pt.x,
                     state->end_pt.y - state->start_pt.y);
        // Clamp ROI to image bounds
        roi &= cv::Rect(0, 0, state->image.cols, state->image.rows);

        if (roi.width > 0 && roi.height > 0) {
          cv::Mat cropped = state->image(roi).clone();
          saveImageOrExit(state->output_path, cropped);
        } else {
          std::cerr << "INFO: Selected ROI has zero area; not saving." << std::endl;
        }
      }
      break;

    default:
      break;
  }
}

int main(int argc, char* argv[]) {
  // Parse input/output paths
  std::filesystem::path input_path = (argc > 1 ? std::filesystem::path(argv[1]) : kDefaultInput);
  std::filesystem::path output_path = (argc > 2 ? std::filesystem::path(argv[2]) : kDefaultOutput);

  // Load image
  MouseState state;
  state.image = loadImageOrExit(input_path, cv::IMREAD_COLOR);
  state.display = state.image.clone();
  state.output_path = output_path;

  const std::string window_name = "ROI Selector";
  cv::namedWindow(window_name, cv::WINDOW_NORMAL);

  // Register mouse callback
  cv::setMouseCallback(window_name, onMouse, &state);

  // Instruction overlay
  const std::string msg1 = "Drag to select ROI";
  const std::string msg2 = "ESC: exit w/o save   ENTER: exit";
  cv::putText(state.display, msg1, {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);
  cv::putText(state.display, msg2, {10, 60}, cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);

  // Main loop
  int key = 0;
  while (key != 27 && key != 13) {  // ESC or ENTER
    cv::imshow(window_name, state.display);
    key = cv::waitKey(20) & 0xFF;
  }

  cv::destroyWindow(window_name);
  return 0;
}