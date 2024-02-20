# Host side DPU library argument CXX_DPU_FLAGS generating
add_compile_options( -Wall -Wextra -g -O3  -DNR_TASKLETS=12 -DNR_DPUS=2048 -DBL=10 -DFLOAT -DENERGY=0 -lpthread)
add_executable(grindAll ./vaHost.cpp)
target_link_libraries(grindAll utilsLib dpu)
