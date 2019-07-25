cmake_minimum_required(VERSION 3.10.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project (InnocenceEngine)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/LibArchive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../Bin)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# compiler config file
configure_file ("${CMAKE_SOURCE_DIR}/Engine/Common/Config.h.in" "${CMAKE_SOURCE_DIR}/Engine/Common/Config.h" @ONLY)

include_directories("External/Include" "External/GitSubmodules/PhysX/physx/include" "External/GitSubmodules/PhysX/pxshared/include")

option (USE_ROW_MAJOR_MEMORY_LAYOUT "Use Row-Major memory layout" ON)
option (USE_COLUMN_MAJOR_MEMORY_LAYOUT "Use Column-Major memory layout" OFF)

option (INNO_PLATFORM_WIN "Windows x86-64" OFF)
option (INNO_PLATFORM_LINUX "Linux x86-64" OFF)
option (INNO_PLATFORM_MAC "Mac OS X x86-64" OFF)

if (INNO_PLATFORM_WIN)
set(PLATFORM_NAME Win)
option (INNO_RENDERER_DIRECTX "DirectX renderer" ON)
else ()
option (INNO_RENDERER_DIRECTX "DirectX renderer" OFF)
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_LINUX)
set(PLATFORM_NAME Linux)
endif (INNO_PLATFORM_LINUX)

if (INNO_PLATFORM_MAC)
set(PLATFORM_NAME Mac)
endif (INNO_PLATFORM_MAC)

set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/Lib/${PLATFORM_NAME}/${CMAKE_BUILD_TYPE})

find_library(VULKAN vulkan)
if (VULKAN)
option (INNO_RENDERER_VULKAN "Vulkan renderer" ON)
else ()
message("No Vulkan pre-compiled library found! Won't compile Vulkan rendering backend.")
option (INNO_RENDERER_VULKAN "Vulkan renderer" OFF)
endif (VULKAN)

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
option (INNO_RENDERER_OPENGL "OpenGL renderer" ON)
else (OPENGL_FOUND)
message("No OpenGL library found! Won't compile OpenGL rendering backend.")
option (INNO_RENDERER_OPENGL "OpenGL renderer" OFF)
endif()

add_subdirectory("Test")
add_subdirectory("Engine")
add_subdirectory("Client")
