project(document_scanner)

# 1. Executable
add_executable(document_scanner
  src/document_scanner.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/document_scanner/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/document_scanner/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(document_scanner PRIVATE include)
target_link_libraries(document_scanner PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(document_scanner PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/document_scanner/data\"
)