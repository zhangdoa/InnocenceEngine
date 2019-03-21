aux_source_directory(./ DIR_SRCS)

if (VULKAN)
add_library(InnoVKRenderingBackend ${DIR_SRCS})
target_link_libraries(InnoVKRenderingBackend "${VULKAN}")
else ()
message("No Vulkan pre-compiled library found! Won't compile Vulkan rendering backend.")
endif (VULKAN)