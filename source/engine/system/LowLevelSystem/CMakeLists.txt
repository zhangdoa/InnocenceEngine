aux_source_directory(. DIR_SRCS)
add_library(InnoLowLevelSystem SHARED ${DIR_SRCS})
include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoLowLevelSystem
    BASE_NAME InnoLowLevelSystem
    EXPORT_MACRO_NAME InnoLowLevelSystem_EXPORT
    EXPORT_FILE_NAME LowLevelSystem_Export.h
    STATIC_DEFINE InnoLowLevelSystem_BUILT_AS_STATIC
)