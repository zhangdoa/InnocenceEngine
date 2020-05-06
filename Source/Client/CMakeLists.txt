configure_file ("${CMAKE_SOURCE_DIR}/Client/ClientMetadata.h.in" "${CMAKE_SOURCE_DIR}/Client/ClientMetadata.h" @ONLY)

add_subdirectory("DefaultGPUBuffers")
add_subdirectory("RenderingClient")
add_subdirectory("LogicClient")
add_subdirectory("Tool")
