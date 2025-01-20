# Update Git submodules
message(STATUS "Updating Git submodules...")
execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE result
)
if(result)
    message(FATAL_ERROR "Failed to update Git submodules.")
endif()

# Create necessary directories
message(STATUS "Creating necessary directories...")
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Build)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Res/ConvertedAssets)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Res/Intermediate)

# Copy ImGui files
message(STATUS "Copying ImGui files...")
set(IMGUI_SRC ${INNO_GITSUBMODULE_DIRECTORIES}/imgui)
set(IMGUI_DST ${CMAKE_SOURCE_DIR}/Source/Engine/ThirdParty/ImGui)

# Collect ImGui files
file(GLOB IMGUI_FILES
    ${IMGUI_SRC}/*.h
    ${IMGUI_SRC}/*.cpp
)

file(COPY ${IMGUI_FILES} DESTINATION ${IMGUI_DST})
if (WIN32)
message(STATUS "Windows detected. Copying Windows-specific ImGui files...")
file(COPY
    ${IMGUI_SRC}/backends/imgui_impl_win32.h
    ${IMGUI_SRC}/backends/imgui_impl_win32.cpp
    ${IMGUI_SRC}/backends/imgui_impl_dx11.h
    ${IMGUI_SRC}/backends/imgui_impl_dx11.cpp
    ${IMGUI_SRC}/backends/imgui_impl_dx12.h
    ${IMGUI_SRC}/backends/imgui_impl_dx12.cpp
    DESTINATION ${IMGUI_DST}
)
file(COPY
    ${IMGUI_SRC}/backends/imgui_impl_opengl3.h
    ${IMGUI_SRC}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_SRC}/backends/imgui_impl_opengl3_loader.h 
    DESTINATION ${IMGUI_DST}
)
elseif (APPLE)
    message(STATUS "Support for ImGUI is not ready on macOS yet")
elseif (UNIX)
message(STATUS "Linux detected. Applying Linux-specific ImGui fix...")

# Define the ImGui file paths
set(IMGUI_HEADER ${IMGUI_SRC}/backends/imgui_impl_opengl3.h)
set(TEMP_HEADER ${CMAKE_BINARY_DIR}/temp.h.new)

# Add custom commands to modify the ImGui header
add_custom_command(
    OUTPUT ${IMGUI_HEADER}
    COMMAND echo "#define IMGUI_IMPL_OPENGL_LOADER_GLAD" > ${TEMP_HEADER}
    COMMAND cat ${IMGUI_HEADER} >> ${TEMP_HEADER}
    COMMAND mv -f ${TEMP_HEADER} ${IMGUI_HEADER}
    DEPENDS ${IMGUI_HEADER}
    COMMENT "Updating ImGui OpenGL loader header for Linux"
)
file(COPY
    ${IMGUI_SRC}/backends/imgui_impl_opengl3.h
    ${IMGUI_SRC}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_SRC}/backends/imgui_impl_opengl3_loader.h 
    DESTINATION ${IMGUI_DST}
)
else()
    # Fallback for unknown platforms
    message(FATAL_ERROR "Unsupported platform detected!")
endif()

# Copy QDarkStyleSheet files
message(STATUS "Copying QDarkStyleSheet files...")
file(COPY
    ${INNO_GITSUBMODULE_DIRECTORIES}/QDarkStyleSheet/qdarkstyle/dark/rc
    DESTINATION ${CMAKE_SOURCE_DIR}/Source/Editor/qdarkstyle/rc
)

file(COPY
    ${INNO_GITSUBMODULE_DIRECTORIES}/QDarkStyleSheet/qdarkstyle/dark/darkstyle.qss
    DESTINATION ${CMAKE_SOURCE_DIR}/Source/Editor/qdarkstyle/
)

# Download Khronos headers
message(STATUS "Downloading Khronos headers...")
file(MAKE_DIRECTORY ${INNO_EXTERNAL_DIRECTORIES}/Include/GL)
file(DOWNLOAD
    https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
    ${INNO_EXTERNAL_INCLUDE_DIRECTORIES}/GL/wglext.h
)

file(DOWNLOAD
    https://www.khronos.org/registry/OpenGL/api/GL/glext.h
    ${INNO_EXTERNAL_INCLUDE_DIRECTORIES}/GL/glext.h
)

# Include the module for downloading DXC binaries
if (WIN32)
    message(STATUS "Downloading DXC binaries...")
    include(cmake/DownloadDXC.cmake)
else()
    message(STATUS "Skipping DXC download: Non-Windows platform detected.")
endif()