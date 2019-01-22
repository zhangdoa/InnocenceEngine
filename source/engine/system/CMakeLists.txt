aux_source_directory(. DIR_SRCS)
if (INNO_PLATFORM_LINUX OR INNO_PLATFORM_MAC)
list(REMOVE_ITEM DIR_SRCS ./DXGuiSystem.h ./DXGuiSystem.cpp ./DXRenderingSystem.h ./DXRenderingSystem.cpp ./DXRenderingSystemUtilities.h ./DXRenderingSystemUtilities.cpp ./DXWindowSystem.h ./DXWindowSystem.cpp ./DXHeaders.h)
endif (INNO_PLATFORM_LINUX OR INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
find_library(VULKAN vulkan)
else ()
find_library(VULKAN vulkan-1)
endif (INNO_PLATFORM_LINUX)

if(NOT VULKAN)
message("No Vulkan pre-compiled library found! Won't compile Vulkan rendering backend.")
list(REMOVE_ITEM DIR_SRCS ./VKGuiSystem.h ./VKGuiSystem.cpp ./VKRenderingSystem.h ./VKRenderingSystem.cpp ./VKWindowSystem.h ./VKWindowSystem.cpp)
endif ()

add_library(InnoSystem SHARED ${DIR_SRCS})
include (GenerateExportHeader)
GENERATE_EXPORT_HEADER (InnoSystem
    BASE_NAME INNO_SYSTEM
    EXPORT_MACRO_NAME INNO_SYSTEM_EXPORT
    EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/engine/exports/InnoSystem_Export.h
    STATIC_DEFINE INNO_SYSTEM_BUILT_AS_STATIC
)

if (VULKAN)
target_link_libraries(InnoSystem "${VULKAN}")
endif (VULKAN)

target_link_libraries(InnoSystem GLAD)
target_link_libraries(InnoSystem ImGui)

find_library(ASSIMP assimp)
target_link_libraries(InnoSystem "${ASSIMP}")
target_link_libraries(InnoSystem STB_Image)

if (INNO_PLATFORM_LINUX)
find_library(GLFW glfw)
elseif (INNO_PLATFORM_MAC)
find_library(GLFW glfw)
else ()
find_library(GLFW glfw3dll)
endif (INNO_PLATFORM_LINUX)
target_link_libraries(InnoSystem "${GLFW}")

if (INNO_PLATFORM_LINUX)
target_link_libraries(InnoSystem -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lGL -lm -lpthread)
elseif (INNO_PLATFORM_MAC)
#find_library(IRRXML IrrXML)
find_library(COCOA Cocoa)
find_library(OPENGL OpenGL)
find_library(IOKIT IOKit)
find_library(COREVIDEO CoreVideo)
target_link_libraries(InnoSystem "${COCOA}" "${OPENGL}" "${IOKIT}" "${COREVIDEO}" -lz -lc++fs)
else ()
endif (INNO_PLATFORM_LINUX)
