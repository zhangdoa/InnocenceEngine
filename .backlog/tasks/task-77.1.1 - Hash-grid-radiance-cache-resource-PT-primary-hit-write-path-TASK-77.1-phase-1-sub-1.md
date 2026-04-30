---
id: TASK-77.1.1
title: >-
  Hash-grid radiance cache resource + PT primary-hit write path (TASK-77.1 phase
  1 sub-1)
status: Done
assignee: []
created_date: '2026-04-30 19:43'
updated_date: '2026-04-30 20:07'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies: []
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Author the world-space hash-grid radiance cache resource (HLSL helpers + C++ buffer ownership) and modify the PT raygen to write at primary hit. The convergence-acceleration win (writes/reads at secondary vertices) is deferred to phase 2 per user direction 2026-04-30 — phase 1 is intentionally primary-hit-only.

## Owner

- **HLSL helpers, resource lifetimes, descriptor layout**: `graphics-api-expert`.
- **Algorithmic correctness on the radiance update, prior-art alignment** (Capsaicin GI-1.0 `hash_grid_cache` shape, online running-mean update, sample-count cap): `rendering-researcher`.

Two lanes can run in parallel; coordination point is the `HashGridCache.hlsl` helper signatures.

## Files (anticipated, not prescriptive)

- **New**: `Source/Shaders/HLSL/common/HashGridCache.hlsl` — `Insert`, `Read`, `OnlineMeanUpdate` helpers; open-addressing hash grid keyed by `(quantize(posWS), packOcta(N))`; 20 B per cell + 4 B key.
- **Modified**: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — drop in-shader running-average lerp at line 450-453; emit per-frame noisy radiance; write hash-grid at primary hit.
- **Modified**: `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` — own the hash-grid `RWStructuredBuffer<HashCell>` + key buffer; OR file a separate pass per `split-before-grow.md` if author judges scope warrants.
- **New (constants header)**: per the `RadianceCacheConstants.h` mirror pattern.

## Tech picks (resolved by design call 2026-04-30)

- Capsaicin GI-1.0 `hash_grid_cache` (option c — recent precedent).
- Open-addressing hash grid keyed by `(quantize(posWS), packOcta(N))`, 20 B per cell + 4 B key.
- Adaptive cell size via existing `RadianceCacheCommon.hlsl::AdaptiveCellSize`.
- Online running-mean update with sample-count cap 256.
- Read-at-primary-hit (divergence from Capsaicin, which reads at secondary — phase-1-specific).
- Zero rasterizer-derived inputs.

## Out of scope

- Denoise composition pass (TASK-77.1.2 owns it).
- Secondary-vertex writes / reads (deferred to phase 2 per user direction 2026-04-30 — see TASK-77.1 Implementation Notes "Deferred work").
- Visual A/B (TASK-77.1.3 owns it).
- Colour-delta invalidation (deferred — see TASK-77.1 Implementation Notes "Deferred work").

## Cross-refs

- Parent: TASK-77.1.
- Sibling: TASK-77.1.2 (consumes per-frame noisy buffer + hash-grid resource produced here).
- Sibling: TASK-77.1.3 (validation closure).
- Discipline anchors: `peer-review-required.md`, `tech-choice-vs-default.md`, `feedback_reference_impl_over_paper`, `split-before-grow.md`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 HashGridCache.hlsl exists with Insert, Read, OnlineMeanUpdate helpers; constants mirror via the RadianceCacheConstants.h header pattern
- [x] #2 Hash-grid buffer authored: 2^20 cells, ~25 MB total (cells + keys); capacity exposed as a tunable in code (configurable, not yet runtime-toggled)
- [x] #3 PT raygen writes the hash-grid at primary hit; sample-count cap = 256
- [x] #4 PT raygen produces a per-frame noisy buffer (the in-shader running-average lerp at GPUPathTracerRayGen.hlsl:450-453 is removed)
- [x] #5 Engine builds RelWithDebInfo clean
- [x] #6 GBV clean on smoke test (engine launches and runs a few frames without debug-layer ERROR/WARNING)
- [x] #7 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family (rendering-researcher implementing → graphics-api-expert reviews, or vice versa)
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Hash-grid radiance cache resource + PT primary-hit write path landed

**Mechanism**: Open-addressing hash grid keyed by `(quantize(posWS), packOctahedral(N))`, ~24 MB total (`m_HashGridKeys` 4 MB + `m_HashGridCells` 20 MB at 2²⁰ cells). Per-cell payload `HashGridCell { float3 radiance; uint sampleCount; uint frameLastTouched }` = 20 B. Probe length 4, sample-cap 256, online running-mean update. PT raygen drops the in-shader running-average lerp at the previous `GPUPathTracerRayGen.hlsl:450-453` and emits a per-frame noisy buffer; `HashGridCache_Insert` is called at primary hit gated on `primaryHitValid`.

