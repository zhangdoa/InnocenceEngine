file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoWinWindowSystem ${HEADERS} ${SOURCES})

if (INNO_RENDERER_DIRECTX)
add_subdirectory("DXWindowSurface")
target_link_libraries(InnoWinWindowSystem InnoWinDXWindowSurface)
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_OPENGL)
add_subdirectory("GLWindowSurface")
target_link_libraries(InnoWinWindowSystem InnoWinGLWindowSurface)
endif (INNO_RENDERER_OPENGL)

if (INNO_RENDERER_VULKAN)
add_subdirectory("VKWindowSurface")
target_link_libraries(InnoWinWindowSystem InnoWinVKWindowSurface)
endif (INNO_RENDERER_VULKAN)