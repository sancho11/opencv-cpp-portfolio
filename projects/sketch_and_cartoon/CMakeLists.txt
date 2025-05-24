project(sketch_and_cartoon)

# 1. Executable
add_executable(sketch_and_cartoon
  src/sketch_and_cartoon.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/sketch_and_cartoon/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/sketch_and_cartoon/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(sketch_and_cartoon PRIVATE include)
target_link_libraries(sketch_and_cartoon PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(sketch_and_cartoon PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/sketch_and_cartoon/data\"
)