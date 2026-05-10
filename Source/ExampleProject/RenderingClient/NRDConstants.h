#pragma once

#include <cstdint>

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

    // ReBLUR hit-distance normalization parameters. NRD v4.17.4's
    // ReblurHitDistanceParameters (NRDSettings.h:207) declares only A/B/C —
    // the historical D field shipped in CL-1's NRDConstants.h is dead under
    // this NRD version (no NRD code reads a fourth coefficient). D is left
    // in the struct as a forward-compat placeholder; the dispatch site only
    // copies A/B/C into nrd::ReblurHitDistanceParameters. Defaults match
    // NRD's own struct defaults.
    struct HitDistParams
    {
        float A;
        float B;
        float C;
        float D;  // unused under NRD v4.17.4; kept for forward compat. CL-4.
    };
    static constexpr HitDistParams DefaultHitDistParams = { 3.0f, 0.1f, 20.0f, -25.0f };

    // CL-4 force-off-on-non-NV switch. Compile-time so the runtime check at
    // NRDIntegrationAdapter::Initialize can be elided if zhangdoa wants to
    // test ReBLUR on AMD/Intel later (flip to false). NV's docs do not
    // guarantee perf or correctness on non-NV vendors; the runtime check
    // below logs a warning + returns false from Initialize when this is
    // true and the active DXGI adapter VendorId != 0x10DE. Failure to
    // initialize the adapter cleanly degrades to the existing fallback:
    // PTNRDDenoisePass returns false from PrepareCommandList → composition
    // pass stays non-Activated → the tonemap source-selection path in
    // ExampleRenderingClient_PrepareCommands.cpp (lines 156-163) reads the
    // raw PT AccumBuffer instead of the composition output. Single binary
    // ships everywhere; AMD/Intel users see raw 1-spp PT.
    static constexpr bool FORCE_OFF_ON_NON_NV_GPU = true;

    // CL-4 ReBLUR tuning hooks. Read from the dispatch site each frame and
    // copied into nrd::CommonSettings / nrd::ReblurSettings before
    // SetCommonSettings + SetDenoiserSettings. Defaults are tuned for the
    // three engine reference scenes (UnitTest sun-shadow edge sharpness,
    // GITestBox directly-lit-area stabilization, GISponza static-frame
    // jitter); see TASK-77.4 Implementation Notes / "CL-4 default-value
    // rationale" for per-knob justification.
    //
    // Mutability shape:
    //   - HitDistParams / MaxAccumulatedFrameNum / LobeAngleFraction /
    //     RoughnessFraction: tweakable by edit-and-recompile at runtime.
    //     The engine has no typed-tunable runtime registry available in the
    //     RenderingClient target (DevToggleRegistry is bool-only;
    //     TweakRegistry lives in LogicClient and would force a cross-client
    //     coupling). These are exposed through a single mutable global
    //     `g_DenoiserSettings` so the dispatch site's read path is a single
    //     known venue for future ImGui / UI hookup.
    //   - EnableAntiFirefly: bool, also routed through DevToggleRegistry as
    //     "NRDAntiFirefly" so it IS live-toggleable from any DevToggle
    //     surface (Editor-Next dev panel, IPC, etc.). The dispatch site
    //     reads from g_DenoiserSettings.EnableAntiFirefly which is the
    //     setter-write target; the registry getter reads the same field.
    //
    // The four-element HitDistParams.D is unused under NRD v4.17.4 (see the
    // struct comment above).
    struct DenoiserSettings
    {
        HitDistParams HitDistanceParams;

        // ReblurSettings::maxAccumulatedFrameNum (NRDSettings.h:265).
        // [0; REBLUR_MAX_HISTORY_FRAME_NUM=63]. NRD's stock default is 30
        // (corresponds to ~0.5s at 60 fps per
        // REBLUR_DEFAULT_ACCUMULATION_TIME). 16 is ~0.27s at 60 fps —
        // chosen lower than stock because (a) the engine's static-camera
        // 60-frame capture harness benefits from faster sample turnover
        // and (b) the user-observed GITestBox 1-2s stabilization on
        // directly-lit areas reads as too-deep history clamping for the
        // engine's sample budget (1 spp). Trade-off: less stable output
        // under sustained motion, more responsive convergence on cuts.
        uint32_t MaxAccumulatedFrameNum;

        // ReblurSettings::enableAntiFirefly (NRDSettings.h:325).
        // NRD's v4.17.4 default is already true ("better keep enabled to
        // maximize quality"). We mirror the default here and route the
        // value through DevToggleRegistry "NRDAntiFirefly" so a developer
        // can A/B against the firefly-suppressor-off path live without a
        // rebuild. Direct effect on the user-reported GISponza motion
        // fireflies / static-camera jitter — NRD's anti-firefly cancels
        // sporadic outliers BEFORE temporal blending, so toggling it
        // ON->OFF will visually surface the suppressor's footprint.
        bool EnableAntiFirefly;

        // ReblurSettings::lobeAngleFraction (NRDSettings.h:303).
        // Normalized %; spatial-filter normal-weight aggressiveness. NRD
        // stock default is 0.15. Lower values tighten the filter footprint
        // around a pixel's surface normal, preserving sharper geometric-
        // edge transitions (UnitTest sun-shadow edges read soft/faded
        // because shadow boundaries on diffuse surfaces have continuous
        // normals — the spatial filter sees no edge to preserve under
        // 0.15). 0.05 is the engine's chosen default; the trade-off is
        // less smoothing across shallow-angle surface variation.
        float LobeAngleFraction;

        // ReblurSettings::roughnessFraction (NRDSettings.h:306).
        // Normalized %; spatial-filter roughness-weight aggressiveness.
        // NRD stock 0.15. Lower values tighten the footprint around the
        // pixel's roughness band — same UnitTest sun-shadow axis.
        float RoughnessFraction;
    };

    // Initial values. Mutability lives at the inline-variable g_DenoiserSettings
    // below; this constexpr instance exists so any future "reset to defaults"
    // codepath has a single source of truth.
    static constexpr DenoiserSettings k_InitialDenoiserSettings = {
        DefaultHitDistParams,
        16u,        // MaxAccumulatedFrameNum (vs NRD stock 30)
        true,       // EnableAntiFirefly (mirrors NRD stock)
        0.05f,      // LobeAngleFraction (vs NRD stock 0.15)
        0.05f       // RoughnessFraction (vs NRD stock 0.15)
    };

    // Mutable runtime-tunable storage. inline so the definition lives in this
    // header and every TU sees the same instance without a separate .cpp.
    // Read by NRDIntegrationAdapter_Dispatch.cpp each frame. Initialised from
    // k_InitialDenoiserSettings; mutated by the DevToggleRegistry setter for
    // EnableAntiFirefly and (for the non-bool fields) by edit-rebuild.
    inline DenoiserSettings g_DenoiserSettings = k_InitialDenoiserSettings;

    // NVIDIA's PCI vendor ID. Used by FORCE_OFF_ON_NON_NV_GPU runtime check
    // at NRDIntegrationAdapter::Initialize against IDXGIAdapter::GetDesc()
    // VendorId. Sourced from PCI-SIG vendor database and matches NV samples
    // (e.g. NRD's own NRDIntegration.hpp vendor-detection helpers).
    static constexpr uint32_t NVIDIA_VENDOR_ID = 0x10DEu;
}
} // namespace Inno
