aux_source_directory(./ DIR_SRCS)

if (INNO_PLATFORM_LINUX)
find_library(VULKAN vulkan)
else ()
find_library(VULKAN vulkan-1)
endif (INNO_PLATFORM_LINUX)

if (VULKAN)
add_library(InnoVKRenderingBackend ${DIR_SRCS})
target_link_libraries(InnoVKRenderingBackend "${VULKAN}")
else ()
message("No Vulkan pre-compiled library found! Won't compile Vulkan rendering backend.")
endif (VULKAN)