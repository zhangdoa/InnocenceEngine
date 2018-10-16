aux_source_directory(. DIR_SRCS)
add_library(InnoVisionSystem SHARED ${DIR_SRCS})

target_link_libraries(InnoVisionSystem InnoGameSystem)
target_link_libraries(InnoVisionSystem InnoDXRenderer)
target_link_libraries(InnoVisionSystem InnoGLRenderer)