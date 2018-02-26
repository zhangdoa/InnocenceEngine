aux_source_directory(. DIR_SRCS)
add_library(InnoGraphicManager ${DIR_SRCS})

find_library(ASSIMP assimp-vc140-mt)
find_library(GLFW glfw3)

target_link_libraries(InnoGraphicManager "${ASSIMP}")
target_link_libraries(InnoGraphicManager "${GLFW}")

target_link_libraries(InnoGraphicManager GLAD)
target_link_libraries(InnoGraphicManager STB_Image)