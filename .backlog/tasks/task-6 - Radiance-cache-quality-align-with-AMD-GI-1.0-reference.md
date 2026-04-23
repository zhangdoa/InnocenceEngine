---
id: TASK-6
title: 'Radiance cache quality: align with AMD GI 1.0 reference'
status: In Progress
assignee: []
created_date: '2026-04-07 09:26'
updated_date: '2026-04-23 13:10'
labels:
  - rendering
  - GI
  - long-term
dependencies: []
references:
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Source/Shaders/HLSL/RadianceCacheReprojection.comp
  - Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp
  - Source/Shaders/HLSL/RadianceCacheIntegration.comp
  - Source/Shaders/HLSL/lightPass.comp
  - Build/GI1_0.pdf
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Align the engine's radiance cache to the AMD **GI-1.0** paper (Boissé et al., AMD, Oct 2022 — *A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination*). Paper PDF: `Build/GI1_0.pdf` (fetched once, not committed).

This is the umbrella; Implementation Notes below track gap matrix, sub-project decomposition, priority order, status, and landed-slice commentary. Sub-slices are filed as their own tasks (TASK-114..118 and successors); each updates the Status table here on landing.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->

### Paper pipeline (summary)

Two-level cache:

1. **Screen cache** — 8×8 octahedral probes; one probe per 8×8 screen tile per frame, spawned at a Halton-jittered pixel; incoming radiance; reprojected, ray-guided, blended with a shadow-preserving temporal hysteresis; filtered with a mask-MIP-aware separable bilateral; projected to SH.
2. **World cache** — spatial hash grid addressed by `(quantized pos, quantized dir, short-ray bit)`, two-level tiled layout with in-tile MIP prefilter; caches outgoing radiance at secondary path vertices.

Per-pixel irradiance = 4 neighbor probes + edge-aware weights + SH·cosine. Optional: HBIL short-range SS GI + world-space ReSTIR light sampling.

### Gap matrix (paper → current)

Severity: **M** = major (quality-critical) · **m** = medium · **o** = optional.

| # | Paper stage | Current | Sev |
|---|---|---|---|
| 1 | Sparse probe spawning via temporal upscale | Every tile every frame; `upscaleFactor = (1,1)` | M |
| 2 | Halton jitter inside spawn tile | R2 | o |
| 3 | Adaptive sampling (Algo 2): empty/override-tile queues, ray stealing | — | M |
| 4 | Ray guiding: 3×3 tile neighborhood + parallax correction (travel distance in α) | 1-tile CDF, no parallax | M |
| 5 | Biased shadow-preserving temporal hysteresis (Algo 3) | Simple EMA + firefly clamp | M |
| 6 | Radiance-average backup for untraced cells | — | m |
| 7 | Probe mask sentinel texture + MIP chain | `in_ProbePosition.w>0` check — wrong structure | M |
| 8 | Separable 7×7 bilateral filter w/ mask-MIP search + parallax + 50-unit dir check (Algo 5) | 7×7 separable present, no mask MIP, no parallax | M |
| 9 | Unified `adaptive_cell_size = depth·tan(fov·8/max_dim)/√2` across reprojection/sample/filter (Algo 6) | Partial in filter; reprojection uses hardcoded `MAX_PLANE_DISTANCE = 0.5` | M |
| 10 | LRU persistent side cache for evicted probes (thin-geometry stability) | — | M |
| 11 | Per-pixel 4-probe interpolation w/ jitter + plane cancel + relaxed fallback | LightPass reads SH directly at probe anchor | M |
| 12 | SH bands 0–2 (9 coefs) | Bands 0–1 (4 coefs) | m |
| 13 | Spatiotemporal GI denoiser (disocclusion mask → dilated blur mask) | — | M |
| 14 | World hash: bucket + fingerprint + linear probing, two hash fns | Single LCG hash, no collision handling | M |
| 15 | World descriptor: (quant pos, **quant dir**, **short-ray bit**) | Pos only | M |
| 16 | Two-level world hash: 8×8 tiles w/ in-tile MIP prefilter, 2D-projected along major axis | Flat | M |
| 17 | Decay-based eviction + cached-index lookup | Write-only | m |
| 18 | Light grid + world-space ReSTIR reservoirs | — | o |
| 19 | HBIL short-range SS GI (bent cone × cos × radiance) | — | o |

