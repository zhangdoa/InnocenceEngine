aux_source_directory(. DIR_SRCS)
add_library(InnoGameSystem ${DIR_SRCS})
set_property(TARGET InnoGameSystem PROPERTY POSITION_INDEPENDENT_CODE ON)

