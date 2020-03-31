file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoWinVKWindowSurface ${HEADERS} ${SOURCES})
set_target_properties(InnoWinVKWindowSurface PROPERTIES FOLDER Engine/Window)