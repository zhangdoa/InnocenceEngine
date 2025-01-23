file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(WinWindowSystem ${HEADERS} ${SOURCES})
set_target_properties(WinWindowSystem PROPERTIES FOLDER Engine/Window)

if (INNO_RENDERER_DIRECTX)
add_subdirectory("DXWindowSurface")
target_link_libraries(WinWindowSystem WinDXWindowSurface)
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
add_subdirectory("VKWindowSurface")
target_link_libraries(WinWindowSystem WinVKWindowSurface)
endif (INNO_RENDERER_VULKAN)