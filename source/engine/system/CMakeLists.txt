aux_source_directory(. DIR_SRCS)
add_library(InnoSystem ${DIR_SRCS})

find_library(ASSIMP assimp)
find_library(GLFW glfw3)

target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem "${GLFW}" -lglfw3 -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem STB_Image)
