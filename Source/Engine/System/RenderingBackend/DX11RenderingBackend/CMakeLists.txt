file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDX11RenderingBackend ${HEADERS} ${SOURCES})