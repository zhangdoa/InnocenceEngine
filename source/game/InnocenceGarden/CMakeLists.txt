aux_source_directory(. DIR_SRCS)
add_library(InnoGame ${DIR_SRCS})

target_link_libraries(InnoGame InnoLowLevelSystem)
