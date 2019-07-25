if (INNO_PLATFORM_WIN)
add_subdirectory("DX11")
add_subdirectory("DX12")
endif (INNO_PLATFORM_WIN)

if (VULKAN)
add_subdirectory("VK")
endif (VULKAN)

if (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)
add_subdirectory("GL")
endif (OPENGL_FOUND AND NOT INNO_PLATFORM_MAC)

if (INNO_PLATFORM_MAC)
add_subdirectory("MT")
endif (INNO_PLATFORM_MAC)