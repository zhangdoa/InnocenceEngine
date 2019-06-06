aux_source_directory(. DIR_SRCS)
add_library(GLAD ${DIR_SRCS})
set_property(TARGET GLAD PROPERTY POSITION_INDEPENDENT_CODE ON)
