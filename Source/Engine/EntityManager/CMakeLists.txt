file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoEntityManager ${HEADERS} ${SOURCES})
set_property(TARGET InnoEntityManager PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(InnoEntityManager PROPERTIES FOLDER Engine)