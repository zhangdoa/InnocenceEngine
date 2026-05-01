# TASK-77.1 rework — PT secondary-vertex hash-grid radiance cache: design artifact

Author: rendering-researcher (2026-05-01)
Status: Design — implementation has not started.
Successor to: superseded `task-77.1` "Design call resolution (2026-04-30)" (primary-hit cache + lerp composition; reverted at HEAD `10d7b158`).

## Cache-off baseline at HEAD `10d7b158`

Read-confirmed against the current source tree:

- `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` integrates a 4-bounce path tracer with sun NEE, sky NEE (cosine-weighted hemisphere over N), point/sphere-light NEE, multi-lobe BSDF importance sampling, Russian roulette, and a temporal accumulation lerp `AccumBuffer = lerp(prev, candidate, 1/g_FrameCount)` keyed on view-matrix change to reset.
- `Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl` populates `PathTracerPayload` with hit position, shading normal (two-sided shading flip on back-face), texCoord, albedo / metalness / roughness sampled from the bindless `g_MaterialTextures[]`. No cache read or write at the hit. No SH probe lookup. No world-tile hash. Pure material fetch.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` line 482-490: when `m_GPUPathTracerActive`, `l_hdrSource = GPUPathTracerPass::Get().GetResult()` (i.e. the accumulation UAV from raygen). The `if (!m_GPUPathTracerActive)` block at line 435-477 holds the cache + denoise dispatch — it does **not** run in PT mode. Confirmed: nothing sits between PT raygen and the tonemap consumer.
- `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}`: ray-tracing pass with bindings b0–b2 / t0–t7 / u0 / s0 (PerFrame, FrameCount, LightCount, TLAS, Material, MegaVB, MegaIB, MeshOffsets, PointLights, SphereLights, bindless materials, AccumBuffer, sampler). No hash-grid resources owned. Geometry mega-buffers rebuilt per scene load.

This matches the user's described baseline: noisy on motion, settles to stable accumulation when the camera is held. The reset key is `m_PrevViewMatrix` change in `GPUPathTracerPass::Update`.

`Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl` is the **rasterizer-side** screen-probe closest-hit (consumes `in_opaquePassRT3`, `in_LightPassOutgoingLuminance`, world-tile hash). Unrelated to the PT cache rework — it stays as-is for the rasterized fallback path.

## Tech choice

Per `tech-choice-vs-default.md`:

- **(a) default**: per-pixel screen-space temporal reprojection (TAA-style on the PT output). Rejected — depends on rasterizer-derived motion vectors (G-pass output), violates PT-primary direction (TASK-77 approval 2026-04-30: "denoiser must not depend on rasterizer-derived inputs").
- **(b) SOTA**: NVIDIA OptiX denoiser / Intel OIDN. Rejected for first-landing — fundamentally different architecture (NN-based, external dependency), and the user-confirmed direction is hash-grid cache.
- **(c) project precedent**: rasterizer screen-probe + world-tile hash (`RadianceCacheRaytracingPass`, `RadianceCacheReprojectionPass`, `RadianceCacheClosestHit.hlsl`). Adjacent — same engine has shipped a hash-style world cache, but that one is keyed on screen-tile probes. Not directly reusable for the PT path's per-bounce secondary-vertex caching, but the helper file `common/RadianceCacheCommon.hlsl::AdaptiveCellSize` (Algorithm 6) is reusable verbatim.
- **Pick**: hybrid (b)+(c) — Capsaicin GI-1.0 `hash_grid_cache.hlsl` open-addressing world-space hash with secondary-vertex read/write, reusing `AdaptiveCellSize` for cell-size selection. Rationale: this is the published reference cited in `references.json` for `RadianceCacheClosestHit.hlsl`, the user has confirmed the secondary-vertex site shift, and the engine already has the AdaptiveCellSize precedent so the cell-size knob is consistent across passes.

## Cache structure

Open-addressing spatial hash, per Capsaicin GI-1.0 §2.2 / `hash_grid_cache.hlsl`:

| Field | Type | Purpose |
|---|---|---|
| `key` | `uint` | Fingerprint of `(quantize(posWS, cellSize), packOcta(N))` for collision rejection. `0` = empty slot. |
| `radianceSum` | `float3` | Running estimator of indirect-only outgoing radiance at the cell's representative direction. |
| `sampleCount` | `uint` | How many samples have contributed (sample-weighted running mean on read). |

Capacity & sizing — to be matched to Capsaicin's reference numbers in the paper-port audit (see "Open questions / surface-back" below). Order-of-magnitude target: `2^20` cells × ~20 B = ~20 MB. Under-budget for Sponza-class scenes per the existing `RadianceCacheRaytracingPass` allocation patterns.

## Site shift — what goes where

The path tracer runs all bounces in **raygen**, not in `TraceRay`-driven recursion. The closest-hit shader is purely material-fetch; integration is in raygen's `for (bounce = 0; bounce < MAX_BOUNCES; bounce++)`. Therefore:

- **Cache read** lives at `GPUPathTracerRayGen.hlsl`, *inside* the bounce loop, at `bounce >= 1`. This is "the secondary-vertex indirect-lobe read." Primary visibility (bounce 0) is always re-traced — no cache read at the primary hit, ever. This is the structural fix vs. the previous attempt.
- **Cache write** also lives in raygen, at `bounce >= 1`, *after* the integration step that computed the secondary vertex's outgoing radiance. The write feeds the cache for future read at the same world position + normal cell.
- **Bypass `#define`** at both sites (read and write) inside `GPUPathTracerRayGen.hlsl`, named `PT_HASH_GRID_CACHE_ENABLED` (default = 0 → bypass on → bit-identical to baseline). When `0`, both the cache read and the cache write are `#if`-stripped at compile time — no UAV bind, no integration-side modification. When `1`, the indirect lobe at secondary vertices reads from the cache when available and writes to the cache after integration.

