aux_source_directory(. DIR_SRCS)
add_library(InnoGameSystem SHARED ${DIR_SRCS})

find_library(GLFW glfw3)

if (INNO_PLATFORM_LINUX64)
target_link_libraries(InnoGameSystem "${GLFW}" -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
elseif (INNO_PLATFORM_MACOS)
find_library(IRRXML IrrXML)
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoGameSystem "${IRRXML}" "${GLFW}" "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}" -lz)
else ()
target_link_libraries(InnoGameSystem "${GLFW}")
endif (INNO_PLATFORM_LINUX64)

link_directories(${CMAKE_SOURCE_DIR}/game/)

if (BUILD_EDITOR)
target_link_libraries(InnoGameSystem InnoEditor)
endif (BUILD_EDITOR)

if(BUILD_GAME)
target_link_libraries(InnoGameSystem InnoGame)
endif (BUILD_GAME)

if(BUILD_TEST)
target_link_libraries(InnoGameSystem InnoTest)
endif (BUILD_TEST)