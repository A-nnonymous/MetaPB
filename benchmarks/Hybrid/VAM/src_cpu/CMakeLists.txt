# Host side DPU library argument CXX_DPU_FLAGS generating
add_compile_options( -Wall -Wextra -g -O3  -DNR_TASKLETS=12 -DNR_DPUS=2048 -DBL=10 -DINT32 -DENERGY=0)
add_executable(va ./vaHost.cpp)
target_link_libraries(va utilsLib dpu)
