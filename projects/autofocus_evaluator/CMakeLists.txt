project(autofocus_evaluator)

# 1. Executable
add_executable(autofocus_evaluator
  src/autofocus_evaluator.cpp
)

# 2
configure_file(
  ${CMAKE_SOURCE_DIR}/projects/autofocus_evaluator/src/config.pch.in
  ${CMAKE_SOURCE_DIR}/projects/autofocus_evaluator/src/config.pch
  @ONLY
)

# 3. Headers and libs
#target_include_directories(autofocus_evaluator PRIVATE include)
target_link_libraries(autofocus_evaluator PRIVATE ${OpenCV_LIBS})

# 4. Reference needed data
target_compile_definitions(autofocus_evaluator PRIVATE
  DATA_DIR=\"${CMAKE_SOURCE_DIR}/projects/autofocus_evaluator/data\"
)