file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoRenderingFrontend ${HEADERS} ${SOURCES})
set_property(TARGET InnoRenderingFrontend PROPERTY POSITION_INDEPENDENT_CODE ON)