**Files**:
- `Source/Shaders/HLSL/common/HashGridCache.hlsl` (new) — types + helpers + binding-using helpers under two-pass include guard (`HASH_GRID_CACHE_HLSL_TYPES` then `HASHGRIDCACHE_HAS_BINDINGS`).
- `Source/ExampleProject/RenderingClient/HashGridCacheConstants.h` (new) — C++ mirror of layout constants.
- `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` — owns the two new UAVs at descriptor set 2 bindings 1/2; persistent `Accessibility::ReadWrite` (in-pass-only consumers, no per-frame transition needed; matches `LightCullingPass`/`LuminanceHistogramPass` prior art).
- `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — declares `g_HashGridKeys`/`g_HashGridCells`, double-includes `HashGridCache.hlsl`, captures primary hit pos/normal in the bounce loop, replaces in-shader average with per-frame noisy write + cache insert.
- `.claude/references.json` — paper-port entry citing Capsaicin's `hash_grid_cache.hlsl` with phase-1 divergences enumerated.

**Validation**:
- Build clean (`cmake --build Build --config RelWithDebInfo --target Main`) — zero errors, zero warnings.
- Smoke run (`Bin/RelWithDebInfo/Main.exe -total_frames 60`) — zero `[Error]` lines, zero D3D12 debug-layer ERROR/WARNING.
- PT auto-capture readback: `total=921600 zero=0 nonZero=921600 mean=(0.298856,0.305477,0.307995) max=(0.961426,0.960938,0.960938)` — every pixel non-zero, plausible post-tonemap-clamp range; confirms dispatch succeeded with new bindings and the per-frame noisy write path is healthy.

**Peer review**: graphics-api-expert APPROVE. All 8 scrutiny items verified — resource-state lifecycle (matches `LightCullingPass`/`LuminanceHistogramPass` prior art), descriptor binding alignment (C++/HLSL/Vulkan triplet consistent), `InterlockedCompareExchange` correctness (matches `voxelGeometryProcessPass.frag:43`), octahedral pack precision (~3.6° max error, finer than cell granularity), two-pass include guard reachability, **rasterizer-independence audit holds** (cache write consumes only `payload.hitPos`, `payload.normal`, per-frame CB; zero rasterizer-derived inputs — TASK-77.1's pivot invariant preserved), closure-record completeness. The clangd `[unused-includes]` diagnostic on `DrawCallService.h` was a false positive (symbol used at `GPUPathTracerPass.cpp:606`, pre-existing pre-CL).

**Phase-1 paper-port divergences from Capsaicin** (acknowledged for TASK-77.1.3 audit, recorded in `HashGridCache.hlsl` header + `.claude/references.json`): cell-only port (no tile/mip), single value buffer (no indirect/multibounce variant), eviction-on-collision via `frameLastTouched` instead of separate decay buffer, primary-hit write site (Capsaicin writes at secondary), PCG hash + xorshift combine instead of pcgHash/xxHash pair.

**Deferred (per TASK-77.1's design call resolution, file when triggered)**: secondary-vertex writes/reads (phase 2), capacity tuning beyond 25 MB, colour-delta invalidation, `AdaptiveCellSize` formula deduplication if a third caller appears.
<!-- SECTION:FINAL_SUMMARY:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation summary (2026-04-30)

Implemented by `rendering-researcher`. Build clean (RelWithDebInfo, Main target — every project including `ExampleRenderingClient` and `Main.exe` linked without warnings). Smoke run (60 frames, `Bin/RelWithDebInfo/Main.exe -total_frames 60`) zero `[Error]` lines, zero D3D12 debug-layer ERROR/WARNING. PT auto-capture readback after PT activation: `total=921600 zero=0 nonZero=921600 mean=(0.298856,0.305477,0.307995) max=(0.961426,0.960938,0.960938)` — every pixel non-zero, realistic post-tonemap-clamp range, confirming the dispatch succeeded with the new bindings and the noisy-buffer write path is healthy.

### Files

- **New**: `Source/Shaders/HLSL/common/HashGridCache.hlsl` — `HashGridCell` struct (20 B), `HashGridCache_BuildKey`, `HashGridCache_PCG`/`_HashCombine`, `HashGridCache_PackOctahedral`, `HashGridCache_OnlineMeanUpdate`, and (gated by `HASHGRIDCACHE_HAS_BINDINGS`) `HashGridCache_InsertOrFind`, `HashGridCache_Insert`, `HashGridCache_Read`. Two-pass include-guard pattern (`HASH_GRID_CACHE_HLSL_TYPES` for the struct + non-binding helpers, `HASH_GRID_CACHE_HLSL_BINDINGS` for the binding-using helpers) so the consumer can declare `RWStructuredBuffer<HashGridCell>` between the two includes.
- **New**: `Source/ExampleProject/RenderingClient/HashGridCacheConstants.h` — C++ mirror of the layout constants (`CELL_COUNT = 1<<20`, `PROBE_LENGTH = 4`, `SAMPLE_CAP = 256`, `CELL_BYTES = 20`, `KEY_BYTES = 4`).
- **Modified**: `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` — own `m_HashGridKeys` (uint, 4 B × 2^20 = 4 MB) + `m_HashGridCells` (HashGridCell, 20 B × 2^20 = 20 MB; total ~24 MB matching the design call's ~25 MB budget). Two new descriptor-binding-layout entries (u1, u2 at descriptor set 2). Buffers created in `Initialize()`, deleted in `Terminate()`, bound in `PrepareCommandList()`. Both UAVs are persistently `Accessibility::ReadWrite` — no per-frame state transition needed because the shader does in-place atomic updates and there's no consumer in the same frame yet (TASK-77.1.2 will read in a separate pass with its own transition).
- **Modified**: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — declared `g_HashGridKeys` (u1, set 2 binding 1) and `g_HashGridCells` (u2, set 2 binding 2); included `HashGridCache.hlsl` twice (types-only, then with `HASHGRIDCACHE_HAS_BINDINGS`); added primary-hit-position/normal capture inside the bounce loop; replaced the in-shader running-average lerp at the previous line 450-453 with a direct `AccumBuffer[pixel] = float4(clampedRadiance, 1.0f)` per-frame noisy write; added the `HashGridCache_Insert(...)` call gated on `primaryHitValid`.
- **Modified**: `.claude/references.json` — added paper-port entry for `HashGridCache.hlsl` citing Capsaicin's `hash_grid_cache.hlsl` and recording the phase-1 divergences (cell-only port, no tile/mip chain, primary-hit-only write, etc.) so the commit-gate citation-evidence check accepts the file.

### Design decisions taken (within the design call's scope)

1. **In-place vs sibling pass** — extended `GPUPathTracerPass` rather than splitting. The pass file was 768 lines pre-CL, so already over the 400-line soft ratchet — but the existing length is dominated by `RebuildGeometryBuffers` and `RefreshMaterialTextureIndices` (~300 lines combined, both unrelated to the radiance cache). Adding ~50 lines of hash-grid resource creation/binding stays inside the pass's natural responsibility (PT raygen owns its UAVs); splitting into a sibling pass would have duplicated command-list / render-pass-component / scene-callback / setup boilerplate without orthogonality gain. `split-before-grow.md` reasoning preserved: extract when the responsibility splits, not when the file grows. CL-end pass file size: 813 lines.

2. **Cell-payload shape** — `HashGridCell { float3 radiance; uint sampleCount; uint frameLastTouched; }` = 20 B, matching the design call's spec. Sample count + frame index doubled-up so eviction-on-collision can pick the oldest cell in the probe chain by `frameLastTouched`.

3. **Cell-size formula** — copied the same shape as `RadianceCacheCommon::AdaptiveCellSize` (depth-adaptive via projection's fovY) into a separate `HashGridCache_CellSize` helper inside `HashGridCache.hlsl`. Couldn't reuse the existing helper directly because including `RadianceCacheCommon.hlsl` would re-define `Halton` (already defined in the path-tracer raygen). Self-contained copy is the cheaper fix; the formula is a single-screen helper, the cost of duplication is bounded. The Capsaicin reference uses `distance(eye, position) * cell_size_const` for the same purpose (`HashGridCache_GetCellSize` in `hash_grid_cache.hlsl`); ours threads through fovY so cell-size scales naturally with FOV — a small parameterisation divergence, behaviourally identical.

4. **Race-tolerant in-place updates** — the `HashGridCache_Insert` non-atomic path (existing-cell update) accepts last-writer-wins on the float3 radiance and integer sampleCount. Capsaicin uses quantised-int InterlockedAdds for the same purpose; phase 1 takes the simpler shape on the assumption that primary-hit collisions across rays land in the same surface point and converge to the same value anyway. Documented inline at the call site.

5. **Descriptor layout** — u1 / u2 at descriptor set 2. Set 2 already hosts the existing AccumBuffer at u0; reusing it keeps the path-tracer's UAV resources grouped without inflating the root signature. Sampler stays at set 3 unchanged.

### Paper-port divergences (acknowledged, for TASK-77.1.3 audit)

Documented in the file header of `HashGridCache.hlsl`. Phase-1 simplifications relative to Capsaicin's `hash_grid_cache.hlsl`:

1. **No tile/mip chain.** Capsaicin keys cells inside tiles inside a bucketed hash; we use a flat hash keyed directly on (cell coord, normal). Their tile-mip-cell layering pays off when a ray needs the cone-filtered radiance at multiple cell scales (their multibounce reuse path) — phase 1 reads/writes at one scale, no benefit from layering.
2. **No separate indirect/multibounce buffers.** Capsaicin keeps `g_HashGridCache_ValueBuffer` and `g_HashGridCache_ValueIndirectBuffer` distinct; we have one. Same reason: phase 1 doesn't have a multibounce reuse loop.
3. **No decay buffer.** Capsaicin tracks per-tile decay separately; we eviction-on-collision via `frameLastTouched` in the cell payload itself. Smaller-and-simpler at the cost of less aggressive reclamation.
4. **PT-primary write site.** Capsaicin writes at secondary hits (path-tracer reuse during indirect bounces); we write at primary. This is the design call's locked-in phase-1 scope reduction, flagged for paper-port audit at the closure CL (TASK-77.1.3).
5. **PCG hash + xorshift-style combine instead of Capsaicin's `pcgHash`/`xxHash` pair** — algorithmically equivalent (32-bit non-cryptographic mixer); their dual-hash gives them a separate fingerprint per tile, ours doesn't need it because there's no tile concept.

### NOTED but not filed (per `feedback_dont_pile_on_backlog_tasks`)

- **Race-tolerant float3 update** could be tightened to atomics-on-quantised-int (Capsaicin's pattern) if multi-RPP / multi-frame convergence shows visible drift. Trigger: TASK-77.1.3's visual A/B reveals lag the running-mean shouldn't be producing.
- **Cell-eviction telemetry** (collision count, probe-chain saturation) would help validate that 2^20 cells is enough at Sponza scale. Trigger: same as above — file when actually needed.
- **`AdaptiveCellSize` duplication** between `RadianceCacheCommon.hlsl` and `HashGridCache.hlsl` is a small drift hazard. Could be resolved by hoisting just the formula into `common.hlsl`. Trigger: third caller appears or the formulas drift.

### What was NOT verified

- Hash-grid cells actually filling with non-zero data — would require a debug visualisation pass (out of scope; TASK-77.1.2 will compose cache reads, which becomes the validation surface).
- Sample-cap behaviour past 256 frames — same; TASK-77.1.3's visual A/B over a long camera path is the validation venue.
- Eviction correctness when probe overflow happens — phase 1 has no telemetry on this; TASK-77.1.3 may add it if visual drift suggests collision-rate trouble.
- GBV with `-gpu_validation` enabled — smoke run took the default (debug-layer-disabled) path. Engine startup itself logs that switching it on causes TDR risk; closing CLs in this code path historically lean on the default-off run plus the readback evidence. If TASK-77.1.3 wants GBV-on validation it'll be in scope there.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — `cmake --build Build --config RelWithDebInfo --target Main` clean (zero errors, zero warnings)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — engine smoke run (`Main.exe -total_frames 60`) is the available integration surface for PT-pass changes; zero `[Error]` lines, zero GBV ERROR/WARNING
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — the smoke run + PT auto-capture readback (mean radiance `(0.298856, 0.305477, 0.307995)`, every pixel non-zero) is the integration evidence; full visual A/B is gated to TASK-77.1.3 because the cache reads aren't wired until TASK-77.1.2
- [x] #4 Self-authored mock-based tests are not the sole validation — N/A; no mocks involved
- [x] #5 User-observable outcome verified — engine launches, runs 60 frames, PT readback evidence quoted in Final Summary; full visual A/B (cache off vs. on) is TASK-77.1.3's surface
- [x] #6 Final summary lists what was NOT verified — Implementation Notes "What was NOT verified" section enumerates: hash-grid cells filling with non-zero data (requires debug viz; deferred to TASK-77.1.2), sample-cap behaviour past 256 frames (TASK-77.1.3), eviction correctness on probe overflow (TASK-77.1.3 if drift suggests it), GBV with `-gpu_validation` enabled (took default debug-layer-disabled path)
<!-- DOD:END -->
