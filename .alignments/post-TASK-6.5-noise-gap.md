# Alignment audit — Post-TASK-6.5 noise gap (uniform-distribution full-frame speckle)

- Paper: Boissé et al., "GI-1.0: A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination", AMD Tech. Report 22-10-9831, October 2022 (`Build/GI1_0.pdf`).
- Reference impl: AMD Capsaicin v1.3 at commit `914b91596cd119eda85fbc1d3c7ee6ac391b1452` (2025-11-17), under `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/`.
- In-house impl: `Source/Shaders/HLSL/{RadianceCacheReprojection,RadianceCacheRayGen,RadianceCacheClosestHit,RadianceCacheFilter*,RadianceCacheIntegration,GIDenoise,GIFilter*,lightPass}.{comp,hlsl}` + `Source/ExampleProject/RenderingClient/{RadianceCache,GIDenoise,GIFilter,LightPass}*.cpp`.
- Audit date: 2026-04-26.
- User observation: "ours are so noisy, like a temporal filtering is missing, or the blur is missing."
- Counted summary: **1 faithful (-ish) / 7 divergent / 2 N/A**.

---

## Headline finding

**The noise gap is dominated by a 64× ray-budget shortfall, hidden behind a structural mismatch in how spawned probes feed unspawned tiles.**

Capsaicin's `PopulateScreenProbes` (gi1.cpp:129) traces `max_probe_spawn_count * probe_size² = (W/16)·(H/16) · 64` rays per frame: **one ray per octahedral cell** of every spawned probe. At 1920×1080 that's ~522,240 rays/frame. After `BlendScreenProbes` (gi1.comp:1228–1232) computes a per-probe radiance backup over the 64 cells and `FilterScreenProbes` (gi1.comp:1357–1443) widens that across the 6 nearest probes per direction, every probe-grid cell has a radiance value for every cell direction even though only one probe per spawn-tile got rays this frame.

Our `RadianceCacheRayGen` (`RadianceCacheRayGen.hlsl:306`) hard-codes `NUM_SAMPLES = 1` and the dispatch (`RadianceCacheRaytracingPass.cpp:253–256`) is sized at spawn-tile granularity, so we trace **one ray per spawn-tile per frame** = ~8,160 rays at 1920×1080. That's **64× under Capsaicin's count** at the identical sparse-spawn density. Each spawned probe touches one of its 64 octahedral cells; the other 63 cells get whatever stale value persisted in the atlas, decayed by the 0.1 EMA in `RadianceCacheIntegration.comp:152` if the previous-frame DC differs. The SH projection then projects 1 fresh cell + 63 stale cells, producing an irradiance estimate that wobbles per-frame at every probe.

The temporal accumulator (`GIDenoise.comp`) and spatial bilateral (`GIFilter*.comp`) downstream **cannot rescue this**: they accumulate up to ~16-128 history samples weighted by adaptive cell-size validity gates, but every input frame is itself the noisy 1-ray-per-probe SH estimate. Capsaicin's denoiser is tuned for an input that's already 64-rays-per-probe converged at the screen-probe stage; ours feeds it pre-denoise rays.

Two further structural divergences amplify the visible noise:
1. **No probe-mask MIP chain** in our `FindClosestProbe` (`RadianceCacheCommon.hlsl:93–136`). When all 4 corner probes for a pixel get zero edge-aware weight, our Chebyshev ring walk to radius 2 fails harder than Capsaicin's MIP-cascade fallback (`screen_probes.hlsl:110–166`) which can reach an arbitrarily distant probe in O(log r). Tiles that fail the ring-2 search produce relaxed-interpolation pixels whose denoiser_hint we now (post-TASK-6.5) widen the blur radius for, but the radius caps at `kGIDenoiser_MaxBlurMask = 16.0` per axis (vs Capsaicin's 8.0 — yes, we have a 2× **larger** cap) and the bilateral kernel rejects taps via `c.w > 0` so the radius widening only helps when surrounding pixels have history.
2. **No per-pixel sub-pixel jitter at interpolation site** — Capsaicin's `InterpolateScreenProbes` (gi1.comp:1543–1553) jitters the 4-corner probe-search anchor by a blue-noise sample within ±probe_spawn_tile_size, so adjacent screen pixels read different probe quads each frame and the temporal accumulator integrates over the jitter. Our `SampleRadianceCache` (`RadianceCacheCommon.hlsl:250`) uses a deterministic floor-of-(screenCoord/TILE_SIZE - 0.5) probe quad — every pixel always reads the same 4 probes, so the noise from the under-sampled probes shows up as fixed-pattern speckle rather than averaging into a smooth temporal signal.

**Recommended root-cause fix: raise our per-probe ray budget toward 64/frame** (or at minimum 8–16/frame with adaptive distribution). The remaining divergences are second-order until that gap closes.

---

## Pipeline stage inventory — Capsaicin per-frame dispatches vs ours

Capsaicin dispatch order from `gi1.cpp::render` (gi1.cpp:1716–3030, screen-probe + GI-denoiser path only — multibounce, ReSTIR, and glossy reflection branches are excluded for the GI-only comparison):

