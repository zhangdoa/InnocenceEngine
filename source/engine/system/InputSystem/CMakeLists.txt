aux_source_directory(. DIR_SRCS)
add_library(InnoInputSystem SHARED ${DIR_SRCS})

target_link_libraries(InnoInputSystem InnoGameSystem)

