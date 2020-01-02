file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(InnoDefaultRenderingClient SHARED ${HEADERS} ${SOURCES})
target_link_libraries(InnoDefaultRenderingClient InnoEngine)
target_link_libraries(InnoDefaultRenderingClient InnoDefaultGPUBuffers)