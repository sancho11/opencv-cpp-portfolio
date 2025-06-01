project(sunglasses++)

# 1. Executable
add_executable(sunglasses++
  src/sunglasses++.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/sunglasses++/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/sunglasses++/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(sunglasses_collage PRIVATE include)
target_link_libraries(sunglasses++ PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(sunglasses++ PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/sunglasses++/data\"
)