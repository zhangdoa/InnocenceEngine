# Define directories
set(DXC_DIR ${CMAKE_BINARY_DIR}/Tools/dxc)
set(DXC_ZIP ${CMAKE_BINARY_DIR}/dxc_latest.zip)

# GitHub API URL for latest DXC release
set(DXC_RELEASE_API "https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/latest")

# Step 1: Fetch latest release metadata
set(DXC_JSON ${CMAKE_BINARY_DIR}/dxc_release.json)
if (NOT EXISTS ${DXC_JSON})
    message(STATUS "Fetching latest DXC release metadata...")
    file(DOWNLOAD ${DXC_RELEASE_API} ${DXC_JSON} SHOW_PROGRESS)
endif()

# Step 2: Parse JSON to find the download URL for the DXC ZIP
# Read the JSON content
file(READ ${CMAKE_BINARY_DIR}/dxc_release.json DXC_JSON_CONTENT)

# Extract the assets array
string(JSON DXC_ASSETS GET ${DXC_JSON_CONTENT} "assets")

# Iterate over the array to find the matching asset
set(DXC_DOWNLOAD_URL "")
math(EXPR NUM_ASSETS "0")
string(JSON NUM_ASSETS LENGTH ${DXC_ASSETS})
foreach(INDEX RANGE ${NUM_ASSETS})
    string(JSON ASSET_NAME GET ${DXC_ASSETS} ${INDEX} "name")
    string(JSON ASSET_URL GET ${DXC_ASSETS} ${INDEX} "browser_download_url")
    if (ASSET_NAME MATCHES "^dxc_.*\\.zip$")
        set(DXC_DOWNLOAD_URL ${ASSET_URL})
        break()
    endif()
endforeach()

# Verify the download URL was found
if (NOT DEFINED DXC_DOWNLOAD_URL OR DXC_DOWNLOAD_URL STREQUAL "")
    message(FATAL_ERROR "Could not find DXC ZIP download URL in release metadata.")
endif()

message(STATUS "Latest DXC download URL: ${DXC_DOWNLOAD_URL}")


# Step 3: Download the DXC ZIP file
if (NOT EXISTS ${DXC_ZIP})
    message(STATUS "Downloading latest DXC binaries...")
    file(DOWNLOAD ${DXC_DOWNLOAD_URL} ${DXC_ZIP} SHOW_PROGRESS)
endif()

# Step 4: Extract the DXC ZIP file
if (NOT EXISTS ${DXC_DIR})
    message(STATUS "Extracting DXC binaries with PowerShell...")
    execute_process(
        COMMAND powershell -Command "Expand-Archive -Path '${DXC_ZIP}' -DestinationPath '${DXC_DIR}' -Force"
    )
endif()