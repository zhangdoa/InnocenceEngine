file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoRayTracer ${HEADERS} ${SOURCES})
set_property(TARGET InnoRayTracer PROPERTY POSITION_INDEPENDENT_CODE ON)
