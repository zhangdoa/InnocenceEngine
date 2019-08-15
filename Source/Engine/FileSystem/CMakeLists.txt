file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoFileSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoFileSystem PROPERTY POSITION_INDEPENDENT_CODE ON)

find_library(ASSIMP assimp)
target_link_libraries(InnoFileSystem "${ASSIMP}")

target_link_libraries(InnoFileSystem STB)