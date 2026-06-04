#pragma once

#include <cstdint>

// NRD integration toggle and runtime-tunable settings. Header is self-contained — does not
// #include any NRD symbol so OFF-builds compile without the NRD submodule.

namespace Inno
{
namespace NRD
{
    // True iff CMake configured with BUILD_WITH_NRD=ON. Engine call sites gate NRD references
    // behind `if constexpr (Inno::NRD::ENABLED)` so OFF-builds emit no NRD symbols.
#if defined(INNO_BUILD_WITH_NRD) && (INNO_BUILD_WITH_NRD != 0)
    static constexpr bool ENABLED = true;
#else
    static constexpr bool ENABLED = false;
#endif

    // ReBLUR hit-distance normalization parameters. NRD v4.17.4's ReblurHitDistanceParameters
    // declares only A/B/C; D is unused but kept for forward compat.
    struct HitDistParams
    {
        float A;
        float B;
        float C;
        float D;
    };
    static constexpr HitDistParams DefaultHitDistParams = { 3.0f, 0.1f, 20.0f, -25.0f };

    // NV's docs do not guarantee correctness on non-NV vendors. Setting this false re-enables
    // ReBLUR on AMD/Intel for testing.
    static constexpr bool FORCE_OFF_ON_NON_NV_GPU = true;

    struct DenoiserSettings
    {
        HitDistParams HitDistanceParams;

        // [0, 63]. NRD stock default 30 (~0.5s @ 60fps). 16 (~0.27s) tuned for engine's 1 spp
        // budget — faster turnover at the cost of less stability under sustained motion.
        uint32_t MaxAccumulatedFrameNum;

        // Live-toggleable via DevToggleRegistry "NRDAntiFirefly".
        bool EnableAntiFirefly;

        // Normalized %; spatial-filter normal-weight aggressiveness. NRD stock 0.15. Lower
        // tightens the footprint around the surface normal — sharper shadow-edge transitions on
        // diffuse surfaces where shadow boundaries have continuous normals.
        float LobeAngleFraction;

        // Normalized %; spatial-filter roughness-weight aggressiveness. NRD stock 0.15.
        float RoughnessFraction;
    };

    static constexpr DenoiserSettings k_InitialDenoiserSettings = {
        DefaultHitDistParams,
        16u,
        true,
        0.05f,
        0.05f
    };

    // Read by NRDIntegrationAdapter_Dispatch.cpp each frame.
    inline DenoiserSettings g_DenoiserSettings = k_InitialDenoiserSettings;

    // PCI-SIG NVIDIA vendor ID. Matched against IDXGIAdapter::GetDesc().VendorId.
    static constexpr uint32_t NVIDIA_VENDOR_ID = 0x10DEu;
}
} // namespace Inno
