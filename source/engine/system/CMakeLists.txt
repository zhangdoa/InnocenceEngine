if (INNO_PLATFORM_WIN)
add_subdirectory("DXRenderingBackend")
endif (INNO_PLATFORM_WIN)

add_subdirectory("GLRenderingBackend")
add_subdirectory("VKRenderingBackend")

aux_source_directory(. DIR_SRCS)

add_library(InnoSystem SHARED ${DIR_SRCS})
include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoSystem
    BASE_NAME INNO_SYSTEM
    EXPORT_MACRO_NAME INNO_SYSTEM_EXPORT
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/engine/exports/InnoSystem_Export.h
    STATIC_DEFINE INNO_SYSTEM_BUILT_AS_STATIC
)

if (INNO_PLATFORM_WIN)
target_link_libraries(InnoSystem InnoDXRenderingBackend)
endif (INNO_PLATFORM_WIN)

target_link_libraries(InnoSystem InnoGLRenderingBackend)

if (VULKAN)
target_link_libraries(InnoSystem InnoVKRenderingBackend)
endif (VULKAN)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem ImGui)

find_library(ASSIMP assimp)
target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem STB_Image)

if (INNO_PLATFORM_LINUX)
find_library(GLFW glfw)
elseif (INNO_PLATFORM_MAC)
find_library(GLFW glfw)
else ()
find_library(GLFW glfw3dll)
endif (INNO_PLATFORM_LINUX)
target_link_libraries(InnoSystem "${GLFW}")

if (INNO_PLATFORM_LINUX)
target_link_libraries(InnoSystem -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread -lstdc++fs)
elseif (INNO_PLATFORM_MAC)
#find_library(IRRXML IrrXML)
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoSystem "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}" -lz -lc++fs)
else ()
endif (INNO_PLATFORM_LINUX)
