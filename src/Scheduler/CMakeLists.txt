file(GLOB SOURCES "./*.cpp")

add_library(schedulerLib ${SOURCES})

target_link_libraries(schedulerLib taskLib)

