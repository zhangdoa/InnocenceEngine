aux_source_directory(. DIR_SRCS)
add_library(InnoEditor ${DIR_SRCS})

target_link_libraries(InnoEditor InnoEntity)
target_link_libraries(InnoEditor InnoComponent)

