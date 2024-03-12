# Host side DPU library argument CXX_DPU_FLAGS generating
add_executable(bench.base ./main.cpp)
add_compile_options(bench.base ${CXX_DPU_FLAGS} -fopenmp)
target_link_libraries(bench.base utilsLib dpu)