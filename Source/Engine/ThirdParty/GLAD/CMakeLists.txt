# Path to glad directory
set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/External/GitSubmodules/GLAD/")

# Install the dependencies
find_package(PythonInterp REQUIRED)
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -m pip install -r ${GLAD_SOURCES_DIR}/requirements.txt
    )

# Path to glad cmake files
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)

# Specify glad settings
glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:compatibility=4.6)