This is the load-bearing structural shift from the previous attempt:

| Previous (reverted) | This rework |
|---|---|
| Cache read at primary hit | Cache read at secondary vertex only |
| `lerp(noisy, cached, sampleCount/32)` composition at primary hit | Cache feeds the indirect lobe inside the integrator |
| Once a cell saturates → primary-hit composition reads stale cell average regardless of currently-visible geometry → ring artifacts on motion | Primary visibility re-traced fresh each frame, every frame; cache contributes only to bounce ≥ 1 estimator. No stale-primary-pixel exposure. |

## Bypass invariant (commit 1 must satisfy)

With `PT_HASH_GRID_CACHE_ENABLED = 0`:

- `GPUPathTracerRayGen.hlsl` compiles into the same SPIR-V / DXIL it does today (modulo new `#if`/`#endif` lines that are zero-cost when stripped). The bounce-loop integrator path is unchanged, the AccumBuffer write is unchanged.
- `GPUPathTracerPass` does not bind the new hash-grid resources (or binds null). The dispatch shape is identical to today.
- Output `AccumBuffer` is bit-identical to baseline at every pixel for any fixed scene + camera + `g_FrameCount`.

The first cache-implementation CL ships with the toggle already in place. Closure of *any* commit on this rework requires: cache OFF (toggle=0) is bit-identical to HEAD `10d7b158`; cache ON (toggle=1) passes layer-1 visual gate.

## File / pass plan

| File | Status | Change shape |
|---|---|---|
| `Source/Shaders/HLSL/common/PTHashGridCache.hlsl` | **new** | Hash-grid primitives (key build, hash, lookup, insert, read, write). Paper-port-aligned to Capsaicin `hash_grid_cache.hlsl`. Shared between consumers (only one consumer in phase 1 — `GPUPathTracerRayGen.hlsl` — but isolated in a header so future readers/writers reuse). Octahedral normal pack + cell-size builder. |
| `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | edit | (a) `#define PT_HASH_GRID_CACHE_ENABLED 0` at top. (b) `#if`-gated UAV declaration for the cache buffer. (c) `#if`-gated cache read + write at secondary-vertex sites in the bounce loop. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` | edit | (a) Allocate the cache UAV (sized by the Capsaicin-derived capacity). (b) Add binding slot for the new UAV (descriptor set 2, binding 1; set 2 already holds AccumBuffer at binding 0 → contiguous slot). (c) `m_FrameCount`/scene-load reset clears the cache (TBD whether we clear or hash-evict; design choice deferred to paper-port audit). |
| `Source/ExampleProject/RenderingClient/RadianceCacheConstants.h` | edit | If the constant naming scope grows, add cache-capacity / cell-size constants here so C++ and HLSL agree (mirror pattern of `MaxCSMSplits`). |
| `.claude/references.json` | edit | Add entry mapping `Source/Shaders/HLSL/common/PTHashGridCache.hlsl` → Capsaicin `hash_grid_cache.hlsl`. |

No new render pass — the cache is a UAV owned by `GPUPathTracerPass`, written and read inside the same RT dispatch. No reprojection / filter / integration / denoise passes are added in this phase; the cache *is* the denoiser via the running-mean estimator inside it.

## Acceptance criteria — visual-first

Per the dispatch brief; inserted as the active ACs on `task-77.1`:

| AC | Type | Bar |
|---|---|---|
| AC-1 | Visual, blocking | Candidate (cache ON, bypass off) is visually equivalent-or-improved vs reference (cache OFF / bypass on, long accumulation) at every sampled frame across the capture protocol. Any spatial structure present in candidate but not in reference is a regression. Reviewer's independent layer-1 read must corroborate. |
| AC-2 | Visual, blocking | Settle behavior matches or exceeds baseline. Candidate must converge at least as fast as reference, AND remain stable (no drift / boil) on a 60-frame fixed-camera hold. |
| AC-3 | Numeric, supporting only | Temporal stddev reduction on settled frames vs baseline. Cannot close on its own. |
| AC-4 | Numeric, supporting only | MAE vs cache-off reference on settled frames within tolerance. Cannot close on its own. |

