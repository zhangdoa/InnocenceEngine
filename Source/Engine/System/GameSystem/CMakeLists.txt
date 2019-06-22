file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoGameSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoGameSystem PROPERTY POSITION_INDEPENDENT_CODE ON)

