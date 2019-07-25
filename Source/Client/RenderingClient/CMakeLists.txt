file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDefaultRenderingClient SHARED ${HEADERS} ${SOURCES})