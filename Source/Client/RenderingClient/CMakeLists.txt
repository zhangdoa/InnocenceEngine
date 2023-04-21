file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultRenderingClient SHARED ${HEADERS} ${SOURCES})
set_target_properties(DefaultRenderingClient PROPERTIES FOLDER Client)
target_link_libraries(DefaultRenderingClient Engine)
target_link_libraries(DefaultRenderingClient DefaultGPUBuffers)