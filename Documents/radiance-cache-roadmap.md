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

## Capture / test protocol

Async-load timing: GISponza's load request fires at frame 5 but the
geometry/textures stream in over ~30–40 more frames. For a fully-loaded
scene, capture with `-total_frames 100 -capture_frame 60`. Frames before
~50 render a black G-buffer and misleading thumbnails. This is the note
that CLAUDE.md's "good capture range: 6–9" should be updated to reflect;
leave the fix for a separate tooling commit.

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

Save the PNG with a label tied to the CL (e.g. `S1_5_post.png`) so the
next CL can compare against it.

## Status

| ID | Slice | Status | Commit(s) |
|---|---|---|---|
| F | Foundation — cell size + mask (MIP chain deferred to [S1.5]) | ☑ (partial — single-level mask only) | |
| S1.1 | Algorithm 3 biased shadow-preserving temporal hysteresis | ☑ | |
| S1.2 | Ray travel distance in atlas alpha (parallax prep) | ☑ | |
| S1.3 | 3×3 neighbourhood CDF reconstruction with parallax correction | ☑ | |
| S1.4 | Radiance-average backup for untraced cells | ☐ (deferred) | — |
| S1.5 | Sparse spawning (upscale 2×2) + Halton pixel + Reprojection mask invalidation | ☑ | |
| S1.5b | Mask MIP chain + FindClosestProbe MIP walk | ☐ (deferred) | |
| S1.5c | Algorithm 2 ray redistribution (empty/override queues + patch kernel) | ☐ (deferred) | |
| S2.1 | Probe-space filter with parallax-correction angular rejection | ☑ | |
| S2.2 | LRU persistent side cache for evicted probes | ☐ (deferred) | |
| I.1 | Edge-aware 4-probe interpolation + relaxed fallback | ☑ | |
| I.2 | SH L2 upgrade (9 coefficients, 3×3 per-probe storage) | ☑ | |
| I.2b | Ramamoorthi-Hanrahan cosine-lobe convolution | ☑ | |
| I.3 | Temporal GI denoiser (inline, motion-reprojected blend) | ☑ | |
| I.3b | Inline 3x3 depth-bilateral on history read | ☑ | |
| I.3c | Inline 5x5 Gaussian bilateral + tighter temporal blend | ☑ | |
| I.3d | Variance-aware adaptive kernel radius + disocclusion-mask dilation | ☐ | |
| W | World-cache overhaul | ☐ | |
| L | Light sampling (opt) | ☐ | |
| X | Short-range SS GI (opt) | ☐ | |

Backlog subtasks filed per slice; each updates this table on landing.

### [F] landed scope vs. planned

Shipped: shared header `common/RadianceCacheCommon.hlsl` with `AdaptiveCellSize`
(Algorithm 6), probe-mask pack/unpack, and `FindClosestProbe`. New
`ProbeMask` UAV/SRV wired through RayGen → Filter. Reprojection and the
separable filter switched to the unified cell size. Re-enabled the GI
passes (TASK-60 disablement lifted).

Deferred to [S1.5]: the mask MIP chain itself. With the current dense
spawning, every tile is valid so the MIP walk is a no-op; sparse spawning
and the MIP chain land together in [S1.5], because the chain only does
useful work in the presence of holes. The `FindClosestProbe` API is
already MIP-shaped so callers won't change when the levels are populated.

Also in this slice: `common/common.hlsl` picked up include guards — needed
because `RadianceCacheCommon.hlsl` transitively pulled `common.hlsl` into
translation units that already included it via `RayTracingBindings.hlsl`.

### [S1] landed pieces

**[S1.1] Algorithm 3 temporal hysteresis** — replaces the ad-hoc EMA +
relative-variance firefly clamp in RayGen with the paper's single formula
(copied from the Capsaicin reference impl): t squared-normalised so the
history dominates only when the new sample is more than 2× brighter than
history (firefly rejection), with immediate adoption of darker samples
for shadow preservation. The world-probe grid line got a hardcoded 0.1
Karis EMA — its full treatment lives in [W].

**[S1.2] Ray travel distance in atlas alpha** — `RayPayload` extended with
a `distance` field; `ClosestHit` writes `RayTCurrent()`, `Miss` writes
`ray.TMax`. RayGen stores the result in the atlas alpha channel so [S1.3]
can parallax-correct reused cells.

**[S1.3] 3×3 CDF reconstruction with parallax** — `ImportanceSampleFromCDF`
iterates a 3×3 tile neighbourhood of reprojected probes, re-aims each
neighbour cell's direction through its stored hit distance, and scatters
the radiance into the current probe's octahedral CDF in the current
probe's tangent frame. Neighbour positions come from the opaque G-buffer
at each tile's anchor (avoids the cross-thread race on
`in_ProbePosition` that would happen if we read it inside the same RayGen
dispatch that writes it). Cell rejection uses the unified
`AdaptiveCellSize * 3` threshold.

### [S1] deferred pieces

**[S1.4] Radiance-average backup** — paper §2.1.4 last paragraph. Only
meaningful once per-cell sample counts are tracked (or ray budgets are
high enough that "some cells populated, others not" is a frequent case).
With the current 1-spp-per-probe configuration and a 64-cell octahedral
map, distinguishing "no ray this frame" from "legitimately dark cell"
requires tracking state we don't have. Deferred until [S1.5] lands the
ray-redistribution queues, at which point the "untraced cell" set is
explicitly known.

