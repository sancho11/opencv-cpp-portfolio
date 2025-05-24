project(sunglasses_collage)

# 1. Executable
add_executable(sunglasses_collage
  src/sunglasses_collage.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/sunglasses_collage/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/sunglasses_collage/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(sunglasses_collage PRIVATE include)
target_link_libraries(sunglasses_collage PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(sunglasses_collage PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/sunglasses_collage/data\"
)