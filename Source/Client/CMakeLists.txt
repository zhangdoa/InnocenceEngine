configure_file ("${CMAKE_SOURCE_DIR}/Source/Client/ClientMetadata.h.in" "${CMAKE_SOURCE_DIR}/Source/Client/ClientMetadata.h" @ONLY)

add_subdirectory("RenderingClient")
add_subdirectory("LogicClient")