#TODO: 
# 1. Add c++ compile flag and debug option toggler

cmake_minimum_required(VERSION 3.22.1)

project(MetaPB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH /output/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

#add_subdirectory(${CMAKE_SOURCE_DIR}/ext/stdexec)
include_directories(${CMAKE_SOURCE_DIR}/submodules/stdexec/include) # Adding external library stdexec
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(tests)




