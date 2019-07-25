file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDX12RenderingServer ${HEADERS} ${SOURCES})