if (INNO_PLATFORM_WIN)
add_subdirectory("DX11RenderingBackend")
add_subdirectory("DX12RenderingBackend")
endif (INNO_PLATFORM_WIN)

if (VULKAN)
add_subdirectory("VKRenderingBackend")
endif (VULKAN)

if (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)
add_subdirectory("GLRenderingBackend")
endif (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)

if (INNO_PLATFORM_MAC)
add_subdirectory("MTRenderingBackend")
endif (INNO_PLATFORM_MAC)