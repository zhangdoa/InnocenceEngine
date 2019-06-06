file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoFileSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoFileSystem PROPERTY POSITION_INDEPENDENT_CODE ON)
