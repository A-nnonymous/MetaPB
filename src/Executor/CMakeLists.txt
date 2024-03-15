file(GLOB SOURCES "./*.cpp")

add_library(executorLib ${SOURCES})

target_link_libraries(executorLib utilsLib)

