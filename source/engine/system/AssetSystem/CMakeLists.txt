aux_source_directory(. DIR_SRCS)
add_library(InnoAssetSystem SHARED ${DIR_SRCS})

target_link_libraries(InnoAssetSystem InnoLowLevelSystem)
find_library(ASSIMP assimp)
target_link_libraries(InnoAssetSystem "${ASSIMP}")
target_link_libraries(InnoAssetSystem STB_Image)