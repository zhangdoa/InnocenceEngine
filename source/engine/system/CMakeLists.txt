if (INNO_RENDERER_OPENGL)
add_subdirectory("GLRenderer")
endif (INNO_RENDERER_OPENGL)

if (INNO_RENDERER_DX)
add_subdirectory("DXRenderer")
endif (INNO_RENDERER_DX)

aux_source_directory(. DIR_SRCS)
add_library(InnoSystem ${DIR_SRCS})

find_library(ASSIMP assimp)
target_link_libraries(InnoSystem "${ASSIMP}")

find_library(GLFW glfw3)
if (INNO_PLATFORM_LINUX64)
target_link_libraries(InnoSystem "${GLFW}" -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
elseif (INNO_PLATFORM_MACOS)
find_library(IRRXML IrrXML)
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoSystem "${IRRXML}" "${GLFW}" "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}" -lz)
else ()
target_link_libraries(InnoSystem "${GLFW}")
endif (INNO_PLATFORM_LINUX64)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem STB_Image)

link_directories(${CMAKE_SOURCE_DIR}/game/)

if (INNO_RENDERER_OPENGL)
target_link_libraries(InnoSystem InnoGLRenderer)
target_link_libraries(InnoSystem ImGui)
endif (INNO_RENDERER_OPENGL)

if (INNO_RENDERER_DX)
target_link_libraries(InnoSystem InnoDXRenderer)
endif (INNO_RENDERER_DX)

if (BUILD_EDITOR)
target_link_libraries(InnoSystem InnoEditor)
endif (BUILD_EDITOR)

if(BUILD_GAME)
target_link_libraries(InnoSystem InnoGame)
endif (BUILD_GAME)

if(BUILD_TEST)
target_link_libraries(InnoSystem InnoTest)
endif (BUILD_TEST)
