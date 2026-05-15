#pragma once
#include <cstdint>

// C++ mirror of the light-culling tile size. The HLSL canonical value is
// `#define LIGHT_CULLING_BLOCK_SIZE 16` in Source/Shaders/HLSL/common/common.hlsl,
// consumed as the `[numthreads(LIGHT_CULLING_BLOCK_SIZE, LIGHT_CULLING_BLOCK_SIZE, 1)]`
// dispatch shape in Source/Shaders/HLSL/lightCulling.comp and
// Source/Shaders/HLSL/tileFrustum.comp, and as the per-tile divisor in
// Source/Shaders/HLSL/lightPass.comp and
// Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl. Consumers on the
// C++ side (dispatch extents = viewportSize / TILE_SIZE, thread-count
// reporting in the DispatchParams cbuffer) must consult this header rather
// than redefining the constant per-pass. The header lives in RenderingClient
// because every consumer is a RenderingClient pass; promote to Engine/Common
// only if a non-client consumer appears.

namespace Inno
{
namespace LightCulling
{
    static constexpr uint32_t TILE_SIZE = 16u;
}
} // namespace Inno
