project(feature_alignment)

# 1. Executable
add_executable(feature_alignment
  src/feature_alignment.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/feature_alignment/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/feature_alignment/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(feature_alignment PRIVATE include)
target_link_libraries(feature_alignment PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(feature_alignment PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/feature_alignment/data\"
)