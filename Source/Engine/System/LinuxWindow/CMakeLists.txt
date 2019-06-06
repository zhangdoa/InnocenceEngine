aux_source_directory(. DIR_SRCS)
add_library(LinuxWindow ${DIR_SRCS})
set_property(TARGET LinuxWindow PROPERTY POSITION_INDEPENDENT_CODE ON)
