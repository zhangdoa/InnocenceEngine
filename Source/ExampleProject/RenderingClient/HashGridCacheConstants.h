#pragma once
#include <cstdint>

// C++ mirror of the world-space hash-grid radiance cache layout constants
// defined in Source/Shaders/HLSL/common/HashGridCache.hlsl. The shader owns
// the canonical values; consumers on the C++ side (buffer allocation,
// binding layout) must consult this header rather than redefining the
// constants per-pass.
//
// TASK-77.1 design call (2026-04-30): cell-only simplification of the
// Capsaicin GI-1.0 hash_grid_cache structure. Phase 1 uses a flat hash
// table (no tile/cell mip chain) keyed on (quantize(posWS), packOcta(N)),
// open-addressing with probe length 4. The Capsaicin-style tile/mip
// machinery is intentionally absent — it pays for itself only at the
// secondary-vertex usage which is deferred to phase 2.

namespace Inno
{
namespace HashGridCache
{
    // Capacity in cells. 2^20 = 1,048,576 cells. Per-cell payload is
    // 20 B (uint3 quantizedRadianceSum + uint sampleCount + uint
    // frameLastTouched); per-cell key is 4 B (uint hash). Total = 24 MB
    // at full capacity, matching the design call's ~25 MB budget.
    // Configurable here at code-level (not yet runtime-toggled per
    // design call).
    static constexpr uint32_t CELL_COUNT = 1u << 20;

    // Open-addressing linear-probe length. Capsaicin's reference impl
    // uses 4 tiles per bucket; our cell-only scheme reuses the same
    // bound — past 4 collisions on a key, the new write evicts the
    // oldest cell in the probe chain (frameLastTouched-based).
    static constexpr uint32_t PROBE_LENGTH = 4u;

    // Per-cell sample cap (write-side). Past the cap, contributions are
    // dropped — see HASHGRID_SAMPLE_CAP in HashGridCache.hlsl for the
    // overflow + denoiser-saturation rationale.
    static constexpr uint32_t SAMPLE_CAP = 32u;

    // Per-cell payload size (must match HashGridCell in the shader).
    // 20 B = uint3 quantizedRadianceSum (12 B) + uint sampleCount (4 B)
    // + uint frameLastTouched (4 B). Zero-initialised buffer is the
    // valid empty state — atomic-add accumulation seeds correctly from
    // sum=0, sampleCount=0. See HashGridCache.hlsl for the per-channel
    // InterlockedAdd shape (TASK-77.1.3 D9 fix).
    static constexpr uint32_t CELL_BYTES = 20u;

    // Per-cell key size (uint hash; 0 = empty slot, MSB-set values
    // are valid hashes).
    static constexpr uint32_t KEY_BYTES = 4u;
}
} // namespace Inno
