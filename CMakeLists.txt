cmake_minimum_required(VERSION 3.27)

# The "C" language in this project is UPMEM-C in fact.
project(MetaPB LANGUAGES CXX C)

#------------------ Runtime environmanet asertions ---------
find_package(xgboost REQUIRED)
find_package(OpenMP REQUIRED)
#------------------- CPU Compiler settings-------------------
set(CMAKE_CXX_COMPILER /usr/bin/g++-12 )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -mavx512f -march=native  -I/usr/include/dpu -ldpu -Wno-pointer-arith -Wno-narrowing -Wno-unused-result ")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -I/usr/include/dpu -ldpu -Wno-pointer-arith -Wno-narrowing -Wno-unused-result ")

execute_process(
  COMMAND dpu-pkg-config --libs --cflags dpu
  OUTPUT_VARIABLE CXX_DPU_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
#------------------- DPU Compiler settings-------------------
set(CMAKE_C_COMPILER "dpu-upmem-dpurte-clang" )  
set(CMAKE_C_FLAGS " -O2 -DNR_TASKLETS=12")  

#------------------- Path variables -------------------------
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/cpu_bin)
set(DPU_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/dpu_bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/include/dpu)

add_subdirectory(src)
add_subdirectory(benchmarks)
add_subdirectory(tests)




