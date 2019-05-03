file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoCore ${HEADERS} ${SOURCES})
set_property(TARGET InnoCore PROPERTY POSITION_INDEPENDENT_CODE ON)
