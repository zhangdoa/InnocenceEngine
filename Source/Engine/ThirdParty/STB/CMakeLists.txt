aux_source_directory(. DIR_SRCS)
add_library(STB ${DIR_SRCS})
set_property(TARGET STB PROPERTY POSITION_INDEPENDENT_CODE ON)
