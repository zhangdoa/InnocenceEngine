aux_source_directory(. DIR_SRCS)
add_library(ImGui ${DIR_SRCS})
set_property(TARGET ImGui PROPERTY POSITION_INDEPENDENT_CODE ON)