| # | Capsaicin stage | gi1.cpp:line | Ours? | Engine site |
|---|-----------------|--------------|-------|-------------|
| 1 | PurgeRadianceCache (decay-tile cleanup) | 2238–2254 | **No** (no decay tracking) | — |
| 2 | ReprojectScreenProbes | 2256–2272 | Yes (different shape) | `RadianceCacheReprojection.comp` |
| 3 | LookupScreenProbes (cached-tile LRU) | 2274–2292 | **No** (no probe cache) | — |
| 4 | SpawnScreenProbes + CompactScreenProbes (Halton-jittered seed inside spawn tile) | 2294–2309 | Implicit (RayGen does Halton inline at `RadianceCacheRayGen.hlsl:189`) | `RadianceCacheRayGen.hlsl:174–304` |
| 5 | PatchScreenProbes (empty-tile redirect) | 2311–2320 | Inline in RayGen (`RadianceCacheRayGen.hlsl:217–272`) | `RadianceCacheRayGen.hlsl` |
| 6 | SampleScreenProbes (importance sample over previous-probe CDF) | 2322–2331 | Inline in RayGen (`ImportanceSampleFromCDF`) | `RadianceCacheRayGen.hlsl:55–172` |
| 7 | PopulateScreenProbes (ray-tracing, **64 rays/probe**) | 2333–2361 | **1 ray/probe** | `RadianceCacheRayGen.hlsl:308–428` |
| 8 | PopulateMultibounceCells (multibounce ray-tracing, gated on `gi1_use_multibounce`) | 2363–2394 | **No** | — |
| 9 | LightSampler::update (per-frame light sampling) | 2396–2399 | **No** | — |
| 10 | (Re)Generate / Compact / ResampleReservoirs (ReSTIR for secondary-ray direct lighting) | 2401–2463 | **No** | — |
| 11 | PopulateRadianceCache (closest-hit tile-cache writes) | 2465–2492 | Inline in `RadianceCacheClosestHit.hlsl:60–95` | `RadianceCacheClosestHit.hlsl` |
| 12 | UpdateRadianceCache (resolve atomic accumulators) | 2494–2503 | **No** (we EMA-blend at write site) | — |
| 13 | UpdateMultibounceCells | 2505–2515 | **No** | — |
| 14 | ResolveCells (project hash-grid cells back into screen probes) | 2517–2529 | **No equivalent** — closest-hit reads `in_LightPassOutgoingLuminance` of previous frame | `RadianceCacheClosestHit.hlsl:53` |
| 15 | BlendScreenProbes (atomic-add ray contributions into probe cells + per-probe **radiance backup** for untraced cells) | 2531–2542 | Implicit / partial — `RadianceCacheIntegration.comp:64–94` has a coverage-gated avg backup, **gated off below 90 % coverage**, and we never reach that gate (~1.5 % coverage at quarter-spp / 1 ray) | `RadianceCacheIntegration.comp:90–94` |
| 16 | ReorderScreenProbes (cached-tile LRU eviction) | 2544–2557 | **No** | — |
| 17 | **FilterProbeMask** (build mip chain for fallback search) | 2559–2586 | **No** — we use a hand-rolled Chebyshev ring walk to radius 2 (`RadianceCacheCommon.hlsl:115–133`) | — |
| 18 | FilterScreenProbes (probe-space separable bilateral blur, **6 taps along each axis × 2 dispatches**, hit-distance parallax + angular-error gate) | 2588–2614 | Yes, structurally similar — `RadianceCacheFilterHorizontal.comp:87–124`, `RadianceCacheFilterVertical.comp:77–108`. ~1:1 paper-faithful (modulo `FindClosestProbe` MIP vs ring) | `RadianceCacheFilter{Horizontal,Vertical}.comp` |
| 19 | ProjectScreenProbes (project 64-cell radiance into 9 SH coefficients) | 2616–2625 | Yes, same shape — `RadianceCacheIntegration.comp:96–128` | `RadianceCacheIntegration.comp` |
| 20 | InterpolateScreenProbes (per-pixel 4-probe edge-aware interpolation **with sub-pixel jitter**, write to GIDenoiser_ColorBuffer) | 2627–2637 | **Different topology** — we don't have a separate interpolate dispatch; instead `GIDenoise.comp:170` calls `SampleRadianceCache` per pixel (`RadianceCacheCommon.hlsl:244–314`). **No sub-pixel jitter** | `RadianceCacheCommon.hlsl::SampleRadianceCache` |
| 21 | TraceReflections + denoise (atrous, fireflies, reproject) | 2639–2867 | **No** (no glossy GI path) | — |
| 22 | ReprojectGI (temporal accumulator, 3×3 reprojection, dynamic cap, blur-mask emission) | 2882–2902 | Yes, ported faithfully — `GIDenoise.comp:136–330` (modulo `kGIDenoiser_MaxBlurMask` value, see below) | `GIDenoise.comp` |
| 23 | FilterGI (separable variable-radius bilateral, 2 dispatches) | 2899–2927 | Yes — `GIFilter{Horizontal,Vertical}.comp` + `GIFilterCommon.hlsl` | `GIFilterCommon.hlsl` |
| 24 | ResolveGI1 (per-pixel material × irradiance compose, raster fullscreen) | 2929–2940 | Yes — `lightPass.comp:139` calls `ComposeIndirectLighting` | `lightPassIndirectCompose.hlsl` |

