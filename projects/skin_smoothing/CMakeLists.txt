project(skin_smoothing)

# 1. Executable
add_executable(skin_smoothing
  src/skin_smoothing.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/skin_smoothing/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/skin_smoothing/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(skin_smoothing PRIVATE include)
target_link_libraries(skin_smoothing PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(skin_smoothing PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/skin_smoothing/data\"
)