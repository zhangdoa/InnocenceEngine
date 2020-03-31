file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoWinDXWindowSurface ${HEADERS} ${SOURCES})
set_target_properties(InnoWinDXWindowSurface PROPERTIES FOLDER Engine/Window)