# Radiance Cache — GI-1.0 Alignment Roadmap

Tracking document for aligning the engine's radiance cache to the AMD **GI-1.0** paper (Boissé et al., AMD, Oct 2022 — *A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination*). Paper PDF: `Build/GI1_0.pdf` (fetched once, not committed).

Source-of-truth for gaps, decomposition, order of work, and status. Update as sub-projects land. Parent: `TASK-6`.

---

## Paper pipeline (summary)

Two-level cache:

1. **Screen cache** — 8×8 octahedral probes; one probe per 8×8 screen tile per frame, spawned at a Halton-jittered pixel; incoming radiance; reprojected, ray-guided, blended with a shadow-preserving temporal hysteresis; filtered with a mask-MIP-aware separable bilateral; projected to SH.
2. **World cache** — spatial hash grid addressed by `(quantized pos, quantized dir, short-ray bit)`, two-level tiled layout with in-tile MIP prefilter; caches outgoing radiance at secondary path vertices.

Per-pixel irradiance = 4 neighbor probes + edge-aware weights + SH·cosine. Optional: HBIL short-range SS GI + world-space ReSTIR light sampling.

---

## Gap matrix (paper → current)

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

Current passes disabled since commit `1734baaa` (TASK-60 workaround). Foundational issues (#7, #9) make the filter incorrect, which is the direct cause of the "strong per-pixel noise" referenced there.

---

## Sub-project decomposition

Sized as a CL each. Commit references this doc section.

### [F] Foundation — cell size + mask MIP  *(covers #7, #9)*

Unify `adaptive_cell_size` in a shared HLSL header; add a new `RadianceCacheProbeMaskPass` that writes a 1-texel-per-tile sentinel texture (sub-tile pixel coords or `INVALID`) and a MIP chain where each level keeps the first valid probe in the 2×2 upper level. Replace every current `ProbePosition.w > 0` check with `FindClosestProbe` against the mask MIP. Reprojection/filter start using the unified cell size.

### [S1] Screen-cache convergence *(covers #1, #3, #4, #5, #6)*

Sparse spawning: drive spawn via `upscaleFactor` → one probe per `8·Ux × 8·Uy` tile per frame, pixel chosen by Halton(2,3). Add empty/override-tile queues and the patch kernel (Algo 2). Rewrite ray-guiding CDF over 3×3 probe neighborhood with parallax correction (alpha channel = ray travel distance). Replace RayGen EMA with Algo 3 biased hysteresis. Add radiance-average backup for untraced cells.

### [S2] Screen-cache robustness *(covers #8, #10)*

Persistent LRU side cache for evicted probes: separate 2D texture for radiance + 128-bit `(pos, packed_normal)` tile-list scatter. Rewrite the separable filter to use `find_closest_probe` on the mask MIP, apply parallax correction, enforce the 50-unit-relative direction threshold.

### [I] Irradiance evaluation *(covers #11, #12, #13)*

LightPass rewrite: per-pixel jittered lookup → 4 probe neighbors → edge-aware weights (depth + normal) → SH·cosine evaluation → relaxed-interpolation fallback with denoiser hint flagged in alpha. Upgrade SH storage to bands 0–2 (9 coefs → 3×3 atlas-per-probe, or packed). Add spatiotemporal GI denoiser pass (adaptive spatial radius from history count, disocclusion mask dilation).

### [W] World-cache overhaul *(covers #14, #15, #16, #17)*

Replace the flat `WorldProbeGrid` RWStructuredBuffer with a two-level hash: bucket table + fingerprint-addressed tile entries; tiles hold 8×8 cells with in-place MIP prefilter, 2D-projected along the major axis of the outgoing direction. Descriptor = `(quant(pos, level), quant(dir), short_ray_bit)`. Decay-based eviction; per-vertex cache-the-index pattern for single-atomic lookups.

### [L] Light sampling *(covers #18)* — optional

Scene-AABB voxel light grid storing top-N important lights per cell (weight = relative luminance of deposited light). World-space ReSTIR reservoir grid (same hashing scheme as world cache but pos-only descriptor; normal stored in side stream for bilateral reuse). Single temporal reuse pass, single visibility ray per reservoir after resampling.

### [X] Short-range screen-space GI *(covers #19)* — optional

HBIL-style horizon-based bent cone + AO mask; multiply bent cone by clamped cosine before integrating incoming radiance; fill small-scale occlusion and color bleeding missed by the grid.

---

## Order

```
[F]  ──► [S1] ──► [S2] ──► [I] ──► [X?]
          │                 ▲
          └─────────────────┘ (via radiance feedback)

[W]  (independent of screen path; parallelizable)
[L]  feeds [W]; run after [W] stabilizes
```

**Ground rule:** no sub-project merges until the CL re-enables (or keeps enabled) the GI passes and produces a RenderDoc frame-8 GISponza capture that visually validates the slice. Baseline capture before [F] goes into the [F] commit for comparison.

---

## Status

| ID | Slice | Status | Commit(s) |
|---|---|---|---|
| F | Foundation — cell size + mask MIP | ☐ | |
| S1 | Screen-cache convergence | ☐ | |
| S2 | Screen-cache robustness | ☐ | |
| I | Irradiance evaluation | ☐ | |
| W | World-cache overhaul | ☐ | |
| L | Light sampling (opt) | ☐ | |
| X | Short-range SS GI (opt) | ☐ | |

Backlog subtasks filed per slice; each updates this table on landing.
