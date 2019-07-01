file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoEngine SHARED ${HEADERS} ${SOURCES})

include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoEngine
    BASE_NAME INNO_ENGINE
    EXPORT_MACRO_NAME INNO_ENGINE_API
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/Engine/Export/InnoEngine_Export.h
    STATIC_DEFINE INNO_ENGINE_BUILT_AS_STATIC
)

target_link_libraries(InnoEngine InnoCore)
target_link_libraries(InnoEngine InnoFileSystem)
target_link_libraries(InnoEngine InnoEntityManager)
target_link_libraries(InnoEngine InnoComponentManager)
target_link_libraries(InnoEngine InnoSceneHierarchyManager)
target_link_libraries(InnoEngine InnoGameSystem)
target_link_libraries(InnoEngine InnoPhysicsSystem)
target_link_libraries(InnoEngine InnoRenderingFrontend)
target_link_libraries(InnoEngine InnoRayTracer)

if (INNO_PLATFORM_WIN)
target_link_libraries(InnoEngine InnoDX11RenderingBackend)
target_link_libraries(InnoEngine InnoDX12RenderingBackend)
target_link_libraries(InnoEngine WinWindow)
endif (INNO_PLATFORM_WIN)

if (VULKAN)
target_link_libraries(InnoEngine InnoVKRenderingBackend)
endif (VULKAN)

if (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)
target_link_libraries(InnoEngine InnoGLRenderingBackend)
endif (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)

if (INNO_PLATFORM_MAC)
target_link_libraries(InnoEngine InnoMTRenderingBackend)
target_link_libraries(InnoEngine MacWindow)
endif (INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
target_link_libraries(InnoEngine LinuxWindow)
endif (INNO_PLATFORM_LINUX)

find_library(ASSIMP assimp)
target_link_libraries(InnoEngine "${ASSIMP}")

target_link_libraries(InnoEngine STB_Image)

target_link_libraries(InnoEngine ImGuiWrapper)

if (INNO_PLATFORM_LINUX)
target_link_libraries(InnoEngine -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)

if (INNO_PLATFORM_MAC)
find_library(COCOA Cocoa)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoEngine "${COCOA}" "${IOKIT}" "${COREVIDEO}" -lz -lc++fs)
endif (INNO_PLATFORM_MAC)
