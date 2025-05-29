# Global properties
cmake_minimum_required(VERSION 3.16)
message (STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")

project (InnocenceEngine)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/LibArchive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin)

# Set source directories
set(INNO_EXTERNAL_DIRECTORIES ${CMAKE_SOURCE_DIR}/Source/External)
set(INNO_EXTERNAL_INCLUDE_DIRECTORIES ${INNO_EXTERNAL_DIRECTORIES}/Include)
set(INNO_GITSUBMODULE_DIRECTORIES ${INNO_EXTERNAL_DIRECTORIES}/GitSubmodules)

# Language standard
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Set platform-specific properties
if (WIN32)
    set(PLATFORM_FOLDER "Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif (APPLE)
    set(PLATFORM_FOLDER "MacOS")
    set(CMAKE_MACOSX_RPATH OFF)
    set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
    set(CMAKE_XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
elseif (UNIX)
    set(PLATFORM_FOLDER "Linux")
else()
    message(FATAL_ERROR "Unsupported platform! Cannot determine platform-specific folder name.")
endif()

# Set up Python virtual environment
set(PYTHON_VENV_PATH ${CMAKE_BINARY_DIR}/venv)
find_package(Python REQUIRED COMPONENTS Interpreter)
message(STATUS "Python executable: ${Python_EXECUTABLE}")

# Ensure the virtual environment exists
if (NOT EXISTS ${PYTHON_VENV_PATH})
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m venv ${PYTHON_VENV_PATH}
        RESULT_VARIABLE VENV_CREATION_RESULT
        OUTPUT_VARIABLE VENV_CREATION_OUTPUT
        ERROR_VARIABLE VENV_CREATION_ERROR
    )
    if (NOT VENV_CREATION_RESULT EQUAL 0)
        message(FATAL_ERROR 
            "Failed to create Python virtual environment at ${PYTHON_VENV_PATH}\n"
            "Error: ${VENV_CREATION_ERROR}\n"
            "Output: ${VENV_CREATION_OUTPUT}"
        )
    else()
        message(STATUS "Created Python virtual environment at ${PYTHON_VENV_PATH}")
    endif()
endif()

# Set the Python executable to the one in the virtual environment
unset(Python_EXECUTABLE)
set(Python_EXECUTABLE ${PYTHON_VENV_PATH}/Scripts/python)

option(FORCE_REBUILD_THIRD_PARTY "Force rebuild of all third-party libraries" OFF)

option(PREPARE_ENV "Prepare the environment" ON)
if (PREPARE_ENV)
    message(STATUS "Preparing the environment...")
    include(CMake/PrepareEnvironment.cmake)
endif()

option(BUILD_THIRD_PARTY "Build third-party libraries" ON)
include(CMake/BuildThirdPartyLibs.cmake)

# Add subdirectories
add_subdirectory("Source")