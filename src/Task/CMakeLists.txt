file(GLOB SOURCES "./*.cpp")

add_library(taskLib ${SOURCES})

target_link_libraries(taskLib operatorLib)

