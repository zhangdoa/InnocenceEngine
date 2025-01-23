# Set up CMake options
set(INNO_LOGIC_CLIENT DefaultLogicClient)
set(INNO_RENDERING_CLIENT DefaultRenderingClient)

option(INNO_DEBUG OFF)
option(INNO_RELEASE OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(INNO_DEBUG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
set(INNO_DEBUG ON)
else()
set(INNO_RELEASE ON)
endif()

# Set platform and renderer
option(INNO_PLATFORM_WIN OFF)
option(INNO_PLATFORM_LINUX OFF)
option(INNO_PLATFORM_MAC OFF)

option(INNO_RENDERER_DIRECTX OFF)
option(INNO_RENDERER_VULKAN OFF)
option(INNO_RENDERER_METAL OFF)

if (WIN32)
set(INNO_PLATFORM_WIN ON)
set(INNO_RENDERER_DIRECTX ON)
elseif (APPLE)
set(INNO_PLATFORM_MAC ON)
set(INNO_RENDERER_METAL ON)
elseif (UNIX)
set(INNO_PLATFORM_LINUX ON)
endif ()

find_package(Vulkan)

if (VULKAN_FOUND)
set(INNO_RENDERER_VULKAN ON)
endif (VULKAN_FOUND)

option(USE_ROW_MAJOR_MEMORY_LAYOUT OFF)
option(USE_COLUMN_MAJOR_MEMORY_LAYOUT OFF)
set(USE_ROW_MAJOR_MEMORY_LAYOUT ON)
set(USE_COLUMN_MAJOR_MEMORY_LAYOUT OFF)

option(INNO_BUILD_TOOLS OFF)

configure_file ("${CMAKE_SOURCE_DIR}/Source/Engine/Common/Config.h.in" "${CMAKE_SOURCE_DIR}/Source/Engine/Common/Config.h" @ONLY)

# Add subdirectories
set(INNO_BUILD_TOOLS OFF)
add_subdirectory("Engine")
add_subdirectory("Test")
add_subdirectory("Client")
if(INNO_BUILD_TOOLS)
add_subdirectory("Tool")
endif (INNO_BUILD_TOOLS)

target_link_libraries(Engine ${INNO_RENDERING_CLIENT})
target_link_libraries(Engine ${INNO_LOGIC_CLIENT})