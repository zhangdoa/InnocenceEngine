aux_source_directory(. DIR_SRCS)
add_library(AssimpWrapper ${DIR_SRCS})
set_property(TARGET AssimpWrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(AssimpWrapper PROPERTIES FOLDER ThirdParty)

if (APPLE)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
find_library(ASSIMP assimpd)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
else ()
find_library(ASSIMP assimp)
endif (APPLE)

target_link_libraries(AssimpWrapper "${ASSIMP}")