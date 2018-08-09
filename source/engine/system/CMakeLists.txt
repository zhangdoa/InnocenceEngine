aux_source_directory(. DIR_SRCS)
add_library(InnoSystem ${DIR_SRCS})

target_link_libraries(InnoSystem InnoComponent)

find_library(ASSIMP assimp)
find_library(GLFW glfw3)
find_library(ImGui imgui)

if (INNO_PLATFORM_LINUX64)
target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem "${GLFW}" -lglfw3 -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
elseif (INNO_PLATFORM_MACOS)
target_link_libraries(InnoSystem "${ASSIMP}")
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoSystem "${GLFW}" "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}")
else ()
target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem "${GLFW}")
endif (INNO_PLATFORM_LINUX64)

target_link_libraries(InnoSystem ImGui)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem STB_Image)

link_directories(${CMAKE_SOURCE_DIR}/game/)

if (BUILD_EDITOR)
target_link_libraries(InnoSystem InnoEditor)
endif (BUILD_EDITOR)

if(BUILD_GAME)
target_link_libraries(InnoSystem InnoGame)
endif (BUILD_GAME)

if(BUILD_TEST)
target_link_libraries(InnoSystem InnoTest)
endif (BUILD_TEST)
