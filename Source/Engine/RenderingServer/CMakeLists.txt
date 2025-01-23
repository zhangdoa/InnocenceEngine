add_subdirectory("Common")

if (INNO_RENDERER_DIRECTX)
add_subdirectory("DX12")
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
add_subdirectory("VK")
endif (INNO_RENDERER_VULKAN)

if (INNO_RENDERER_METAL)
add_subdirectory("MT")
endif (INNO_RENDERER_METAL)