file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(Engine SHARED ${HEADERS} ${SOURCES})
set_target_properties(Engine PROPERTIES FOLDER Engine)

include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (Engine
    BASE_NAME INNO_ENGINE
    EXPORT_MACRO_NAME INNO_ENGINE_API
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/Source/Engine/Export/Engine_Export.h
    STATIC_DEFINE INNO_ENGINE_BUILT_AS_STATIC
)

target_link_libraries(Engine Core)
target_link_libraries(Engine EntityManager)
target_link_libraries(Engine SubSystem)
target_link_libraries(Engine RenderingFrontend)
target_link_libraries(Engine RayTracer)
target_link_libraries(Engine RenderingServerCommon)

if (INNO_RENDERER_DIRECTX)
target_link_libraries(Engine DX11RenderingServer)
target_link_libraries(Engine DX12RenderingServer)
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
target_link_libraries(Engine VKRenderingServer)
endif (INNO_RENDERER_VULKAN)

if (INNO_RENDERER_OPENGL)
target_link_libraries(Engine GLRenderingServer)
endif (INNO_RENDERER_OPENGL)

if (INNO_RENDERER_METAL)
target_link_libraries(Engine MTRenderingServer)
endif (INNO_RENDERER_METAL)

if (INNO_PLATFORM_WIN)
target_link_libraries(Engine WinWindowSystem)
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_MAC)
target_link_libraries(Engine MacWindow)
find_library(COCOA Cocoa)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(Engine "${COCOA}" "${IOKIT}" "${COREVIDEO}" -lz)
endif (INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
target_link_libraries(Engine LinuxWindow)
target_link_libraries(Engine -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
