add_subdirectory("WinDXWindow")
add_subdirectory("WinGLWindow")
if (VULKAN)
add_subdirectory("WinVKWindow")
endif (VULKAN)

aux_source_directory(. DIR_SRCS)
add_library(WinWindow ${DIR_SRCS})

target_link_libraries(WinWindow WinDXWindow)
target_link_libraries(WinWindow WinGLWindow)
if (VULKAN)
target_link_libraries(WinWindow WinVKWindow)
endif (VULKAN)