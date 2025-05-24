project(qr_decoder)

# 1. Executable
add_executable(qr_decoder
  src/qr_decoder.cpp
)

# 2 Precompiled PATHDIR
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/qr_decoder/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/qr_decoder/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(qr_decoder PRIVATE include)
target_link_libraries(qr_decoder PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(qr_decoder PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/qr_decoder/data\"
)