file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoComponentManager ${HEADERS} ${SOURCES})
set_property(TARGET InnoComponentManager PROPERTY POSITION_INDEPENDENT_CODE ON)