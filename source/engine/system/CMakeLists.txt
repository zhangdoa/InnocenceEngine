aux_source_directory(. DIR_SRCS)
if (INNO_PLATFORM_LINUX64 OR INNO_PLATFORM_MACOS)
list(REMOVE_ITEM DIR_SRCS ./DXGuiSystem.h ./DXGuiSystem.cpp ./DXRenderingSystem.h ./DXRenderingSystem.cpp ./DXWindowSystem.h ./DXWindowSystem.cpp)
endif (INNO_PLATFORM_LINUX64 OR INNO_PLATFORM_MACOS)
add_library(InnoSystem SHARED ${DIR_SRCS})
include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoSystem
    BASE_NAME INNO_SYSTEM
    EXPORT_MACRO_NAME INNO_SYSTEM_EXPORT
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/engine/exports/InnoSystem_Export.h
    STATIC_DEFINE INNO_SYSTEM_BUILT_AS_STATIC
)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem ImGui)

find_library(ASSIMP assimp)
target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem STB_Image)

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
