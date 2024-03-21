include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/benchmarks)

add_executable(benchmarks ./benchmarks.cpp)
target_link_libraries(benchmarks dpu executorLib operatorLib schedulerLib utilsLib OpenMP::OpenMP_CXX)