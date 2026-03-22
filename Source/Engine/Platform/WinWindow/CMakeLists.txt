file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(WinWindowService ${HEADERS} ${SOURCES})
set_target_properties(WinWindowService PROPERTIES FOLDER Engine/Window)

if (INNO_RENDERER_DIRECTX)
add_subdirectory("DXWindowSurface")
target_link_libraries(WinWindowService WinDXWindowSurface)
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
add_subdirectory("VKWindowSurface")
target_link_libraries(WinWindowService WinVKWindowSurface)
endif (INNO_RENDERER_VULKAN)