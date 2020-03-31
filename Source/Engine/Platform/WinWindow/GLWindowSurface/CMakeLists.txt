file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoWinGLWindowSurface ${HEADERS} ${SOURCES})
set_target_properties(InnoWinGLWindowSurface PROPERTIES FOLDER Engine/Window)