**Stages we lack that materially affect noise on the GI-only path: 1, 7 (the 64× shortfall), 14, 15 (radiance backup), 17 (probe-mask MIP).** Stages 3, 8–10, 12–13, 16, 21 are quality-of-life or out-of-scope features.

---

## Numerical config table

| Parameter | Paper §/Capsaicin source | Capsaicin value | Our value | Status |
|-----------|--------------------------|-----------------|-----------|--------|
| `probe_size` (octahedral resolution per probe) | gi1.h:191 / RadianceCacheConstants.h:22 | 8 | 8 | FAITHFUL |
| `probe_spawn_tile_size` (sparse-spawn factor in pixels) | gi1.h:195 (QuarterSpp = probe_size << 1) / RadianceCacheConstants.h:39 | 16 | 16 | FAITHFUL |
| **Rays per spawn-tile per frame** | gi1.cpp:129 (`max_ray_count = max_probe_spawn_count * probe_size²`) | **64** | **1** (`RadianceCacheRayGen.hlsl:306`) | **DIVERGENT (64×)** |
| Probe-mask fallback search | screen_probes.hlsl:110–166 | MIP-chain (O(log r) to whole screen) | Chebyshev ring radius 2 (`RadianceCacheCommon.hlsl:91, 115–133`) | DIVERGENT |
| Per-pixel interpolation sub-pixel jitter | gi1.comp:1540–1553 | Blue-noise jittered ±probe_spawn_tile_size | None — deterministic floor of probeUV (`RadianceCacheCommon.hlsl:250`) | DIVERGENT |
| Probe-cell radiance backup for untraced cells | gi1.comp:1213–1232 | Always on, sample-count-weighted average, fills any cell with `sample_count == 0` | Coverage-gated at 0.9 (`RadianceCacheIntegration.comp:90–94`); never fires at 1-ray density | DIVERGENT |
| `kGIDenoiser_MaxBlurMask` (caps history N **and** spatial blur radius) | gi_denoiser.hlsl:26 | **8.0** | `GIDenoise.comp:101` and `GIFilterCommon.hlsl:34` both set to **16.0** | **DIVERGENT (2× wider)** — see detail row |
| `MAX_CAP_FACTOR · MAX_BLUR_MASK` (dynamic history ceiling) | gi1.comp:4092 (`8.0 * min(...)`) | 8 × 8 = 64 samples max | 8 × 16 = 128 samples max (`GIDenoise.comp:113`) | DIVERGENT (consistent with above) |
| Temporal-accumulator min N (history floor) | gi1.comp:4092 | 4 | 4 (`GIDenoise.comp:107`) | FAITHFUL |
| Temporal blend alpha shape | gi1.comp:4077 (color-delta EMA at 1/8) | 1/8 | 1/8 (`GIDenoise.comp:117`) | FAITHFUL |
| Reprojection 3×3 gather sub-pixel weight | gi1.comp:4031 | `kOneOverSqrtOfTwo = 0.707107` | Same (`GIDenoise.comp:122`) | FAITHFUL |
| Bilateral depth-tightness (converged / disocclusion) | gi1.comp:4134 | K = 200 / 20 | K = 200 / 20 (`GIFilterCommon.hlsl:41–42`) | FAITHFUL |
| Bilateral normal-weight power | gi1.comp:4136 | normal² × normal² (= ⁴) | Same (`GIFilterCommon.hlsl:144`) | FAITHFUL |
| Probe-space filter taps per axis | gi1.comp:1393 (`kRadius = 3, kSize = 6`) | 6 | 6 (`RadianceCacheFilterHorizontal.comp:85–86`) | FAITHFUL |
| Probe-space filter angular threshold | screen_probes.hlsl:30 (`cos(2e-2 * PI)`) | 0.998 | 0.998 (`RadianceCacheCommon.hlsl:145`) | FAITHFUL |
| Adaptive cell size formula | §2.1, Algorithm 6 | `tan(fovY · 8 / max(W,H)) · depth / sqrt(2)` (probe path) | Same (`RadianceCacheCommon.hlsl:64–71`) | FAITHFUL (modulo `delta` parameter scoped to 1) |
| Closest-hit on-screen radiance source for bounces | gi1.comp:2517–2529 (`ResolveCells`: re-evaluate hash-grid + ReSTIR) | Hash-grid filtered radiance (mip-cascade) + reservoir sampling | **Previous-frame fully-shaded LightPass output** (`RadianceCacheClosestHit.hlsl:48–58`) | DIVERGENT (energy double-count risk) |

---

## Detail entries (DIVERGENT rows)

### D1 — 64× ray-budget shortfall (the headline)

**Capsaicin** (`gi1.cpp:115–129`):
```
max_probe_spawn_count = ((W + spawn_tile - 1) / spawn_tile) * ((H + spawn_tile - 1) / spawn_tile);
max_ray_count        = max_probe_spawn_count * probe_size_ * probe_size_;
```
Then `gi1.cpp:2326–2330` dispatches `SampleScreenProbes` over `max_ray_count` and `gi1.cpp:2354–2360` dispatches `PopulateScreenProbes` over the same count. `SampleScreenProbes` (gi1.comp:647) maps `did → (cell_index, probe_index)` and writes one ray direction *per cell* of *every spawned probe*. `BlendScreenProbes` (gi1.comp:1167) then atomic-adds the 64 returned radiance values into the probe's 8×8 octahedral atlas — every cell of the 1 spawned probe per spawn-tile gets a fresh sample this frame.

