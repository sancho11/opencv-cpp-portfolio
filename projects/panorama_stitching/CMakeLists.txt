project(panorama_stitching)

# 1. Executable
add_executable(panorama_stitching
  src/panorama_stitching.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/panorama_stitching/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/panorama_stitching/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(panorama_stitching PRIVATE include)
target_link_libraries(panorama_stitching PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(panorama_stitching PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/panorama_stitching/data\"
)