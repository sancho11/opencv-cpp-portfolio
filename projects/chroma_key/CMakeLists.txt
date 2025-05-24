project(chroma_key)

# 1. Executable
add_executable(chroma_key
  src/chroma_key.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/chroma_key/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/chroma_key/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(chroma_key PRIVATE include)
target_link_libraries(chroma_key PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(chroma_key PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/chroma_key/data\"
)