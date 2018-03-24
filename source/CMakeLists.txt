cmake_minimum_required(VERSION 3.5)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compiler config file
configure_file ("${CMAKE_SOURCE_DIR}/engine/common/config.h.in" "${CMAKE_SOURCE_DIR}/engine/common/config.h")

# Use Clang?
option (USE_CLANG "Use Clang as the default compiler" OFF)
if (USE_CLANG)
set (CMAKE_C_COMPILER               "clang-cl")
set (CMAKE_C_FLAGS                  "-Wall")
set (CMAKE_C_FLAGS_DEBUG            "/Debug")
set (CMAKE_C_FLAGS_MINSIZEREL       "-Os -DNDEBUG")
set (CMAKE_C_FLAGS_RELEASE          "-O4 -DNDEBUG")
set (CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 /Debug")
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_COMPILER             "clang-cl")
set (CMAKE_CXX_FLAGS                "-Wall -Xclang -std=gnu++14")
set (CMAKE_CXX_FLAGS_DEBUG          "/Debug")
set (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 /Debug")
endif (USE_CLANG)

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
