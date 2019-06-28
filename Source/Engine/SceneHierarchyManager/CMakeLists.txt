file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoSceneHierarchyManager ${HEADERS} ${SOURCES})
set_property(TARGET InnoSceneHierarchyManager PROPERTY POSITION_INDEPENDENT_CODE ON)