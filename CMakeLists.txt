cmake_minimum_required(VERSION 3.10)
project(opencv_cpp_portfolio LANGUAGES CXX)

# 1. C++17 and common directory for executables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 2. Look for OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 3. Sub-projects List
set(PROJECT_LIST
  sunglasses_collage
  qr_decoder
  panorama_stitching
  blemish_removal
  sketch_and_cartoon
  chroma_key
  interactive_scaler
  feature_alignment
  autofocus_evaluator
  roi_selector
)

# 4. Add every directory in projects/
foreach(proj IN LISTS PROJECT_LIST)
  add_subdirectory(projects/${proj})
endforeach()