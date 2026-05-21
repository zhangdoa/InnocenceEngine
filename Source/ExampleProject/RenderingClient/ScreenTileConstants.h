#pragma once
#include <cstdint>

// C++ mirror of the screen-space dispatch tile size used by every full-screen
// compute pass. The HLSL canonical value is the `[numthreads(8,8,1)]` literal
// in Source/Shaders/HLSL/lightPass.comp:151 (and ~14 sibling shaders:
// skyPass.comp, finalBlendPass.comp, preTAAPass.comp, postTAAPass.comp,
// TAAPass.comp, SSAONoisePass.comp, SSRCFilterHorizontal.comp,
// SSRCFilterVertical.comp, SSRCReprojection.comp,
// SSRCTemporal.comp, common/SSRCSpatialCommon.hlsl, PTNRDFormatConvert.comp,
// PTNRDComposition.comp, PTToneMap.hlsl, mipmapGenerator2D.comp).
// Consumers on the C++ side (dispatch extents = viewportSize / tile) must
// consult this header rather than redefining the constant per-pass. The
// header lives in RenderingClient because every consumer is a RenderingClient
// pass; promote to Engine/Common only if a non-client consumer appears.

namespace Inno
{
namespace ScreenTile
{
    static constexpr uint32_t SCREEN_TILE_SIZE = 8u;
}
} // namespace Inno