**Ours** (`RadianceCacheRaytracingPass.cpp:253–256`, `RadianceCacheRayGen.hlsl:306`):
```cpp
auto dispatch_x = (...Width  + SPAWN_TILE_SIZE_X - 1u) / SPAWN_TILE_SIZE_X;  // W/16
auto dispatch_y = (...Height + SPAWN_TILE_SIZE_Y - 1u) / SPAWN_TILE_SIZE_Y;  // H/16
DispatchRays(..., dispatch_x, dispatch_y, 1);
```
```hlsl
const int NUM_SAMPLES = 1;
for (int i = 0; i < NUM_SAMPLES; i++) { ... TraceRay(...); ... }
```
One ray per spawn-tile per frame — fills exactly one cell of the spawned probe's 8×8 atlas. The other 63 cells either retain whatever was in `in_RadianceCacheResults` (if reprojection succeeded) or remain at zero (if reprojection wrote zero, which it does on tile invalidate). After Algorithm 3 hysteresis at the write site (`RadianceCacheRayGen.hlsl:338`), the temporal blend `lerp(radiance, oldScreenSpaceRadiance, t)` keeps the new sample at the fresh-cell coordinate but every other cell of the same probe is unchanged this frame.

**Impact**: SH projection (`RadianceCacheIntegration.comp:96–128`) consumes all 64 cells. With only 1 fresh cell per frame at any given probe, the projected SH coefficients are dominated by 63 cells of stale/zero radiance plus 1 newly-traced cell. The 0.1 EMA blend at `RadianceCacheIntegration.comp:152` then averages this against the previous frame's SH, but the input itself wobbles per-frame because *which* cell is fresh changes per Halton+CDF roll. Per-pixel `SampleRadianceCache` reads 4 probes × this wobbly SH and the result has full-frame, full-amplitude per-pixel boil that no downstream filter is sized to absorb.

