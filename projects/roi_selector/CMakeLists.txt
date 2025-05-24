project(roi_selector)

# 1. Executable
add_executable(roi_selector
  src/roi_selector.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/roi_selector/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/roi_selector/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(roi_selector PRIVATE include)
target_link_libraries(roi_selector PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(roi_selector PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/roi_selector/data\"
)