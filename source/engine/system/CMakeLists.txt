aux_source_directory(. DIR_SRCS)
add_library(InnoSystem ${DIR_SRCS})

find_library(ASSIMP assimp-vc140-mt HINTS "${CMAKE_SOURCE_DIR}/external/lib")
find_library(GLFW glfw3 HINTS "${CMAKE_SOURCE_DIR}/external/lib")

target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem "${GLFW}")

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem STB_Image)