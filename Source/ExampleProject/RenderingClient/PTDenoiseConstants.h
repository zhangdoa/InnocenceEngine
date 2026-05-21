#pragma once

// Compile-time toggle for the PT denoiser GBuffer-equivalent UAVs. MUST stay in lockstep with
// Source/Shaders/HLSL/common/PTDenoiseShared.hlsl's PT_DENOISE_ENABLED — drift produces a
// root-signature / DXIL mismatch.

namespace Inno
{
namespace PTDenoise
{
    static constexpr bool ENABLED = true;
}
} // namespace Inno
