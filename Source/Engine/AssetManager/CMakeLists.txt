file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoAssetManger ${HEADERS} ${SOURCES})
set_property(TARGET InnoAssetManger PROPERTY POSITION_INDEPENDENT_CODE ON)

if (APPLE)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
find_library(ASSIMP assimpd)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
else ()
find_library(ASSIMP assimp)
endif (APPLE)

target_link_libraries(InnoAssetManger "${ASSIMP}")

target_link_libraries(InnoAssetManger STBWrapper)
