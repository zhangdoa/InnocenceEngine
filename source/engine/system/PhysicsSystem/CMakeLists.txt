aux_source_directory(. DIR_SRCS)
add_library(InnoPhysicsSystem SHARED ${DIR_SRCS})

target_link_libraries(InnoPhysicsSystem InnoAssetSystem)
target_link_libraries(InnoPhysicsSystem InnoGameSystem)