**Resolution**: Two paths.
1. **Paper-faithful**: increase `NUM_SAMPLES` toward 64. With our current spawn-tile size of 16² = 256 pixels per spawn-tile, and traversal-tier RT cost dominated by the BVH walk, 64 rays per spawn-tile is ~520 K rays @ 1080p — well within real-time budget on modern hardware. If shooting all 64 in one dispatch is too memory-intensive (LDS for the ImportanceSampleFromCDF CDF builds), restructure the RayGen to dispatch at `(W/16, H/16, 64)` so each thread is one cell of one probe (Capsaicin's exact shape).
2. **Cheaper interim**: raise `NUM_SAMPLES` to 8 or 16 with a stratified low-discrepancy sequence over the octahedral hemisphere. Each frame fills 8–16 cells; in 4–8 frames the EMA history fills the rest. Combine with re-enabling the radiance backup gate (D5 below) at a lower threshold (e.g. coverage > 0.05) so untraced cells get a non-zero seed instead of biasing SH DC toward zero.

---

### D2 — Probe-mask MIP chain replaced by Chebyshev ring radius 2

**Paper** (§2.1.5, §2.4.1): "We use a probe mask MIP chain so very large probe searches (up to the entire screen) can be performed in O(log r)" (paraphrased from the paper text underlying `screen_probes.hlsl:110–129`).

**Capsaicin** (`screen_probes.hlsl:110–166`):
```hlsl
uint ScreenProbes_FindClosestProbe(in uint2 pos)
{
    for (uint i = 0; i < g_ScreenProbesConstants.probe_mask_mip_count; ++i)
    {
        uint probe = g_ScreenProbes_ProbeMask.Load(int3(pos, i)).x;
        if (probe != kGI1_InvalidId) return probe;
        dims = max(dims >> 1, 1);
        pos = min(pos >> 1, dims - 1);
    }
    return kGI1_InvalidId;
}
```
Plus `gi1.cpp:2559–2586` dispatches `FilterProbeMask` per frame to build the mip chain.

**Ours** (`RadianceCacheCommon.hlsl:91, 115–133`):
```hlsl
static const int PROBE_SEARCH_MAX_RING = 2;
ProbeLookup FindClosestProbe(...)
{
    // Ring 0 + Rings 1..2 — Chebyshev outline
    for (int ring = 1; ring <= PROBE_SEARCH_MAX_RING; ring++) { ... }
    return r;  // r.packed = PROBE_MASK_INVALID if no hit within ring 2
}
```

**Nature of divergence**: Capsaicin can find a substitute probe up to log₂(probe_count) tiles away (~8 tiles for 1080p, the whole screen). Ours bails after radius 2 = 2 probe-tiles = 16 pixels. Under sparse spawning at quarter-spp, ξ_x·ξ_y = 4 probe slots cycle through Halton over 4 frames — the worst-case empty-tile pattern is at most 1 invalid probe per 2×2 quad, so radius-2 rings *should* always find something. **However**, when the reprojection invalidates a contiguous patch (large-camera-rotation disocclusion, off-screen edges, or the side-cache also fails), our search falls off a cliff and `SampleRadianceCache` enters its relaxed-interpolation `else` branch (`RadianceCacheCommon.hlsl:303–311`), tagging the pixel for blur-radius widening but contributing wrong-direction probe SH to its irradiance.

**Impact**: Adds disocclusion-edge noise specifically (radius-2 misses on motion edges). Less catastrophic than D1 but contributes to the visible noise on the curtain-fold edges of the GISponza capture.

**Resolution**: Add a `FilterProbeMask` dispatch + a multi-mip `RTexture2D<uint>` for the probe mask, then port Capsaicin's MIP-cascade `FindClosestProbe`. Or, cheaper: raise `PROBE_SEARCH_MAX_RING` to 4 or 8 — each step costs (2k+1)² - (2k-1)² = 8k texels read, so radius 4 = 64 reads, still cheap.

---

### D3 — No sub-pixel jitter at per-pixel interpolation site

**Capsaicin** (`gi1.comp:1540–1553`):
```hlsl
BlueNoiseSampler blue_noise_sampler = MakeBlueNoiseSampler(did, g_FrameIndex, 1);
float2 s        = blue_noise_sampler.rand2();
int2   jitter   = (2.0f * s - 1.0f) * g_ScreenProbesConstants.probe_spawn_tile_size;
uint2  new_pos  = clamp(int2(did) + jitter, 0, int2(g_BufferDimensions) - 1);
// only apply jitter if (new_world_pos − world_pos) projects into the original pixel plane
if (abs(dot(new_world_pos - world_pos, normal)) < 0.5f * cell_size)
    pos = new_pos;
```
Then the 4-corner probe lookup uses `pos` (jittered), so adjacent pixels in the same plane sample probes from a ±16-pixel jittered window. The ReprojectGI temporal accumulator (gi1.comp:3997) integrates ~16 frames of these jittered reads, which combined with the spatial bilateral averages out the per-probe SH wobble.

**Ours** (`RadianceCacheCommon.hlsl:250`):
```hlsl
float2 probeUV    = float2(screenCoord) / float2(RADIANCE_CACHE_TILE_SIZE) - 0.5;
int2   probeFloor = int2(floor(probeUV));
// ... 4-corner search with deterministic targetTL / TR / BL / BR
```
Every screen pixel reads the same 4 probes every frame. There's no per-frame, per-pixel variation in *which* probes are consulted — only the underlying SH coefficients change. Per-frame SH wobble (from D1) becomes the per-pixel wobble.

**Impact**: This is the proximate cause of the *spatial* speckle pattern — adjacent pixels see the same wobble at the same time, so the noise has a 1-pixel-coherent footprint rather than the spatially-decorrelated noise that the bilateral filter is built to suppress. The bilateral kernel rejects it as "this looks like signal" because it's coherent.

**Resolution**: Port Capsaicin's blue-noise jitter into `SampleRadianceCache`. Requires the engine's blue-noise sampler to be available at the GIDenoise binding site (it isn't yet on this path — `RadianceCacheRayGen.hlsl:31` uses an ad-hoc R2 sequence rather than the engine-wide blue noise). Cheap workaround: a Halton(2)/Halton(3) jitter using `g_Frame.frameIndex` is also paper-acceptable and avoids adding the blue-noise binding.

---

### D4 — Closest-hit reads previous-frame LightPass output for on-screen bounces (energy double-count risk)

**Capsaicin** (`gi1.comp:2517–2529` ResolveCells + closest-hit hash-grid reads at `gi1.comp:1962, 2377, 2381`): secondary path vertices read `HashGridCache_FilteredRadianceDirect/Indirect` — *direct* lighting only at MIP-cascade level, then the multibounce path (gi1.comp:2891–2895) adds indirect on top. Critically, the hash-grid stores **direct lighting evaluated by `LightSampler::sample`**, not the resolved indirect-included LightPass output. There's no path where bounce contribution comes from a buffer that already includes integrated GI.

**Ours** (`RadianceCacheClosestHit.hlsl:48–58`):
```hlsl
if (withinBounds)
{
    // Read the Lambertian diffuse outgoing radiance (albedo * E / PI) from the previous frame.
    hitRadiance = in_LightPassOutgoingLuminance.Load(int3(prevScreenCoord, 0)).rgb;
}
```
`in_LightPassOutgoingLuminance` is the previous frame's `out_lightPassRT0` — which `lightPass.comp:139–143` set to `l_DirectLuminance + l_IndirectLuminance`. So our bounce contribution carries last frame's GI back into this frame's GI input.

**Impact**: This is structurally a **path-tracer feedback loop** (energy-conserving in the limit but slow to converge), not the GI-1.0 deterministic two-level cache the paper specifies. It introduces:
- Multiplicative noise: variance in last frame's GI propagates into this frame's bounce reads with no decorrelation.
- Frame-coupled brightness banding when last frame's GI was wrong (e.g. relaxed-interpolation tiles): the wrong value enters this frame's screen-probe radiance and re-projects into the SH.
- Energy growth in resonant geometry (two facing walls of bright albedo) — bounded by the 0.1 EMA at `RadianceCacheIntegration.comp:152` but visible as drift over hundreds of frames.

There **is** an off-screen path that walks `in_WorldTileGrid` (`RadianceCacheClosestHit.hlsl:60–95`) that's structurally similar to Capsaicin's hash-grid reads, but the on-screen path dominates in interior scenes like GISponza where most rays hit on-screen geometry.

**Resolution**: Off-strategy for the noise-gap deliverable (this affects bias more than per-frame variance), but flag for follow-up — the on-screen path should consult a **direct-light-only buffer** (not RT0 = direct + indirect). Either store a direct-only-luminance LightPass output, or compute direct lighting inline at the hit point using the same path Capsaicin uses (`LightSampler` + BRDF eval).

---

### D5 — Radiance backup gate too restrictive; SH projection sees zero radiance in untraced cells

**Capsaicin** (`gi1.comp:1213–1232`):
```hlsl
// LDS reduction over 64 cells producing total_radiance and total sample_count
float4 total_radiance   = lds_ScreenProbes_RadianceBackup[64 - 1];
float3 radiance         = total_radiance.xyz / max(total_radiance.w, 1.0f);
float  empty_cell_count = (probe_size² - total_radiance.w);
lds_ScreenProbes_RadianceBackup[0] = float4(radiance / max(empty_cell_count, 1.0f), MAX_HIT_DISTANCE);
// then per cell:
if (sample_count > 0) radiance /= sample_count;
else                  radiance = lds_ScreenProbes_RadianceBackup[0];  // <-- backup fills empty cells
```
**Always on**. Every cell with no traced sample this frame gets the per-probe average of the cells that did get samples, divided by the count of empty cells (so the total radiance into the probe is conserved).

**Ours** (`RadianceCacheIntegration.comp:64–94`):
```hlsl
const float COVERAGE_ACTIVATION_THRESHOLD = 0.9;
float coverage = gs_avgCount[0] / float(THREAD_COUNT);
float3 tracedAvg = ...;
if (!tracedCell && coverage > COVERAGE_ACTIVATION_THRESHOLD)
    radiance = tracedAvg;
```
Plus the comment "our sparse-spawning configuration (1 ray per spawn tile per frame) hits only ~20% coverage for most of the scene, and filling 80% of a probe with an avg-of-a-dozen-cells doubles SH DC and produces strong per-probe brightness banding."

**Nature of divergence**: The 0.9 gate was added because we hit the backup-too-aggressive failure mode at 1 ray/probe density (the comment is honest about this). But the gate is a workaround, not a fix — at 1 ray/probe we get **no backup at all** (coverage ≪ 0.9), so SH projection sees 1 traced cell × radiance + 63 zero cells. The DC coefficient `Y00` is then ~radiance / 64 — biased low by 64×. The directional bands Y11/Y1_1/Y10 receive `radiance × Y_band(direction_of_traced_cell)`, which is "correct" for *that direction* but with no input from the other 7/8 of the hemisphere.

The actual paper-faithful answer is:
- At Capsaicin's 64-rays/probe density, coverage is ~63/64 = 0.98 most frames and the backup fills the 1-3 cells whose rays missed.
- At our 1-ray/probe density, coverage is 1/64 = 0.016 and the backup *would* dominate. The comment's concern is correct: backing-up from a single sample is just spreading the single sample's radiance across all cells, which is wrong directionally.

**Impact under D1 fix**: Once `NUM_SAMPLES` rises toward 64, lower the gate to ~0.5 or just remove it. The per-probe average backup is correct *when there are enough samples to characterize the hemisphere* — Capsaicin's design assumes 64 cells × 1 frame, our path needs 64 cells × 1 frame OR an equivalent EMA: either trace enough rays per frame to characterize, or accept that the SH projection is integrating over multiple frames of partial coverage and add a *temporal* sample-count to the backup (cells with non-zero history weight contribute their stored value; only truly-cold cells get the per-probe average).

**Resolution**: After D1 fix, change `COVERAGE_ACTIVATION_THRESHOLD` to 0.5 (Capsaicin equivalent: backup is unconditional). Before D1 fix, the better intermediate is to gate per-cell on `weight > 0` from the previous frame's atlas (treat any cell that's *ever* been traced this temporal window as "covered") and apply backup only to never-touched cells.

---

### D6 — `kGIDenoiser_MaxBlurMask` is 16.0 in our code, 8.0 in Capsaicin

**Capsaicin** (`gi_denoiser.hlsl:26`): `#define kGIDenoiser_MaxBlurMask 8.0f`
This is used both as the cap on `lighting.w` (max history N before the emission goes to 0) and the multiplier from normalised mask-texture value back to pixel-radius in `GIDenoiser_GetBlurRadius` (gi_denoiser.hlsl:46).

**Ours**: `GIDenoise.comp:101` and `GIFilterCommon.hlsl:34` both `static const float kGIDenoiser_MaxBlurMask = 16.0;`.

**Nature of divergence**: `gi_denoiser.hlsl:46` says `blur_mask * MAX_BLUR_MASK` — at our value, max blur radius per axis is `min(blur_mask, 1.0) * 16 = 16` pixels. Capsaicin's max is 8 pixels. **We have a 2× larger blur radius cap** — should make our denoiser *more* aggressive, not less. So this divergence isn't the noise cause; if anything it's compensating slightly for the upstream noise. But it pairs with the dynamic-cap divergence: our `MAX_CAP_FACTOR · MAX_BLUR_MASK = 8 × 16 = 128` history samples max, vs Capsaicin's `8 × 8 = 64`. So our temporal accumulator can hold 2× as many samples — this *should* further smooth, but at our 1-ray/probe input rate, even 128 samples integrating 1 noisy SH coefficient set per frame can't catch up if the noise is fixed-pattern (D3) or DC-biased (D5).

The in-source comment at `GIDenoise.comp:108–112` already flagged this divergence ("In our engine the per-pixel SH evaluation is significantly less noisy than Capsaicin's per-ray integration, so this cap rarely binds in practice — flagged for CL1 follow-up"). The assumption that ours is "less noisy" was wrong — the upstream is *more* noisy because of D1.

**Resolution**: Once D1 is fixed and the upstream actually is "less noisy than Capsaicin's per-ray integration" (which a 64-rays-per-probe screen-probe with proper SH projection genuinely is), keep the 16.0 value or drop to 8.0 to match the paper. While D1 is unfixed, drop to 8.0 to align with Capsaicin's reference behaviour and stop the larger blur from cooking edges.

---

### D7 — Reprojection topology: sub-pixel motion-vector reproject vs probe-tile reproject

**Capsaicin** (`gi1.comp:3997–4080`): `ReprojectGI` runs **per-pixel** (1 thread per screen pixel, no LDS reduction). For each pixel, sample motion vector → previous_uv → 3×3 gather around previous_uv with `kOneOverSqrtOfTwo` falloff. Dynamic cap, EMA on color delta. **The screen-probe path's reprojection (`ReprojectScreenProbes`, gi1.comp:293–555) is a separate dispatch** that operates on the probe atlas, not the per-pixel irradiance.

**Ours**: We have **two reprojections**:
1. `RadianceCacheReprojection.comp` runs per-pixel-of-the-screen but does a **probe-tile-level reprojection** — it picks the best of 64 candidate threads in an 8×8 group via `InterlockedMin` and only that thread writes the entire probe tile's reprojected SH atlas. This corresponds (in shape) to Capsaicin's `ReprojectScreenProbes`, NOT `ReprojectGI`.
2. `GIDenoise.comp` runs per-pixel and does the per-pixel temporal accumulator with the 3×3 gather — corresponds to Capsaicin's `ReprojectGI`. Faithfully implemented (mostly — see D6).

**Nature of divergence**: The probe-atlas reprojection in (1) is structurally different from Capsaicin's `ReprojectScreenProbes` (gi1.comp:293–555 is much more complex, doing both LRU cache lookup *and* a per-probe radiance reprojection over a 4-tap blue-noise pattern). Our reprojection's `TemporalFilter` (`RadianceCacheReprojection.comp:98–106`) does:
```hlsl
float blendRate = lerp(0.95, 0.1, confidence);
float4 result = lerp(historySample, currentSample, blendRate);
```
At high confidence (= valid reprojection), `blendRate ≈ 0.1`, so 90% history weight. At low confidence, 95% current. This is fine *in shape* but means the probe atlas integrates ~10 frames of history per cell. Combined with D1, this means each cell's value is ~10-frame EMA over (1 cell-fresh frame + 9 cell-stale frames) = noise gets ~3× attenuation, not enough to overcome 1-ray-per-frame variance.

**Impact**: The stage exists and works; the issue is that no temporal scheme can compensate for an under-budgeted ray count.

**Resolution**: After D1 fix, reduce the probe-atlas reprojection blend rate (currently 0.1) to ~0.05 since each frame now contributes 64× more information. Or keep as-is and let GIDenoise do most of the smoothing — both are paper-faithful.

---

## Faithful entries (representative — confirms the audit is biased toward divergence)

The temporal accumulator (`GIDenoise.comp:136–330`) and the spatial bilateral (`GIFilterCommon.hlsl`) are line-for-line ports of Capsaicin's `ReprojectGI` and `FilterGI`, with the noted DIVERGENT exceptions in D6. Reading the two side by side, the only variable-name differences and the use of world-space position vs depth-buffer-toLinearDepth are functionally equivalent:

```
Capsaicin gi1.comp:4083   ←→  Ours GIDenoise.comp:289
Capsaicin gi1.comp:4087   ←→  Ours GIDenoise.comp:294
Capsaicin gi1.comp:4092   ←→  Ours GIDenoise.comp:309-311
Capsaicin gi1.comp:4096   ←→  Ours GIDenoise.comp:317-320
Capsaicin gi1.comp:4099   ←→  Ours GIDenoise.comp:327
Capsaicin gi1.comp:4134   ←→  Ours GIFilterCommon.hlsl:142
Capsaicin gi1.comp:4136   ←→  Ours GIFilterCommon.hlsl:144
Capsaicin gi1.comp:4138   ←→  Ours GIFilterCommon.hlsl:149
Capsaicin gi1.comp:4145   ←→  Ours GIFilterCommon.hlsl:159
Capsaicin gi1.comp:4150   ←→  Ours GIFilterCommon.hlsl:167
```
The denoise/filter pair is **not** the source of the noise gap.

The probe-space bilateral (`RadianceCacheFilterHorizontal.comp`, `RadianceCacheFilterVertical.comp`) similarly mirrors `FilterScreenProbes` (gi1.comp:1357–1443). The 6-tap radius, parallax-corrected angular-error gate, plane-distance cell-size gate, and depth-ratio bilateral weight are all line-for-line ports.

---

## Open questions (require engine measurement to disambiguate)

1. **What's the actual per-frame coverage at our current density?** Add a debug counter to `RadianceCacheIntegration.comp` summing `gs_avgCount[0]` over all probes per frame, dump via existing readback path. Hypothesis: ~1.5 % coverage at 1080p quarter-spp, 1 ray/probe — but worth confirming since D5 resolution depends on the actual number.
2. **Is the per-pixel SH speckle dominated by cell-to-cell variance within a probe (DC-bias from D5) or by tap-to-tap variance across probes (D3 fixed-pattern)?** A RenderDoc capture comparing two sequential frames' per-pixel `SampleRadianceCache` output — pixel-difference — would split these. Hypothesis: D3-pattern dominates because per-pixel variance is high and frame-to-frame the per-probe SH only changes by the 0.1 EMA; the spatial pattern stays put even as the magnitude changes.
3. **How much would D1 alone close the gap?** Ramp `NUM_SAMPLES` from 1 → 4 → 16 → 64 in single-step PRs and measure. Likely 16 closes 80% of the visible gap based on the convergence math (each frame fills 16/64 = 25% of cells, 4-frame EMA at the integration step gets to ~95% coverage steady-state).
4. **What's the bus / VRAM cost of 64× more rays per frame?** Capsaicin runs this on RDNA3-class hardware at 60+ FPS; our budget should support it on similar hardware but worth a profiling check before committing.

---

## Recommended next moves (ranked by expected impact / effort)

| # | Action | Expected impact on noise | Effort estimate |
|---|--------|--------------------------|-----------------|
| 1 | **Raise `NUM_SAMPLES` from 1 to 16 in `RadianceCacheRayGen.hlsl:306`**, restructure RayGen so the 16 rays use a stratified low-discrepancy sequence over the octahedral hemisphere. Don't change the dispatch shape yet. | Closes 60–80 % of the gap | 2–3 hours including capture re-validation |
| 2 | **Restructure dispatch to `(W/16, H/16, 64)` with one thread = one cell** (Capsaicin's exact shape). Move per-probe state (CDF builds, ImportanceSampleFromCDF) to LDS shared across the 64 threads of a probe. Closes the rest of D1. | Closes the remaining 20 % of D1 | 1–2 days |
| 3 | **Drop `kGIDenoiser_MaxBlurMask` from 16 → 8** (D6) in both `GIDenoise.comp:101` and `GIFilterCommon.hlsl:34` and `GIDenoise.comp:113` (`MaxCapFactor` derived). Aligns with Capsaicin reference and prevents over-blurring once the upstream noise is reduced. | Tightens edge preservation post-D1 fix | 15 minutes |
| 4 | **Lower `COVERAGE_ACTIVATION_THRESHOLD` from 0.9 to 0.5** (D5) once D1 lands. Current value masks the backup at all realistic densities. | Removes per-probe brightness banding from cold cells | 15 minutes + capture re-validation |
| 5 | **Add sub-pixel jitter to `SampleRadianceCache`** (D3). Halton(2)/Halton(3) on `g_Frame.frameIndex` is sufficient — no blue-noise binding needed. Add the world-space-plane gate from Capsaicin gi1.comp:1550. | Eliminates fixed-pattern speckle, lets the bilateral filter actually do its job | 1 hour |
| 6 | **Build `FilterProbeMask` mip chain + port Capsaicin's MIP-cascade `FindClosestProbe`** (D2). Or interim: raise `PROBE_SEARCH_MAX_RING` from 2 to 4. | Closes residual disocclusion-edge noise | 30 min (interim) / 2–3 hours (full) |
| 7 | **Investigate D4 (closest-hit reads previous-frame indirect)** — confirm via RenderDoc whether the energy double-count is visible as drift over hundreds of frames in GISponza, then plan a direct-only LightPass output buffer. | Bias correction, not noise. Defer until #1–6 done. | Investigation 1 hour, fix 4–6 hours |

**Single highest-leverage move: #1**. Predict the visible noise drops by half before any other change.

---

## Not audited

- ReSTIR (`world_space_restir.hlsl`) — we don't implement it; the GI-only comparison doesn't need it.
- Glossy reflections + atrous denoiser — we don't implement glossy GI; out of scope.
- Multibounce path (gi1.cpp:2363–2515) — we don't implement explicit multibounce; our approximation is the previous-frame LightPass-output read in closest-hit (D4).
- Hash-grid bucket-occupancy debug stats and the `ClearBucketOverflow*` chain (gi1.cpp:2227–3030) — debug-only.
- Probe cached-tile LRU (gi1.cpp:2274–2292, 2544–2557) — quality-of-life optimization for multibounce; doesn't affect single-bounce noise floor.
- Visual side-by-side with Capsaicin GISponza screenshots — not present in `Build/reference/Capsaicin/` (only source / docs / assets / third_party present, no rendered references shipped). Paper Figure 18 and the GISponza shots throughout the PDF are the visual reference; PDF rendering tooling failed in this environment so this audit relied on the prior audit's quoted excerpts and the user's verbal characterisation of the paper's noise floor.
- Order-of-operations ordering — confirmed correct via `ExampleRenderingClient.cpp:323–330` (Reproject → Raytracing → FilterH → FilterV → Integration → GIDenoise → GIFilterH → GIFilterV → LightPass), matches Capsaicin's logical order modulo the missing stages noted in the inventory table.
