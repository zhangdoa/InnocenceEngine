cmake_minimum_required (VERSION 3.1) 
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/lib)

project (InnocenceEngine)

include_directories("engine")
include_directories("external/include")

add_subdirectory("engine")
add_subdirectory("game")