**[S1.5] Sparse spawning + Halton pixel + Reprojection mask invalidation** —
drives spawn via `upscaleFactor = (2, 2)`, cutting ray budget to 1/4;
Halton(2)/Halton(3) picks one pixel per 16×16 spawn tile per frame;
Reprojection now owns the "this tile is useless" signal, invalidating
PROBE_MASK on sky and no-reprojection-possible paths. Successful
reprojection leaves the mask alone so the last spawn's mask persists
through the rest of the upscale cycle. Mask encoding updated: validity
is a bit-31 flag, INVALID = 0, so uninitialised memory reads as invalid
without a per-frame clear. RayGen's 3×3 CDF reconstruction now reads
each neighbour's sub-pixel from its own mask instead of the CB jitter
(each neighbour was spawned at its own Halton offset on its own frame).

**[S1.5b] Mask MIP chain + FindClosestProbe MIP walk** — deferred. With
1/4 spawning, up to 3/4 of tiles per frame fall back to reprojected
history or PROBE_MASK_INVALID. Filter's immediate-neighbour tap already
handles most of these (the single-level walk the `FindClosestProbe`
helper does today); the MIP chain only starts paying off once holes
reliably span >1 probe-tile — which is mainly disocclusion scenarios
that [S1.5c] also targets. Bundling these two into a single future CL.

**[S1.5c] Algorithm 2 ray redistribution** — deferred. Without it, fully
disoccluded tiles stay dark for up to `ξ_x · ξ_y` = 4 frames. Visible as
brief dark patches under fast motion. Paper §2.1.2 fixes this via an
`empty_tile` / `override_tile` queue pair plus a `patch_screen_probes`
kernel that steals ray slots from well-reprojected tiles to fill
disoccluded ones; keeps the per-frame ray budget constant. Non-trivial
infrastructure (2 new buffers, a classify-and-populate compute pass, a
dispatch-indirect RayGen invocation).

### [S2.1] shipped, [S2.2] deferred

**[S2.1] Probe-space filter with parallax-correction angular rejection** —
the old filter iterated in screen space, so its stride-1 taps across a
tile boundary sampled cells of a *different* probe representing
*different* world directions; the filter was mathematically wrong for
the atlas layout even though it compiled and ran. New filter iterates
in probe space (6 taps at ±{1,2,3} probe-tiles along the blur axis),
reads the same cell in each neighbour probe, and uses Capsaicin's
parallax rejection: re-aim the stored hit distance through the current
probe's position and reject the tap if the reprojected direction
differs from the original cell direction by more than ~3.6°.

**[S2.2] LRU persistent side cache** — deferred. Paper §2.1.8
describes a side texture + MRU-reorder scheme to keep probes that
reprojection would otherwise evict (mostly useful for thin-geometry
wobble). Substantial infrastructure (eviction signal from reprojection,
a scatter-by-screen-coord pass, MRU reorder, decay-based cleanup).
Not on the critical path for the noise we see on large surfaces.

### [I.1] and [I.2] shipped, [I.3] remains

**[I.1] Edge-aware 4-probe interpolation** — LightPass now queries each
of the 4 surrounding probes for validity (mask bit 31), plane distance
(edge-aware depth against AdaptiveCellSize), and normal dot-product;
each corner's final weight is `screen_bilinear * edge_weight`. If every
corner is rejected, falls back to an unweighted screen-bilinear blend
("relaxed interpolation") so edge pixels don't go black. The paper also
writes this fallback flag into the output alpha as a denoiser hint —
wired in alongside [I.3].

**[I.2] SH L2 upgrade** — 4 → 9 coefficients. Adds the 5 band-2 basis
functions (Y_2_2, Y_2_1, Y_20, Y_21, Y_22) that capture quadrant /
axis-pair directional variation L1 can't express. Atlas layout 2×2 →
3×3 per probe. One follow-up worth noting: `LoadIrradiance` still
evaluates `Σ SH[l,m] · Y_lm(n)` rather than the Ramamoorthi-Hanrahan
irradiance convolution (with per-band cosine-lobe weights π, 2π/3,
π/4). That's a ~1-day fix if we want physically-correct irradiance;
the current form gives a sharper-than-irradiance reconstruction but
matches the behaviour the pipeline was tuned against.

**[I.3] Temporal GI denoiser (minimal variant)** — inline in LightPass,
ping-pong GI-irradiance history (rgb = irradiance, a = linear depth).
Motion-vector reprojection + 5% depth-ratio validity gate + 10% new /
90% history blend. Disocclusion / first-frame / out-of-bounds pixels
use 100% raw irradiance (no over-blur on newly-visible surfaces).

**[I.3b] Inline 3x3 depth-bilateral on history read** — landed. In the
same block that performs motion-vector reprojection, each tap in a 3x3
neighbourhood of the previous history is depth-bilateral-weighted
(tent kernel × `exp(-20 · |Δdepth| / depth)`). Taps with uninitialised
depth or > 5% relative depth mismatch are dropped; if all fail we
stay on raw irradiance. Inline read means no UAV race; the blended
write-back compounds smoothing across frames without tracking an
explicit sample count.

**[I.3c] Inline 5x5 Gaussian bilateral + 0.05 blend rate** — landed.
Swap 3x3 tent (9 taps) for 5x5 Gaussian (25 taps), with the per-tap
bilateral depth weight preserved. Temporal blend rate drops from 0.10
to 0.05 so the denoised output leans harder on the now-wider-smoothed
history. Disoccluded / first-frame pixels still short-circuit to raw.

**[I.3d] Variance-aware adaptive kernel** — deferred. The 5x5 kernel
is fixed-radius and not scaled by per-pixel history age. Paper §2.4.3
enlarges the kernel where sample count is low (disocclusion regions)
and shrinks it where the signal is already converged. Proper
implementation stores a second-moment (luma²) ping-pong texture,
computes per-pixel variance from (moment2 - moment1²), and drives
both kernel radius and blend rate from that variance. Worth a
dedicated pass for A-trous-style multi-stride filtering.
