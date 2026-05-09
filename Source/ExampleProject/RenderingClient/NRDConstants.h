#pragma once

// CL-1 build-system skeleton for NVIDIA NRD (RayTracingDenoiser) integration.
// Mirrors the constexpr ENABLED + DevToggleRegistry runtime-toggle shape used
// by Inno::PTHashGridCache. Engine code lands in CL-2 / CL-3 of TASK-77.4 and
// will branch on Inno::NRD::ENABLED via `if constexpr`; the constant is
// authoritative on whether NRD library symbols are linked at all.
//
// This header is deliberately self-contained: it does NOT #include any NRD
// header. That keeps the file safe to include in OFF-builds where the NRD
// submodule may be absent or unbuilt, and lets the constexpr ENABLED act as
// the single gate that downstream call sites read. Adapter code that pulls
// NRDIntegration.hpp / NRDDescs.h lives in a separate TU (CL-3).
//
// The compile-definition INNO_BUILD_WITH_NRD is propagated by
// Source/ExampleProject/RenderingClient/CMakeLists.txt and tracks the root
// CMake option BUILD_WITH_NRD. The define is added to the compile command
// line ONLY when ON (CMake script does not emit it on OFF) so that OFF-builds
// produce binaries bit-identical to a pre-NRD baseline; the absent-macro arm
// below maps to ENABLED = false. PRIVATE-scoped on the RenderingClient target
// — including this header from a TU outside that target will compile but
// always read ENABLED = false; that is intentional, NRD is RenderingClient-
// internal.

namespace Inno
{
namespace NRD
{
    // Compile-time toggle. true iff CMake configured with BUILD_WITH_NRD=ON
    // AND the consuming TU lives inside the ExampleRenderingClient target.
    // Engine call sites in CL-2 / CL-3 wrap NRD-calling code under
    // `if constexpr (Inno::NRD::ENABLED) { ... }` so the OFF-build elides
    // every NRD reference at compile time and the linker sees no NRD symbols.
#if defined(INNO_BUILD_WITH_NRD) && (INNO_BUILD_WITH_NRD != 0)
    static constexpr bool ENABLED = true;
#else
    static constexpr bool ENABLED = false;
#endif

    // ReBLUR hit-distance normalization parameters (NV reference defaults
    // from NRDDescs.h::HitDistanceParameters: A=3, B=0.1, C=20, D=-25).
    // ReBLUR uses these to map raw hit distance (in metres) to the [0,1]
    // normalized form packed into IN_*_RADIANCE_HITDIST. Exposed here as a
    // POD struct so CL-3's HitDistParams DevToggleRegistry knob can vary
    // them at runtime around the compile-time defaults; CL-1 ships defaults
    // only.
    struct HitDistParams
    {
        float A;
        float B;
        float C;
        float D;
    };
    static constexpr HitDistParams DefaultHitDistParams = { 3.0f, 0.1f, 20.0f, -25.0f };
}
} // namespace Inno
