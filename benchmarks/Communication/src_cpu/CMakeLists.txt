# Host side DPU library argument CXX_DPU_FLAGS generating
add_executable(commuBench ./commuBench.cpp)
add_compile_options(commuBench ${CXX_DPU_FLAGS})
target_link_libraries(commuBench utilsLib dpu)