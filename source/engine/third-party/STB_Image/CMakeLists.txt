aux_source_directory(. DIR_SRCS)
add_library(STB_Image ${DIR_SRCS})
set_property(TARGET STB_Image PROPERTY POSITION_INDEPENDENT_CODE ON)
