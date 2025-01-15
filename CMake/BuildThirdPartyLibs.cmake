function(build_third_party name src_dir cmake_args)
    # Convert name to uppercase for variable names
    string(TOUPPER ${name} UPPER_NAME)

    # Use a single global configuration
    set(CONFIG ${CMAKE_BUILD_TYPE})
    if (NOT CONFIG)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. Please set it to Debug, Release, etc.")
    endif()

    # Dynamically set build directories
    set(BUILD_DIR ${CMAKE_BINARY_DIR}/ThirdParty/${name}/${CONFIG})
    set(${UPPER_NAME}_LIBRARY_DIR "${BUILD_DIR}/lib/${CONFIG}" PARENT_SCOPE)
    set(${UPPER_NAME}_INCLUDE_DIR "${BUILD_DIR}/include" PARENT_SCOPE)

    if (BUILD_THIRD_PARTY)
        # Create build directory
        file(MAKE_DIRECTORY ${BUILD_DIR})

        # Generate and build the third-party project
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S ${src_dir}
                -B ${BUILD_DIR}
                ${cmake_args}
                -DCMAKE_BUILD_TYPE=${CONFIG}  # Pass the build type
            WORKING_DIRECTORY ${BUILD_DIR}
        )

        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --config ${CONFIG} -- -j
        )
    else()
        message(STATUS "Skipping build for ${name} (${CONFIG})")
    endif()

    # Copy runtime files (e.g., .dll) to runtime output directory
    file(GLOB DLL_FILES "${BUILD_DIR}/bin/${CONFIG}/*.dll")
    if (DLL_FILES)
        foreach(DLL_FILE ${DLL_FILES})
            file(COPY ${DLL_FILE} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG})
        endforeach()
        message(STATUS "Copied DLLs for ${name} (${CONFIG}) to runtime output directory.")
    else()
        message(WARNING "No DLL files found for ${name} (${CONFIG}) in ${BUILD_DIR}/bin/${CONFIG}.")
    endif()

    # Dynamically locate all library files
    file(GLOB LIBS "${BUILD_DIR}/lib/${CONFIG}/*.lib" "${BUILD_DIR}/lib/${CONFIG}/*.a" "${BUILD_DIR}/lib/*.so" "${BUILD_DIR}/lib/*.dylib")

    if (LIBS)
        # Save all library files as a list
        set(${UPPER_NAME}_LIBS ${LIBS} PARENT_SCOPE)
        message(STATUS "Found libraries for ${name} (${CONFIG}): ${LIBS}")
    else()
        message(WARNING "No library files found for ${name} in ${BUILD_DIR}/lib/${CONFIG}")
    endif()

    # Update CMAKE_PREFIX_PATH to include the build directory
    list(APPEND CMAKE_PREFIX_PATH ${BUILD_DIR})
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
endfunction()


function(build_physx)
    set(PHYSX_NAME "PhysX")
    set(PHYSX_DIR ${INNO_GITSUBMODULE_DIRECTORIES}/PhysX/physx)

    # Use a single global configuration
    set(CONFIG ${CMAKE_BUILD_TYPE})
    if (NOT CONFIG)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. Please set it to Debug, Release, etc.")
    endif()

    if (CONFIG STREQUAL "RelWithDebInfo")
        set(CONFIG "Release") # TODO: Use the Profile configuration
    endif()
   
    set(BUILD_DIR ${CMAKE_BINARY_DIR}/ThirdParty/${PHYSX_NAME}/${CONFIG})

    if (BUILD_THIRD_PARTY)
        # Pre-build patch (specific to PhysX)
        set(PHYSX_PRESET_FILE ${PHYSX_DIR}/buildtools/presets/public/vc17win64.xml)
        file(READ ${PHYSX_PRESET_FILE} CONTENTS)
        string(REPLACE "\"NV_USE_STATIC_WINCRT\" value=\"True\"" "\"NV_USE_STATIC_WINCRT\" value=\"False\"" CONTENTS "${CONTENTS}")
        file(WRITE ${PHYSX_PRESET_FILE} "${CONTENTS}")
        message(STATUS "Patched PhysX configuration for ${CONFIG}")

        # Generate projects
        message(STATUS "Generating PhysX projects (${CONFIG})")
        execute_process(
            COMMAND ${PHYSX_DIR}/generate_projects.bat vc17win64
            WORKING_DIRECTORY ${PHYSX_DIR}
        )

        # Build the projects
        message(STATUS "Building PhysX (${CONFIG})")
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${PHYSX_DIR}/compiler/vc17win64/
                --config ${CONFIG} -j # multi-thread
            WORKING_DIRECTORY ${PHYSX_DIR}/compiler/vc17win64
        )
    else()
        message(STATUS "Skipping PhysX build (${CONFIG})")
    endif()

    string(TOLOWER "${CONFIG}" CONFIG_LOWER)
    file(GLOB PHYSX_OUTPUT_DIR "${PHYSX_DIR}/bin/win.x86_64.vc143.md/${CONFIG_LOWER}")
    if (NOT PHYSX_OUTPUT_DIR)
        message(FATAL_ERROR "Failed to find PhysX output directory for configuration: ${CONFIG}")
    endif()

    # Collect library paths strictly for the current configuration
    file(GLOB CONFIG_LIB_FILES "${PHYSX_OUTPUT_DIR}/*.lib")

    if (CONFIG_LIB_FILES)
        list(APPEND PHYSX_LIBS ${CONFIG_LIB_FILES})
        message(STATUS "Collected PhysX libraries for ${CONFIG}: ${CONFIG_LIB_FILES}")
    else()
        message(WARNING "No PhysX libraries found for ${CONFIG} in ${PHYSX_OUTPUT_DIR}")
    endif()

    # Copy runtime files (e.g., .dll) to runtime output directory
    file(GLOB DLL_FILES "${PHYSX_OUTPUT_DIR}/*.dll")
    if (DLL_FILES)
        foreach(DLL_FILE ${DLL_FILES})
            file(COPY ${DLL_FILE} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
        endforeach()
        message(STATUS "Copied DLLs for PhysX (${CONFIG}) to runtime output directory.")
    else()
        message(WARNING "No DLL files found for PhysX (${CONFIG}) in ${PHYSX_OUTPUT_DIR}.")
    endif()

    # Export variables to parent scope
    set(PHYSX_LIBS ${PHYSX_LIBS} PARENT_SCOPE)
    set(PHYSX_INCLUDE_DIR "${PHYSX_DIR}/physx/include" PARENT_SCOPE)
endfunction()

function(build_glad)
    # Path to glad directory
    set(GLAD_SOURCES_DIR "${INNO_GITSUBMODULE_DIRECTORIES}/GLAD")

    # Install the dependencies
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pip install -r ${GLAD_SOURCES_DIR}/requirements.txt
        )

    # Path to glad cmake files
    add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)

    # Specify glad settings
    set(GLAD_HEADER_LIB_NAME "glad_gl_core_46")
    glad_add_library(${GLAD_HEADER_LIB_NAME} REPRODUCIBLE API gl:core=4.6)
endfunction()
    
build_third_party(
    assimp
    ${INNO_GITSUBMODULE_DIRECTORIES}/assimp
    "-DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF"
)

build_physx()

build_glad()