file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(ExampleRenderingClient ${HEADERS} ${SOURCES})
set_target_properties(ExampleRenderingClient PROPERTIES FOLDER ExampleProject)
target_link_libraries(ExampleRenderingClient Engine)

# Propagate the root BUILD_WITH_NRD option as a compile definition so
# NRDConstants.h can constexpr-branch without #including any NRD header.
# The define is added ONLY when ON — the OFF path adds nothing to the compile
# command line, which is what AC-3 of TASK-77.4 demands ("OFF-build binary
# identical to pre-CL baseline"). NRDConstants.h treats absent INNO_BUILD_WITH_NRD
# as the OFF state. PRIVATE rather than PUBLIC: only RenderingClient TUs use
# the constexpr; consumers of the lib (Main.exe) must not see the define on
# their command line, again to keep OFF-builds bit-identical.
if(BUILD_WITH_NRD)
    target_compile_definitions(ExampleRenderingClient PRIVATE INNO_BUILD_WITH_NRD=1)
    # CL-3 link wiring: NRDIntegrationAdapter pulls NRD headers + links
    # NRD.lib for nrd::CreateInstance / GetComputeDispatches / etc.
    #
    # NRD's own CMakeLists writes the static library to
    # ${submodule}/_Bin/Release/NRD.lib (not into the engine's build tree),
    # so build_third_party's GLOB_RECURSE in CMake/BuildThirdPartyLibs.cmake
    # never picks it up — ${NRD_LIBS} captures only the dxc / ShaderMakeBlob
    # transitively-fetched libs (which we explicitly do NOT want to link;
    # they're build-time tools, and the ARM64/x86 dxc variants would emit
    # LNK4272 machine-type warnings if linked). We therefore reference
    # NRD.lib directly via its known submodule output path.
    set(NRD_SUBMODULE_LIB "${CMAKE_SOURCE_DIR}/Source/External/GitSubmodules/NRD/_Bin/Release/NRD.lib")
    set(NRD_SUBMODULE_INCLUDE "${CMAKE_SOURCE_DIR}/Source/External/GitSubmodules/NRD/Include")
    # NRD pulls in ShaderMake::FindPermutationInBlob from ShaderMakeBlob.lib
    # (FetchContent dependency built under the engine build tree). Link it
    # explicitly — same stable path NRD's own consumers reference.
    set(SHADERMAKE_BLOB_LIB "${CMAKE_BINARY_DIR}/third_party/NRD/_deps/shadermake-build/Release/ShaderMakeBlob.lib")
    target_include_directories(ExampleRenderingClient PRIVATE ${NRD_SUBMODULE_INCLUDE})
    target_link_libraries(ExampleRenderingClient ${NRD_SUBMODULE_LIB} ${SHADERMAKE_BLOB_LIB})
endif()
