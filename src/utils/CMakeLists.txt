file(GLOB SOURCES "./*.cpp")

add_library(utilsLib ${SOURCES})

target_link_libraries(utilsLib PUBLIC pcm xgboost::xgboost dpu)

