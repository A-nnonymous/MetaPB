file(GLOB SOURCES "./*.cpp")

add_library(metaSchedulerLib ${SOURCES})

target_link_libraries(metaSchedulerLib schedulerLib)

