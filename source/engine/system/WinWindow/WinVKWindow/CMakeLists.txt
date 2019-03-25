aux_source_directory(. DIR_SRCS)
if (VULKAN)
add_library(WinVKWindow ${DIR_SRCS})
endif (VULKAN)