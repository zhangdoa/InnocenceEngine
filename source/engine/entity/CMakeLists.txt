aux_source_directory(. DIR_SRCS)
add_library(InnoEntity ${DIR_SRCS})

option (USE_ROW_MAJOR_MEMORY_LAYOUT "use Row-Major memory layout" ON)
option (USE_COLUMN_MAJOR_MEMORY_LAYOUT "use Column-Major memory layout" OFF)
