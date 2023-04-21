file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultGPUBuffers ${HEADERS} ${SOURCES})
set_target_properties(DefaultGPUBuffers PROPERTIES FOLDER Client)
set_property(TARGET DefaultGPUBuffers PROPERTY POSITION_INDEPENDENT_CODE ON)
