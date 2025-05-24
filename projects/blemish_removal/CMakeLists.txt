project(blemish_removal)

# 1. Executable
add_executable(blemish_removal
  src/blemish_removal.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/blemish_removal/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/blemish_removal/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(blemish_removal PRIVATE include)
target_link_libraries(blemish_removal PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(blemish_removal PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/blemish_removal/data\"
)