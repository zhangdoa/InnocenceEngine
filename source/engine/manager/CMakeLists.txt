add_subdirectory("graphic")
aux_source_directory(. DIR_SRCS)
add_library(InnoCoreManager ${DIR_SRCS})