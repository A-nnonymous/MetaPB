add_subdirectory(dpu)

file(GLOB SOURCES "./*.cpp")

add_library(operatorLib ${SOURCES})

target_link_libraries(operatorLib PUBLIC OpenMP::OpenMP_CXX)
