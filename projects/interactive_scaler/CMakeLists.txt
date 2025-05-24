project(interactive_scaler)

# 1. Executable
add_executable(interactive_scaler
  src/interactive_scaler.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/interactive_scaler/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/interactive_scaler/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(interactive_scaler PRIVATE include)
target_link_libraries(interactive_scaler PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(interactive_scaler PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/interactive_scaler/data\"
)