add_subdirectory("Common")
add_subdirectory("ThirdParty")
add_subdirectory("Services")
add_subdirectory("RenderingServer")
add_subdirectory("RayTracer")

file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(Engine ${HEADERS} ${SOURCES})
set_target_properties(Engine PROPERTIES FOLDER Engine)

target_link_libraries(Engine Common)
target_link_libraries(Engine Services)
target_link_libraries(Engine RayTracer)
target_link_libraries(Engine RenderingServerCommon)

if (INNO_RENDERER_DIRECTX)
target_link_libraries(Engine DX12RenderingServer)
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
target_link_libraries(Engine VKRenderingServer)
endif (INNO_RENDERER_VULKAN)

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

add_subdirectory("Platform")