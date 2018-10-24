aux_source_directory(. DIR_SRCS)
add_library(InnoHighLevelSystem SHARED ${DIR_SRCS})
include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoHighLevelSystem 
    BASE_NAME InnoHighLevelSystem 
    EXPORT_MACRO_NAME InnoHighLevelSystem_EXPORT
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/engine/exports/HighLevelSystem_Export.h
    STATIC_DEFINE InnoHighLevelSystem_BUILT_AS_STATIC
)

target_link_libraries(InnoHighLevelSystem GLAD)
target_link_libraries(InnoHighLevelSystem ImGui)
target_link_libraries(InnoHighLevelSystem InnoMath)
target_link_libraries(InnoHighLevelSystem InnoLowLevelSystem)

find_library(ASSIMP assimp)
target_link_libraries(InnoHighLevelSystem "${ASSIMP}")
target_link_libraries(InnoHighLevelSystem STB_Image)

find_library(GLFW glfw3)

if (INNO_PLATFORM_LINUX64)
target_link_libraries(InnoHighLevelSystem "${GLFW}" -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
elseif (INNO_PLATFORM_MACOS)
find_library(IRRXML IrrXML)
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoHighLevelSystem "${IRRXML}" "${GLFW}" "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}" -lz)
else ()
target_link_libraries(InnoHighLevelSystem "${GLFW}")
endif (INNO_PLATFORM_LINUX64)

link_directories(${CMAKE_SOURCE_DIR}/game/)

if (BUILD_EDITOR)
target_link_libraries(InnoHighLevelSystem InnoEditor)
endif (BUILD_EDITOR)

if(BUILD_GAME)
target_link_libraries(InnoHighLevelSystem InnoGame)
endif (BUILD_GAME)

if(BUILD_TEST)
target_link_libraries(InnoHighLevelSystem InnoTest)
endif (BUILD_TEST)