file(GLOB SOURCES "./*.cpp")

add_library(schedulerLib ${SOURCES})

target_link_libraries(schedulerLib utilsLib)
target_link_libraries(schedulerLib executorLib)
target_link_libraries(schedulerLib operatorLib)

