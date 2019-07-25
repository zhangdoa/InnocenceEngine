file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoGLRenderingBackend ${HEADERS} ${SOURCES})