## Capture protocol

(Same shape as the dispatch brief; restated for the closure record.)

- Three scenes: unit-test scene, GI test box, GISponza. First cache-implementation CL may use GISponza alone; closure requires all three.
- ≥2 camera angles per scene.
- 60-frame sequence per angle: motion in frames 0-30, settled hold in frames 30-60.
- Reference (cache OFF) and candidate (cache ON) from the same binary, same camera, same frames, same total-frame budget — the bypass `#define` is the toggle.
- Layer-1 *Visual Read assessment* on 5 sampled frames per scene per angle (0/10/30/45/59). Closure requires ≥30 reads.
- Archive under `Build/captures/TASK-77.1-rework/<scene>/<angle>/<cache_state>/frame_<NN>.png`.

## Implementation discipline

- Build incrementally on the cache-off baseline at `10d7b158`. Commits may stack but every commit independently passes the visual gate (with bypass off enabling the cache). No "fix later" deferrals.
- Dispatch a `paper-auditor` pre-pass against `https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/hash_grid_cache.hlsl` and the secondary-vertex read sites in `gi1.comp` *before* writing `PTHashGridCache.hlsl`. Output the alignment artifact in `.alignments/` listing every divergence. Skipping that pre-pass bakes in paper-port drift this discipline exists to prevent.
- After each implementation piece, surface back to dispatcher with a verification request — do not run BuildWin / HLSL2DXIL / engine binary directly per the machine-resource constraint.
- Shader comments: WHY only, one short paragraph per function maximum.
- Every commit: `Code-AI-Generated-By: Claude Opus 4.7` + peer-review line.

## Open questions surfaced back to dispatcher

These are blocking on substance, not on me; they are surfaced back rather than guessed:

1. **Capsaicin reference numbers** — cell-size formula constants (Capsaicin uses an explicit per-debug-cell-size knob; we use `AdaptiveCellSize` from Algorithm 6 already), capacity (`2^20` cells assumed but Capsaicin may use a different number tuned to their typical scene scale), sample-cap or running-mean weighting policy, and eviction-on-collision policy. Resolved by the paper-auditor pre-pass dispatch — *not* by me reading the paper unaided.
2. **Per-cell normal binning** — does the cache key include octahedral-packed normal (Capsaicin includes it; this rules out the "concave corner reads cell averaged across both walls" bug)? Confirmed by paper-auditor before writing the key-builder.
3. **Cell clear / reset on scene load** — the simplest correct path is to clear the cache UAV when `f_sceneLoadedCallback` fires, identical to the existing `m_FrameCount = 1` accumulation reset. Capsaicin may do something less aggressive (LRU eviction). Phase-1 picks the conservative option (clear on scene load); verify against Capsaicin in the audit.
4. **Test-mode entry points for unit-test scene + GI test box** — `-test gpu_path_tracer_unittest` / `-test gpu_path_tracer_gitestbox` per the brief are concurrently being added by `test-expert`. First cache-implementation CL may proceed against GISponza alone; closure waits on the test-expert CL to land.

## Termination & surface-back conditions

- Closure: all three scenes × ≥2 angles × 5 sampled frames pass AC-1 with `improvement` or `parity` verdict. AC-2 fixed-camera hold matches/exceeds baseline. AC-3/AC-4 reported as supporting evidence only. Reviewer (`graphics-api-expert`, cross-domain) signs.
- Surface-back triggers: any structural blocker (e.g. a Capsaicin-aligned design choice that does not fit the engine's existing UAV / TLAS lifetime), any layer-1 verdict not in `{improvement, parity}` at any sampled frame, any inability to satisfy the bypass-invariant (cache OFF must be bit-identical to baseline).
- Non-terminal but reportable: AC-3 / AC-4 numeric movement (positive or negative) is recorded but does not gate closure. Per the brief, layer-1 visual is the only blocking gate.

## Cross-references

- `paper-port.md` — `paper-auditor` pre-pass mandatory before HLSL author.
- `visual-validation.md` §3a-3b — bypass-toggle pattern; layer-1 *Visual Read assessment* mandatory at every commit.
- `tech-choice-vs-default.md` — block above.
- `references.json` — to be updated with the new `PTHashGridCache.hlsl` entry mapping to Capsaicin `hash_grid_cache.hlsl`.
- `.backlog/tasks/task-77.1` — task body updated alongside this artifact.
- Superseded section: `task-77.1` "Design call resolution (2026-04-30)" — primary-hit cache + lerp composition. Recorded there as superseded; not deleted, kept as the audit trail of the structural error.
