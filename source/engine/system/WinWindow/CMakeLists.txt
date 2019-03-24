add_subdirectory("WinDXWindow")
add_subdirectory("WinGLWindow")

aux_source_directory(. DIR_SRCS)
add_library(WinWindow ${DIR_SRCS})

target_link_libraries(WinWindow WinDXWindow)
target_link_libraries(WinWindow WinGLWindow)