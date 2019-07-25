file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoVKRenderingServer ${HEADERS} ${SOURCES})