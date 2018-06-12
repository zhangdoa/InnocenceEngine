aux_source_directory(. DIR_SRCS)
add_library(InnoSystem ${DIR_SRCS})

find_library(ASSIMP assimp)
find_library(GLFW glfw3)
find_library(ImGui imgui)

target_link_libraries(InnoSystem "${ASSIMP}")
if (INNO_PLATFORM_LINUX64)
target_link_libraries(InnoSystem "${GLFW}" -lglfw3 -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
else ()
target_link_libraries(InnoSystem "${GLFW}")
endif (INNO_PLATFORM_LINUX64)

target_link_libraries(InnoSystem ImGui)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem STB_Image)
