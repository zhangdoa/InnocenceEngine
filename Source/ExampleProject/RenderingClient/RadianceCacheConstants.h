#pragma once
#include <cstdint>

// C++ mirror of the radiance-cache layout constants from RayTracingTypes.hlsl and
// common/RadianceCacheCommon.hlsl. Shaders own the canonical values; drift between this
// header and the shaders crops or oversizes the SH atlas.

namespace Inno
{
namespace RadianceCache
{
    // Pixel size of one screen probe tile. Mirrors RADIANCE_CACHE_TILE_SIZE
    // in common/RadianceCacheCommon.hlsl and TILE_SIZE in RayTracingTypes.hlsl.
    // Drives probe-grid extent (viewportSize / TILE_SIZE) and every dispatch
    // that runs at probe granularity (the four RadianceCache* passes plus
    // GIDenoise).
    static constexpr uint32_t TILE_SIZE = 8u;

    // Per-probe SH storage extent in cells. 3×3 = 9 coefficients (SH bands
    // 0–2) per the GI-1.0 paper §2.4.2 and Ramamoorthi–Hanrahan 2001
    // cosine-lobe convolution used by LoadIrradiance in
    // common/RadianceCacheCommon.hlsl. Mirrors SH_TILE_SIZE in
    // RayTracingTypes.hlsl. The integration shader writes 9 cells per probe
    // in row-major order; the consumer (SampleRadianceCache) reads the same
    // 3×3 footprint.
    static constexpr uint32_t SH_TILE_SIZE = 3u;

    // Sparse-spawn upscale (paper §2.1.1). Mirrors upscaleFactor in
    // RayTracingTypes.hlsl. One probe spawned per (TILE_SIZE * UPSCALE_X,
    // TILE_SIZE * UPSCALE_Y) pixels per frame; the Halton sequence cycles
    // through all UPSCALE_X * UPSCALE_Y probe slots over that many frames.
    static constexpr uint32_t UPSCALE_X = 2u;
    static constexpr uint32_t UPSCALE_Y = 2u;
    static constexpr uint32_t SPAWN_TILE_SIZE_X = TILE_SIZE * UPSCALE_X;
    static constexpr uint32_t SPAWN_TILE_SIZE_Y = TILE_SIZE * UPSCALE_Y;

    // Ceiling division — every probe-granularity dispatch needs the same
    // (extent + TILE_SIZE - 1) / TILE_SIZE shape, so factor it once. Using
    // floor division here (e.g. extent / TILE_SIZE) silently drops the right
    // and bottom strip when the framebuffer is not a multiple of TILE_SIZE.
    constexpr uint32_t TileCount(uint32_t in_PixelExtent)
    {
        return (in_PixelExtent + TILE_SIZE - 1u) / TILE_SIZE;
    }
}
} // namespace Inno
