cmake_minimum_required(VERSION 3.10.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project (InnocenceEngine)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib_archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# compiler config file
configure_file ("${CMAKE_SOURCE_DIR}/engine/common/config.h.in" "${CMAKE_SOURCE_DIR}/engine/common/config.h" @ONLY)

include_directories("external/include" "external/gitsubmodules/PhysX/physx/include" "external/gitsubmodules/PhysX/pxshared/include")

option (USE_ROW_MAJOR_MEMORY_LAYOUT "use Row-Major memory layout" ON)
option (USE_COLUMN_MAJOR_MEMORY_LAYOUT "use Column-Major memory layout" OFF)

option (INNO_PLATFORM_WIN "Windows x86-64 64-bit" OFF)
option (INNO_PLATFORM_LINUX "Linux x86-64 64-bit" OFF)
option (INNO_PLATFORM_MAC "MAC x86-64 64-bit" OFF)

if (INNO_PLATFORM_WIN)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/lib/win)
find_library(VULKAN vulkan-1)
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_LINUX)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/lib/linux)
find_library(VULKAN vulkan)
endif (INNO_PLATFORM_LINUX)

if (INNO_PLATFORM_MAC)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/lib/mac)
find_library(VULKAN vulkan-1)
endif (INNO_PLATFORM_MAC)

if (VULKAN)
option (INNO_RENDERER_VULKAN "Vulkan renderer" ON)
else ()
message("No Vulkan pre-compiled library found! Won't compile Vulkan rendering backend.")
option (INNO_RENDERER_VULKAN "Vulkan renderer" OFF)
endif (VULKAN)

add_subdirectory("engine")
add_subdirectory("game")
