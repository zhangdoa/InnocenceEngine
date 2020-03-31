aux_source_directory(. DIR_SRCS)
add_library(MacWindow ${DIR_SRCS})
set_property(TARGET MacWindow PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(MacWindow PROPERTIES FOLDER Engine/Window)