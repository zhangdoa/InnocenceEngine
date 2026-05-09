function(build_third_party name src_dir cmake_args)
    string(TOUPPER ${name} UPPER_NAME)

    # Optional 4th positional argument: a CMake config name to use when
    # configuring + building the third-party (e.g. "Release"). Some submodules
    # (NRD) hard-code CMAKE_CONFIGURATION_TYPES = "Debug;Release" for multi-
    # config generators and reject the engine's RelWithDebInfo, mirroring the
    # mapping that build_physx_custom does inline. Defaults to ${CMAKE_BUILD_TYPE}
    # so the existing assimp call stays unchanged.
    set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
    if(ARGC GREATER 3)
        set(BUILD_CONFIG ${ARGV3})
    endif()

    # Simple build directory under main project build
    set(BUILD_DIR ${CMAKE_BINARY_DIR}/third_party/${name})

    # Check if already built (look for any lib files)
    file(GLOB_RECURSE EXISTING_LIBS "${BUILD_DIR}/*${CMAKE_STATIC_LIBRARY_SUFFIX}")

    set(SHOULD_BUILD FALSE)
    if(NOT EXISTING_LIBS OR FORCE_REBUILD_THIRD_PARTY)
        set(SHOULD_BUILD TRUE)
    endif()

    if(BUILD_THIRD_PARTY AND SHOULD_BUILD)
        message(STATUS "Building ${name} (${BUILD_CONFIG})...")

        # Configure and build in one go
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S ${src_dir}
                -B ${BUILD_DIR}
                -DCMAKE_BUILD_TYPE=${BUILD_CONFIG}
                -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/install
                ${cmake_args}
            RESULT_VARIABLE CONFIG_RESULT
        )

        if(CONFIG_RESULT)
            message(FATAL_ERROR "Failed to configure ${name}")
        endif()

        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --config ${BUILD_CONFIG} --parallel
            RESULT_VARIABLE BUILD_RESULT
        )

        if(BUILD_RESULT)
            message(FATAL_ERROR "Failed to build ${name}")
        endif()

        # Install for clean layout
        execute_process(
            COMMAND ${CMAKE_COMMAND} --install ${BUILD_DIR} --config ${BUILD_CONFIG}
        )
    else()
        message(STATUS "Skipping ${name} build (already exists or BUILD_THIRD_PARTY=OFF)")
    endif()

    # Always copy DLLs to runtime output directory (whether built or not).
    # Deployed under the engine's runtime config (CMAKE_BUILD_TYPE), not the
    # third-party's BUILD_CONFIG — the engine binary lives in the engine config
    # directory and Windows DLL search resolves there.
    file(GLOB_RECURSE DLL_FILES
        "${BUILD_DIR}/install/bin/*.dll"
        "${BUILD_DIR}/bin/*.dll"
        "${BUILD_DIR}/*.dll"
    )
    if(DLL_FILES AND CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        foreach(DLL_FILE ${DLL_FILES})
            file(COPY ${DLL_FILE} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
        endforeach()
        message(STATUS "${name}: Copied DLLs to runtime directory")
    endif()

    # Find libraries and headers (check both build and install locations)
    file(GLOB_RECURSE LIBS 
        "${BUILD_DIR}/install/lib/*${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${BUILD_DIR}/lib/*${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${BUILD_DIR}/*${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
    
    # Find include directory
    set(INCLUDE_CANDIDATES 
        "${BUILD_DIR}/install/include"
        "${BUILD_DIR}/include" 
        "${src_dir}/include"
    )
    
    foreach(CANDIDATE ${INCLUDE_CANDIDATES})
        if(EXISTS ${CANDIDATE})
            set(${UPPER_NAME}_INCLUDE_DIR ${CANDIDATE} PARENT_SCOPE)
            break()
        endif()
    endforeach()
    
    set(${UPPER_NAME}_LIBS ${LIBS} PARENT_SCOPE)
    
    if(LIBS)
        list(LENGTH LIBS LIB_COUNT)
        message(STATUS "${name}: Found ${LIB_COUNT} libraries")
    else()
        message(WARNING "${name}: No libraries found")
    endif()
endfunction()

function(build_physx_custom)
    set(name "PhysX")
    string(TOUPPER ${name} UPPER_NAME)
    set(PHYSX_DIR ${INNO_GITSUBMODULE_DIRECTORIES}/PhysX/physx)
    
    # Map CMake config to PhysX config
    set(PHYSX_CONFIG ${CMAKE_BUILD_TYPE})
    if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(PHYSX_CONFIG "Release")  # PhysX doesn't have RelWithDebInfo
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(PHYSX_CONFIG "Release")  # PhysX doesn't have MinSizeRel
    endif()
    
    # Simple build directory under main project build
    set(BUILD_DIR ${CMAKE_BINARY_DIR}/third_party/${name})
    
    # Check if already built
    file(GLOB_RECURSE EXISTING_LIBS "${BUILD_DIR}/*.lib")
    
    set(SHOULD_BUILD FALSE)
    if(NOT EXISTING_LIBS OR FORCE_REBUILD_THIRD_PARTY)
        set(SHOULD_BUILD TRUE)
    endif()
    
    if(BUILD_THIRD_PARTY AND SHOULD_BUILD)
        message(STATUS "Building PhysX (${CMAKE_BUILD_TYPE} -> ${PHYSX_CONFIG})...")
        
        # Patch PhysX configuration
        set(PHYSX_PRESET_FILE ${PHYSX_DIR}/buildtools/presets/public/vc17win64.xml)
        file(READ ${PHYSX_PRESET_FILE} CONTENTS)
        string(REPLACE "\"NV_USE_STATIC_WINCRT\" value=\"True\"" "\"NV_USE_STATIC_WINCRT\" value=\"False\"" CONTENTS "${CONTENTS}")
        file(WRITE ${PHYSX_PRESET_FILE} "${CONTENTS}")
        
        # Generate and build
        execute_process(
            COMMAND ${PHYSX_DIR}/generate_projects.bat vc17win64 
            WORKING_DIRECTORY ${PHYSX_DIR}
            RESULT_VARIABLE GEN_RESULT
        )
        
        if(GEN_RESULT)
            message(FATAL_ERROR "Failed to generate PhysX projects")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${PHYSX_DIR}/compiler/vc17win64/ --config ${PHYSX_CONFIG} --parallel
            RESULT_VARIABLE BUILD_RESULT
        )
        
        if(BUILD_RESULT)
            message(FATAL_ERROR "Failed to build PhysX")
        endif()
        
        # Copy built libraries to our build directory for consistency
        string(TOLOWER "${PHYSX_CONFIG}" CONFIG_LOWER)
        file(GLOB PHYSX_OUTPUT_DIR "${PHYSX_DIR}/bin/win.x86_64.vc143.md/${CONFIG_LOWER}")
        
        if(PHYSX_OUTPUT_DIR)
            file(MAKE_DIRECTORY ${BUILD_DIR})
            file(GLOB PHYSX_BUILT_LIBS "${PHYSX_OUTPUT_DIR}/*.lib")
            foreach(LIB ${PHYSX_BUILT_LIBS})
                file(COPY ${LIB} DESTINATION ${BUILD_DIR})
            endforeach()
        endif()
    else()
        message(STATUS "Skipping PhysX build (already exists or BUILD_THIRD_PARTY=OFF)")
    endif()

    # Always copy PhysX DLLs to runtime directory (whether built or not)
    string(TOLOWER "${PHYSX_CONFIG}" CONFIG_LOWER)
    file(GLOB PHYSX_OUTPUT_DIR "${PHYSX_DIR}/bin/win.x86_64.vc143.md/${CONFIG_LOWER}")
    if(PHYSX_OUTPUT_DIR)
        file(GLOB PHYSX_DLLS "${PHYSX_OUTPUT_DIR}/*.dll")
        if(PHYSX_DLLS AND CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            foreach(DLL_FILE ${PHYSX_DLLS})
                file(COPY ${DLL_FILE} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
            endforeach()
            message(STATUS "PhysX: Copied DLLs to runtime directory")
        endif()
    endif()

    # Find PhysX libraries and set variables
    file(GLOB PHYSX_LIBS "${BUILD_DIR}/*.lib")
    if(NOT PHYSX_LIBS)
        # Fallback to original location if not copied yet
        string(TOLOWER "${PHYSX_CONFIG}" CONFIG_LOWER)
        file(GLOB PHYSX_OUTPUT_DIR "${PHYSX_DIR}/bin/win.x86_64.vc143.md/${CONFIG_LOWER}")
        file(GLOB PHYSX_LIBS "${PHYSX_OUTPUT_DIR}/*.lib")
    endif()
    
    set(PHYSX_LIBS ${PHYSX_LIBS} PARENT_SCOPE)
    set(PHYSX_INCLUDE_DIR "${PHYSX_DIR}/include" PARENT_SCOPE)
    
    if(PHYSX_LIBS)
        list(LENGTH PHYSX_LIBS LIB_COUNT)
        message(STATUS "PhysX: Found ${LIB_COUNT} libraries")
    else()
        message(WARNING "PhysX: No libraries found")
    endif()
endfunction()

# Build third parties
build_third_party(
    assimp
    ${INNO_GITSUBMODULE_DIRECTORIES}/assimp
    "-DASSIMP_BUILD_ASSIMP_TOOLS=OFF;-DASSIMP_BUILD_TESTS=OFF;-DASSIMP_BUILD_SAMPLES=OFF;-DBUILD_SHARED_LIBS=OFF;-DASSIMP_NO_EXPORT=ON"
)

build_physx_custom()

# NVIDIA NRD (RayTracingDenoiser). Gated on the root BUILD_WITH_NRD option so
# OFF-builds skip submodule compile entirely and stay byte-identical to a
# pre-NRD baseline. NRD has self-contained CMake (mirrors the assimp shape)
# and pulls ShaderMake + MathLib via FetchContent during configure. Static
# library variant chosen to keep deployment to a single PE; the SHARED default
# would require extra DLL copy plumbing which CL-1 does not need (no engine TU
# wires to NRD until CL-2). Engine code reads INNO_BUILD_WITH_NRD via the
# Inno::NRD::ENABLED constexpr in NRDConstants.h.
#
# 4th arg "Release": NRD's CMakeLists hard-codes CMAKE_CONFIGURATION_TYPES to
# "Debug;Release" for multi-config generators (Visual Studio), so passing the
# engine's RelWithDebInfo causes "configuration not found" at the --build step.
# Mirrors the RelWithDebInfo→Release mapping that build_physx_custom does.
if(BUILD_WITH_NRD)
    build_third_party(
        NRD
        ${INNO_GITSUBMODULE_DIRECTORIES}/NRD
        "-DNRD_STATIC_LIBRARY=ON"
        "Release"
    )
endif()
