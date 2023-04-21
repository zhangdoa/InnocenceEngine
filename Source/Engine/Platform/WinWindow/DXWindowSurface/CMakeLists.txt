file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(WinDXWindowSurface ${HEADERS} ${SOURCES})
set_target_properties(WinDXWindowSurface PROPERTIES FOLDER Engine/Window)