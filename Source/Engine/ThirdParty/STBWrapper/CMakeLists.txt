aux_source_directory(. DIR_SRCS)
add_library(STBWrapper ${DIR_SRCS})
set_property(TARGET STBWrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
