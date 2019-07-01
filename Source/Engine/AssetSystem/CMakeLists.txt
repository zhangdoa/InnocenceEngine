file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoAssetSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoAssetSystem PROPERTY POSITION_INDEPENDENT_CODE ON)

