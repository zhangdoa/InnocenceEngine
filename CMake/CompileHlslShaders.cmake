# CompileHlslShaders.cmake
#
# Wires the HLSL -> DXIL compile pass into the CMake build graph so
# `cmake --build` produces fresh DXIL the same way `Scripts/BuildWin.ps1`
# does. The BuildWin path invokes `Scripts/HLSL2DXIL.ps1 -NoPause` itself
# as a pre-step; the `cmake --build` path bypasses that script entirely
# and the engine then loads stale DXIL after any HLSL edit (TASK-202).
#
# The custom target defined here invokes the same script with the same
# -NoPause switch (TASK-212 Phase 2 consolidated the previous
# HLSL2DXIL_NoPause.ps1 variant into this switch). The script is
# idempotent — `Scripts/Lib/Compile-HLSL.psm1` does per-shader and
# per-`#include` LastWriteTimeUtc comparisons against the output DXIL
# and skips up-to-date shaders — so wiring it as ALL is cheap on a
# no-edit build (one directory enumeration).
#
# Runtime targets (Main / RenderTest) declare a build-graph dependency
# on this target via `add_dependencies(<target> CompileHlslShaders)`,
# guaranteeing the HLSL recompile runs BEFORE their POST_BUILD
# `inno_deploy_runtime_payload` step mirrors `Bin/Shaders/DXIL/` ->
# `Bin/<Config>/Shaders/DXIL/`. Mirror semantics from TASK-146 then
# propagate fresh DXIL to the per-config payload automatically.
#
# Windows-only: HLSL2DXIL.ps1 shells out to PowerShell and dxc.
# Non-Win platforms have no HLSL2DXIL pipeline; the target is skipped.

if(NOT WIN32)
    return()
endif()

set(_compile_hlsl_script "${CMAKE_SOURCE_DIR}/Scripts/HLSL2DXIL.ps1")

if(NOT EXISTS "${_compile_hlsl_script}")
    message(FATAL_ERROR
        "CompileHlslShaders: script not found at ${_compile_hlsl_script}")
endif()

# ALL keyword: target builds on default `cmake --build` invocations even
# when the user names a different target (e.g. `--target Main`). The
# `add_dependencies` calls in WinMain/CMakeLists.txt make the ordering
# explicit; ALL is belt-and-suspenders for ad-hoc `cmake --build` calls
# that do not pull in Main / RenderTest.
add_custom_target(CompileHlslShaders ALL
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File "${_compile_hlsl_script}" -NoPause
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "HLSL -> DXIL (idempotent; skips unchanged shaders)"
    VERBATIM
    USES_TERMINAL
)

set_target_properties(CompileHlslShaders PROPERTIES FOLDER Engine/Shaders)
