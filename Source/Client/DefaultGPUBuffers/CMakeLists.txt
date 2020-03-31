file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(InnoDefaultGPUBuffers ${HEADERS} ${SOURCES})
set_target_properties(InnoDefaultGPUBuffers PROPERTIES FOLDER Client)
set_property(TARGET InnoDefaultGPUBuffers PROPERTY POSITION_INDEPENDENT_CODE ON)