Foundational issues (#7, #9) made the filter incorrect, which was the direct cause of the "strong per-pixel noise" referenced in the TASK-60 workaround (commit `1734baaa`, now lifted).

### Sub-project decomposition

Sized as a CL each.

**[F] Foundation — cell size + mask MIP** *(covers #7, #9)* — Unify `adaptive_cell_size` in a shared HLSL header; add a new `RadianceCacheProbeMaskPass` that writes a 1-texel-per-tile sentinel texture (sub-tile pixel coords or `INVALID`) and a MIP chain where each level keeps the first valid probe in the 2×2 upper level. Replace every current `ProbePosition.w > 0` check with `FindClosestProbe` against the mask MIP. Reprojection/filter start using the unified cell size.

**[S1] Screen-cache convergence** *(covers #1, #3, #4, #5, #6)* — Sparse spawning: drive spawn via `upscaleFactor` → one probe per `8·Ux × 8·Uy` tile per frame, pixel chosen by Halton(2,3). Add empty/override-tile queues and the patch kernel (Algo 2). Rewrite ray-guiding CDF over 3×3 probe neighborhood with parallax correction (alpha channel = ray travel distance). Replace RayGen EMA with Algo 3 biased hysteresis. Add radiance-average backup for untraced cells.

**[S2] Screen-cache robustness** *(covers #8, #10)* — Persistent LRU side cache for evicted probes: separate 2D texture for radiance + 128-bit `(pos, packed_normal)` tile-list scatter. Rewrite the separable filter to use `find_closest_probe` on the mask MIP, apply parallax correction, enforce the 50-unit-relative direction threshold.

**[I] Irradiance evaluation** *(covers #11, #12, #13)* — LightPass rewrite: per-pixel jittered lookup → 4 probe neighbors → edge-aware weights (depth + normal) → SH·cosine evaluation → relaxed-interpolation fallback with denoiser hint flagged in alpha. Upgrade SH storage to bands 0–2 (9 coefs → 3×3 atlas-per-probe, or packed). Add spatiotemporal GI denoiser pass (adaptive spatial radius from history count, disocclusion mask dilation).

**[W] World-cache overhaul** *(covers #14, #15, #16, #17)* — Replace the flat `WorldProbeGrid` RWStructuredBuffer with a two-level hash: bucket table + fingerprint-addressed tile entries; tiles hold 8×8 cells with in-place MIP prefilter, 2D-projected along the major axis of the outgoing direction. Descriptor = `(quant(pos, level), quant(dir), short_ray_bit)`. Decay-based eviction; per-vertex cache-the-index pattern for single-atomic lookups.

**[L] Light sampling** *(covers #18)* — optional. Scene-AABB voxel light grid storing top-N important lights per cell (weight = relative luminance of deposited light). World-space ReSTIR reservoir grid (same hashing scheme as world cache but pos-only descriptor; normal stored in side stream for bilateral reuse). Single temporal reuse pass, single visibility ray per reservoir after resampling.

**[X] Short-range screen-space GI** *(covers #19)* — optional. HBIL-style horizon-based bent cone + AO mask; multiply bent cone by clamped cosine before integrating incoming radiance; fill small-scale occlusion and color bleeding missed by the grid.

### Order

```
[F]  ──► [S1] ──► [S2] ──► [I] ──► [X?]
          │                 ▲
          └─────────────────┘ (via radiance feedback)

[W]  (independent of screen path; parallelizable)
[L]  feeds [W]; run after [W] stabilizes
```

**Ground rule:** no sub-project merges until the CL re-enables (or keeps enabled) the GI passes and produces a RenderDoc frame-8 GISponza capture that visually validates the slice. Baseline capture before [F] went into the [F] commit for comparison.

### Remaining work — priority order

Each piece is session-sized — take the top item, design, implement, capture, commit; then pick the new top. Every landing CL updates the Status table below AND removes/re-orders the entry here.

[I.3e] was split into four sub-slices on extraction day — see Status table for [I.3e.1–4] detail.

[I.3e] closed — SVGF pipeline (temporal + 3× à-trous + disocclusion dilation) is feature-complete. Remaining items are independent radiance-cache improvements.

1. **[W.3b] Two-level tiled world-hash + MIP prefilter + cache-the-index** — the structural half of the original [W.3]; eviction-of-stale-slots already shipped in [W.3a]
2. **[S2.2] LRU side cache** — thin-geometry stability
3. **[S1.5c-override] Override-tile ray stealing** — follow-up to [S1.5c]: route extra rays from well-reprojected tiles to high-variance ones (currently ray budget is constant at 1 spawn / spawn tile)
4. **[S1.5b-mip-chain]** Real mask-MIP-chain walk in `FindClosestProbe` — current implementation is a direct Chebyshev ring scan. Paper's O(log r) MIP-chain walk only pays off if PROBE_SEARCH_MAX_RING grows much larger; defer until that's the case.

Optional (not in priority order, scheduled separately): [S1.4], [S1.5b], [L], [X].

### Capture / test protocol

Async-load timing: GISponza's load request fires at frame 5 but the geometry/textures stream in over ~30–40 more frames. For a fully-loaded scene, capture with `-total_frames 100 -capture_frame 60`. Frames before ~50 render a black G-buffer and misleading thumbnails.

```
rm -f C:/GitRepo/InnocenceEngine/Build/captures/frame_capture.rdc
cd C:/GitRepo/InnocenceEngine/Bin && powershell.exe -NoProfile -NonInteractive \
  -Command "(Start-Process -FilePath 'RelWithDebInfo\Main.exe' \
    -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -total_frames 100 -capture_frame 60' \
    -Wait -PassThru -NoNewWindow).ExitCode"
"C:/Program Files/RenderDoc/renderdoccmd.exe" thumb \
  --out="C:/GitRepo/InnocenceEngine/Build/captures/<label>.png" \
  --format=png --max-size=512 \
  "C:/GitRepo/InnocenceEngine/Build/captures/frame_capture.rdc"
```

Save the PNG with a label tied to the CL (e.g. `S1_5_post.png`) so the next CL can compare against it.

### Status

| ID | Slice | Status | Commit(s) |
|---|---|---|---|
| F | Foundation — cell size + mask (MIP chain deferred to [S1.5b]) | ☑ (partial — single-level mask only) | |
| S1.1 | Algorithm 3 biased shadow-preserving temporal hysteresis | ☑ | |
| S1.2 | Ray travel distance in atlas alpha (parallax prep) | ☑ | |
| S1.3 | 3×3 neighbourhood CDF reconstruction with parallax correction | ☑ | |
| S1.4 | Radiance-average backup for untraced cells | ☐ (deferred) | — |
| S1.5 | Sparse spawning (upscale 2×2) + Halton pixel + Reprojection mask invalidation | ☑ | |
| S1.5b | FindClosestProbe widening ring search + filter migration + dead-code cleanup | ☑ | |
| S1.5c | Algorithm 2 ray redistribution (empty-tile redirect; override queue deferred) | ☑ (partial) | |
| S2.1 | Probe-space filter with parallax-correction angular rejection | ☑ | |
| S2.2 | LRU persistent side cache for evicted probes | ☐ (deferred) | |
| I.1 | Edge-aware 4-probe interpolation + relaxed fallback | ☑ | |
| I.2 | SH L2 upgrade (9 coefficients, 3×3 per-probe storage) | ☑ | |
| I.2b | Ramamoorthi-Hanrahan cosine-lobe convolution | ☑ | |
| I.3 | Temporal GI denoiser (inline, motion-reprojected blend) | ☑ | |
| I.3b | Inline 3x3 depth-bilateral on history read | ☑ | |
| I.3c | Inline 5x5 Gaussian bilateral + tighter temporal blend | ☑ | |
| I.3d | Inline spatial-variance-adaptive blend rate | ☑ | |
| I.3e.1 | Extract denoiser into standalone GIDenoisePass (zero-behavior-change prerequisite) | ☑ | |
| I.3e.2 | SVGF temporal-variance-driven blend rate (2nd-moment history) | ☑ | |
| I.3e.3 | A-trous multi-stride spatial filter (3 passes @ stride 1/2/4) | ☑ | |
| I.3e.4 | Disocclusion mask dilation | ☑ | |
| W.1 | World cache: fingerprint hash + linear probing | ☑ | |
| W.2 | World cache: directional descriptor + short-ray bit (leak fix) | ☑ | |
| W.3a | World cache: decay-based eviction (stale-slot reuse) | ☑ | |
| W.3b | World cache: two-level tiled layout + MIP prefilter + cache-the-index | ☐ | |
| L | Light sampling (opt) | ☐ | |
| X | Short-range SS GI (opt) | ☐ | |

### Landed scope — per-slice commentary

#### [F] — Foundation

Shared header `common/RadianceCacheCommon.hlsl` with `AdaptiveCellSize` (Algorithm 6), probe-mask pack/unpack, and `FindClosestProbe`. New `ProbeMask` UAV/SRV wired through RayGen → Filter. Reprojection and the separable filter switched to the unified cell size. Re-enabled the GI passes (TASK-60 disablement lifted).

Deferred to [S1.5b]: the mask MIP chain itself. With dense spawning, every tile is valid so the MIP walk is a no-op; sparse spawning and the MIP chain land together, because the chain only does useful work in the presence of holes. The `FindClosestProbe` API is already MIP-shaped so callers won't change when the levels are populated.

Also in this slice: `common/common.hlsl` picked up include guards — needed because `RadianceCacheCommon.hlsl` transitively pulled `common.hlsl` into translation units that already included it via `RayTracingBindings.hlsl`.

#### [S1.1] — Algorithm 3 temporal hysteresis

Replaces the ad-hoc EMA + relative-variance firefly clamp in RayGen with the paper's single formula (copied from the Capsaicin reference impl): `t` squared-normalised so the history dominates only when the new sample is more than 2× brighter than history (firefly rejection), with immediate adoption of darker samples for shadow preservation. The world-probe grid line got a hardcoded 0.1 Karis EMA — its full treatment lives in [W].

#### [S1.2] — Ray travel distance in atlas alpha

`RayPayload` extended with a `distance` field; `ClosestHit` writes `RayTCurrent()`, `Miss` writes `ray.TMax`. RayGen stores the result in the atlas alpha channel so [S1.3] can parallax-correct reused cells.

#### [S1.3] — 3×3 CDF reconstruction with parallax

`ImportanceSampleFromCDF` iterates a 3×3 tile neighbourhood of reprojected probes, re-aims each neighbour cell's direction through its stored hit distance, and scatters the radiance into the current probe's octahedral CDF in the current probe's tangent frame. Neighbour positions come from the opaque G-buffer at each tile's anchor (avoids the cross-thread race on `in_ProbePosition` that would happen if we read it inside the same RayGen dispatch that writes it). Cell rejection uses the unified `AdaptiveCellSize * 3` threshold.

#### [S1.4] — Radiance-average backup (deferred)

Paper §2.1.4 last paragraph. Only meaningful once per-cell sample counts are tracked (or ray budgets are high enough that "some cells populated, others not" is a frequent case). With the current 1-spp-per-probe configuration and a 64-cell octahedral map, distinguishing "no ray this frame" from "legitimately dark cell" requires tracking state we don't have. Deferred until [S1.5c] lands the ray-redistribution queues, at which point the "untraced cell" set is explicitly known.

#### [S1.5] — Sparse spawning + Halton pixel + Reprojection mask invalidation

Drives spawn via `upscaleFactor = (2, 2)`, cutting ray budget to 1/4; Halton(2)/Halton(3) picks one pixel per 16×16 spawn tile per frame; Reprojection now owns the "this tile is useless" signal, invalidating PROBE_MASK on sky and no-reprojection-possible paths. Successful reprojection leaves the mask alone so the last spawn's mask persists through the rest of the upscale cycle. Mask encoding updated: validity is a bit-31 flag, INVALID = 0, so uninitialised memory reads as invalid without a per-frame clear. RayGen's 3×3 CDF reconstruction now reads each neighbour's sub-pixel from its own mask instead of the CB jitter (each neighbour was spawned at its own Halton offset on its own frame).

#### [S1.5b] — FindClosestProbe widening ring search + filter migration

Landed as a focused refactor rather than a full MIP chain:

- `FindClosestProbe` in `common/RadianceCacheCommon.hlsl` now does an expanding Chebyshev-ring search (rings 1..PROBE_SEARCH_MAX_RING=2) when the requested target tile is invalid. First-hit wins; `r.tileCoord` returns the substitute's position so callers drive geometry-rejection tests from the actual probe used.
- RadianceCacheFilterHorizontal.comp / RadianceCacheFilterVertical.comp migrated: replaced the hardcoded `in_ProbeMask.Load + IsValidProbe` immediate-neighbour check with `FindClosestProbe`. When the requested stride-N neighbour is invalid, the filter now finds a ring-substitute and runs the existing plane/normal/parallax rejection against the substitute's geometry — fills multi-tile disocclusion holes that the spawn-redirect in [S1.5c] can't cover within one frame.
- Deleted the legacy stand-alone `RadianceCacheFilter.comp` — it was superseded by the H/V separable pair at [F]-landing time and had been dead code since. Its local `FindClosestProbe` with a broken "MIP walk" (shifting coordinates on a non-MIP texture) was the original motivation for calling out [S1.5b] in the roadmap.

Why ring-walk instead of MIP chain: paper's Algorithm 4 uses a MIP-chain walk for O(log r) scaling. With PROBE_SEARCH_MAX_RING=2 the direct ring scan loads at most 25 tile-mask texels per call — well within budget and avoids the MIP-chain generation pass. If the search radius needs to grow (e.g. world-space light-leak scenarios), a real MIP chain becomes worthwhile — filed as `[S1.5b-mip-chain]` in the Remaining Work list.

#### [S1.5c] — Algorithm 2 ray redistribution (partial — empty-tile redirect only)

Landed the empty-tile half of paper Algorithm 2 as a shader-only redirect inside RayGen. Each dispatched spawn tile scans the ξ_x·ξ_y probe tiles it owns; if any is `PROBE_MASK_INVALID` (Reprojection flagged it this frame), the Halton-picked sampling pixel gets redirected to that disoccluded tile instead of wherever Halton was rolling. Keeps the ray budget constant (one spawn per spawn tile, same as before) and kills the "disoccluded tile waits up to ξ_x·ξ_y frames for Halton to come around" regression.

Intentionally deferred: the paper's `override_tile` queue — routing EXTRA rays to well-reprojected-but-high-variance tiles by stealing from neighbours. Would need the full infrastructure TASK-6 originally listed (two UAV counters, classify-and-populate compute pass, dispatch-indirect RayGen). Filed as `[S1.5c-override]` in the Remaining Work list above. The empty-tile redirect is the high-value half; override is a refinement.

Implementation keeps the Halton sub-pixel offset inside the redirected probe tile so repeated disocclusions of the same tile still sample varied pixels, not the same pixel every frame.

#### [S2.1] — Probe-space filter with parallax-correction angular rejection

The old filter iterated in screen space, so its stride-1 taps across a tile boundary sampled cells of a *different* probe representing *different* world directions; the filter was mathematically wrong for the atlas layout even though it compiled and ran. New filter iterates in probe space (6 taps at ±{1,2,3} probe-tiles along the blur axis), reads the same cell in each neighbour probe, and uses Capsaicin's parallax rejection: re-aim the stored hit distance through the current probe's position and reject the tap if the reprojected direction differs from the original cell direction by more than ~3.6°.

#### [S2.2] — LRU persistent side cache (deferred)

Paper §2.1.8 describes a side texture + MRU-reorder scheme to keep probes that reprojection would otherwise evict (mostly useful for thin-geometry wobble). Substantial infrastructure (eviction signal from reprojection, a scatter-by-screen-coord pass, MRU reorder, decay-based cleanup). Not on the critical path for the noise we see on large surfaces.

#### [I.1] — Edge-aware 4-probe interpolation

LightPass now queries each of the 4 surrounding probes for validity (mask bit 31), plane distance (edge-aware depth against AdaptiveCellSize), and normal dot-product; each corner's final weight is `screen_bilinear * edge_weight`. If every corner is rejected, falls back to an unweighted screen-bilinear blend ("relaxed interpolation") so edge pixels don't go black. The paper also writes this fallback flag into the output alpha as a denoiser hint — wired in alongside [I.3].

#### [I.2] — SH L2 upgrade

4 → 9 coefficients. Adds the 5 band-2 basis functions (Y_2_2, Y_2_1, Y_20, Y_21, Y_22) that capture quadrant / axis-pair directional variation L1 can't express. Atlas layout 2×2 → 3×3 per probe.

#### [I.2b] — Ramamoorthi-Hanrahan cosine-lobe convolution

`LoadIrradiance` now evaluates the Ramamoorthi-Hanrahan irradiance convolution with per-band cosine-lobe weights (π, 2π/3, π/4) instead of raw `Σ SH[l,m] · Y_lm(n)`. Physically-correct irradiance.

#### [I.3] — Temporal GI denoiser (minimal variant)

Inline in LightPass, ping-pong GI-irradiance history (rgb = irradiance, a = linear depth). Motion-vector reprojection + 5% depth-ratio validity gate + 10% new / 90% history blend. Disocclusion / first-frame / out-of-bounds pixels use 100% raw irradiance (no over-blur on newly-visible surfaces).

#### [I.3b] — Inline 3×3 depth-bilateral on history read

In the same block that performs motion-vector reprojection, each tap in a 3×3 neighbourhood of the previous history is depth-bilateral-weighted (tent kernel × `exp(-20 · |Δdepth| / depth)`). Taps with uninitialised depth or > 5% relative depth mismatch are dropped; if all fail we stay on raw irradiance. Inline read means no UAV race; the blended write-back compounds smoothing across frames without tracking an explicit sample count.

#### [I.3c] — Inline 5×5 Gaussian bilateral + 0.05 blend rate

Swap 3×3 tent (9 taps) for 5×5 Gaussian (25 taps), with the per-tap bilateral depth weight preserved. Temporal blend rate drops from 0.10 to 0.05 so the denoised output leans harder on the now-wider-smoothed history. Disoccluded / first-frame pixels still short-circuit to raw.

#### [I.3d] — Spatial-variance-adaptive blend rate

Track 1st/2nd moments across the existing 5×5 filter support (0 extra texture fetches); derive `rel_var = luma(variance) / luma(mean)^2`; `blend_rate = lerp(0.03, 0.20, saturate(rel_var))`. Low-variance interiors lean into history aggressively (3%/97%), high-variance edges/discontinuities respond quickly (20%/80%). Disoccluded pixels still short-circuit to 100% raw.

#### [I.3e] — Temporal-variance (SVGF) + A-trous

Split into four sub-slices because "move the denoiser + add 2nd-moment history + A-trous multi-stride + disocclusion dilation" in one CL is too big to regress-debug cleanly. [I.3e.1] ships the structural prerequisite (standalone pass, zero behavior change target); subsequent slices evolve the new pass's shader only.

#### [I.3e.1] — Extract denoiser into standalone GIDenoisePass

New `GIDenoisePass` (compute) runs between RadianceCacheIntegrationPass and LightPass. Owns the ping-pong GI history (moved from LightPass); samples the radiance cache SH atlas + applies the inline temporal + 5×5 Gaussian-bilateral denoise + spatial-variance-adaptive blend. LightPass drops the SH-sample + denoise block and reads the denoiser's RGBA output directly. Zero behavior change target: same blend formula, same kernel, same history encoding.

Bindings rebalanced: LightPass loses 5 (RadianceCache SH atlas, ProbePosition/Normal/Mask, GIHistoryPrev, GIHistoryCurrent UAV), regains 1 (GIIrradiance). GIDenoisePass takes 10 (PerFrame CB, G-buffer RT0/RT1/RT3, SH atlas, probe pos/normal/mask, history prev/current).

#### [I.3e.2] — SVGF temporal-variance-driven blend rate

Adds a per-pixel moments ping-pong texture (`r = E[luma], g = E[luma²], b = history-count N, a = unused`) to GIDenoisePass. On each frame the center reprojected tap drives validity (depth ≤ 5% error + non-zero prev-N); on valid reprojection `N = min(prev_N + 1, 32)` and `α = max(1/N, 0.05)` mix prev moments with the current frame's (luma, luma²). Disocclusion resets to `N=1, α=1` (raw sample). The 5×5 Gaussian-bilateral spatial tap is retained for history-color smoothing (will be removed in [I.3e.3] when A-trous replaces it).

Temporal variance `σ² = E[L²] − E[L]²` is exposed in the moments UAV for the [I.3e.3] A-trous pass to use as its luminance edge-stopping weight; this slice does not yet drive a spatial filter radius from it.

Bindings: +2 on GIDenoisePass (in_MomentsPrev at t8, out_MomentsCurrent at u1; layout grows from 10 to 12 entries). LightPass unchanged.

#### [I.3e.3] — SVGF A-trous multi-stride filter

Spatial counterpart to the temporal pass. Three new compute passes (`GIATrous1Pass` / `GIATrous2Pass` / `GIATrous4Pass`) dispatched in cascade with tap offsets scaled by stride 1, 2, 4. Each runs a 5×5 SVGF à-trous stencil (binomial kernel) with joint bilateral edge-stops: depth (≤ 10% error budget + exp fall-off), normal (`max(0, dot)^128`), and luminance (`exp(-|Δluma| / (σ_L · √σ_p + eps))` where σ_p is the 3×3-smoothed temporal variance from the moments texture [I.3e.2] lands).

Three sibling shaders — `GIATrousStride1/2/4.comp` — each defines `ATROUS_STRIDE` and `#include "common/GIATrousCommon.hlsl"` so the tap offsets bake at compile time (avoids a runtime-stride CB and lets the compiler unroll the 5×5 loop). Intermediate textures cascade: temporal → stride-1 → stride-2 → stride-4 → LightPass consumes the stride-4 output.

Pipeline ordering: RadianceCacheIntegrationPass → GIDenoisePass → GIATrous1 → GIATrous2 → GIATrous4 → LightPass. Each A-trous pass waits on the previous one's compute signal before issuing its own graphics→compute barrier.

Also removed the 5×5 Gaussian-bilateral spatial tap from GIDenoisePass — A-trous now owns all spatial filtering, so the temporal pass is a pure single-tap reprojected EMA on the center pixel. This matches the original SVGF separation of temporal vs. spatial.

Limitations for follow-up: variance is not filtered between A-trous iterations (paper's "σ' = σ/16" recipe), we just re-read the moments each iteration. Doing proper variance cascading would need a per-iteration variance output; deferred until it measurably matters.

#### [I.3e.4] — Disocclusion mask dilation

Shader-only refinement to the à-trous edge-stopping. Dilates the SVGF moments' history-count field (min-N over a 3×3 neighbourhood) inside `SampleVariance`, and uses the dilated N to scale σ_L. When a pixel is adjacent to a newly-disoccluded neighbour (N=1), its dilated minN drops below the N_LOW threshold (4) and σ_L gets boosted 4× — the à-trous filter can then blend across the halo instead of luma-rejecting against the noisy single-sample neighbour.

Bundled into `SampleVariance` so every à-trous stride iteration sees the dilated estimate without a separate preprocessing pass. Zero binding changes; all three strides (1, 2, 4) inherit the fix through `common/GIATrousCommon.hlsl`.

#### [W.1] — World cache: fingerprint hash + linear probing

First half of the world-cache rewrite. Fingerprint-addressed entries with open-addressing linear probing; two independent hash functions from Jarzynski–Olano 2020.

#### [W.2] — World cache: directional descriptor + short-ray bit

Extends the [W.1] descriptor from `(quant pos)` to `(quant pos, octant(normal), short-ray bit)` — paper §2.2 Fig. 14 light-leak fix. A floor (+Y normal) and ceiling (−Y normal) at the same voxel now land in different cache slots and don't alias. Short rays (< 1 world unit = near-surface AO/detail bounces) split from long rays (distant skybox / bounced radiance) so neither contaminates the other.

Descriptor lives entirely in the fingerprint: `ComputeProbeHash` and `ComputeProbeFingerprint` both take `(pos, normal, shortRayBit)` and fold them into the PCG3D inputs. No WorldProbe struct change, no CPU-side change. Octant quantisation uses sign-per-axis (8 bins) — coarse enough to collide across smooth surface variation inside the same cell, fine enough to split axis crossings where the leak lives.

Write/read asymmetry (and why this still works):
- WRITE (RayGen): caches at the screen probe's world position with the probe's surface normal and the traced ray's travel distance.
- READ (ClosestHit, off-screen fallback): looks up at the hit position using `-WorldRayDirection()` as a normal proxy and `RayTCurrent()` for the short-ray bit.

For diffuse bounces the proxy matches the actual hit normal closely; for high-angle rays it over-rejects, which is the safer failure mode (no contribution vs leaked contribution).

Stale entries: pre-[W.2] fingerprints left in the hash table from earlier runs persist until their bucket is fully probed over — they occupy slots but can never match a new-scheme fingerprint. Convergence is natural as new data streams in; a scene-reload path would clear the buffer if needed (not implemented here; deferred until measurably needed). *Follow-up:* [W.3a] landed decay-based eviction which turns this from a permanent-ish leak into a bounded one (any stale slot is reclaimed within `WORLD_PROBE_EVICTION_AGE` = 64 frames).

#### [W.3a] — World cache: decay-based eviction (stale-slot reuse)

First half of the original [W.3] plan. The structural two-level tiled rewrite is substantial enough to deserve its own slice (filed as [W.3b]); decay-based eviction works independently of that rewrite and directly addresses the [W.2] leftover-fingerprint problem the previous slice flagged.

Adds `uint lastTouchedFrame` to the `WorldProbe` struct (size 32 → 36 bytes; `m_ElementSize` updated on the C++ side). The RayGen write path:

1. Stamps `lastTouchedFrame = g_Frame.frameIndex` on every insert/update.
2. Treats a slot as eligible for reuse if it's empty (`fingerprint == 0`) OR matches our fingerprint OR is stale (`frameIndex - slot.lastTouchedFrame > WORLD_PROBE_EVICTION_AGE`).

`WORLD_PROBE_EVICTION_AGE = 64` frames (≈1 second at 60 fps). Zero-initialised slots read as "very stale" on frame 0; stale-reclaim kicks in naturally once the frame counter exceeds the threshold.

READ side (ClosestHit) unchanged: a stale slot with a matching fingerprint still serves its cached radiance — better to return dead-reckoned old data that the temporal denoiser will filter away than a hard zero that leaves a dark hole.

The integration test was bumped to 100 frames with reload at frame 70 specifically to cross the eviction threshold: after frame 64 every write should be evaluating the stale path, and after the frame-70 reload any world-cache state from the old scene is reclaimable within one more eviction cycle.

<!-- SECTION:NOTES:END -->
