#add_subdirectory("GLFWWrapper")
add_subdirectory("ImGuiWrapper")
add_subdirectory("Core")

if (INNO_PLATFORM_WIN)
add_subdirectory("DX11RenderingBackend")
add_subdirectory("DX12RenderingBackend")
add_subdirectory("WinWindow")
add_subdirectory("PhysXWrapper")
endif (INNO_PLATFORM_WIN)

if (VULKAN)
add_subdirectory("VKRenderingBackend")
endif (VULKAN)

if (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)
add_subdirectory("GLRenderingBackend")
endif (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)

aux_source_directory(. DIR_SRCS)

add_library(InnoSystem SHARED ${DIR_SRCS})

include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoSystem
    BASE_NAME INNO_SYSTEM
    EXPORT_MACRO_NAME INNO_SYSTEM_EXPORT
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/engine/exports/InnoSystem_Export.h
    STATIC_DEFINE INNO_SYSTEM_BUILT_AS_STATIC
)

target_link_libraries(InnoSystem InnoCore)

if (INNO_PLATFORM_WIN)
target_link_libraries(InnoSystem InnoDX11RenderingBackend)
target_link_libraries(InnoSystem InnoDX12RenderingBackend)
target_link_libraries(InnoSystem WinWindow)
target_link_libraries(InnoSystem PhysXWrapper)
endif (INNO_PLATFORM_WIN)

if (VULKAN)
target_link_libraries(InnoSystem InnoVKRenderingBackend)
endif (VULKAN)

if (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)
target_link_libraries(InnoSystem InnoGLRenderingBackend)
endif (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)

find_library(ASSIMP assimp)
target_link_libraries(InnoSystem "${ASSIMP}")

target_link_libraries(InnoSystem STB_Image)

target_link_libraries(InnoSystem ImGuiWrapper)

if (INNO_PLATFORM_LINUX)
target_link_libraries(InnoSystem -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)

if (INNO_PLATFORM_MAC)
find_library(COCOA Cocoa)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoSystem "${COCOA}" "${IOKIT}" "${COREVIDEO}" -lz -lc++fs)
endif (INNO_PLATFORM_MAC)
