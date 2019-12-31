file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoAssetManger ${HEADERS} ${SOURCES})
set_property(TARGET InnoAssetManger PROPERTY POSITION_INDEPENDENT_CODE ON)

find_library(ASSIMP assimp)
target_link_libraries(InnoAssetManger "${ASSIMP}")

target_link_libraries(InnoAssetManger STBWrapper)