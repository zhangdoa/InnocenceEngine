file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(InnoDefaultRenderingClient SHARED ${HEADERS} ${SOURCES})
set_target_properties(InnoDefaultRenderingClient PROPERTIES FOLDER Client)
target_link_libraries(InnoDefaultRenderingClient InnoEngine)
target_link_libraries(InnoDefaultRenderingClient InnoDefaultGPUBuffers)