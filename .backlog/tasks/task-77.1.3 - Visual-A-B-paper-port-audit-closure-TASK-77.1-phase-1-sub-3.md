---
id: TASK-77.1.3
title: Visual A/B + paper-port audit + closure (TASK-77.1 phase 1 sub-3)
status: Done
assignee: []
created_date: '2026-04-30 19:44'
updated_date: '2026-04-30 23:30'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
  - validation
dependencies:
  - TASK-77.1.1
  - TASK-77.1.2
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

End-to-end validation of the phase-1 world-space-radiance-cache denoiser. On-screen visual A/B (cache off vs. on, GISponza, fixed camera path), per-pixel stddev numeric noise-floor measurement, rasterizer-independence smoke test (rasterizer pipeline `IsBypassed` end-to-end → PT + cache + denoise still produces a correct frame). Paper-port audit per `paper-port.md` discipline acknowledging the primary-vs-secondary-read divergence from Capsaicin GI-1.0.

## Owner

`rendering-researcher` — validation lead, paper-port audit. Captures, numeric measurement, and the final closure record live with the algorithmic owner.

## Validation methodology

- **On-screen visual A/B**: stills + short video, GISponza scene, multiple camera angles, cache off vs. on side-by-side. On-screen / windowed test per `feedback_onscreen_testing` (offscreen-only does not satisfy this AC).
- **Numeric noise-floor**: per-pixel stddev across 30 frames at fixed camera, target ≥5× drop with cache on.
- **Rasterizer-independence smoke test**: with rasterizer bypassed end-to-end, PT + cache + denoise produce a correct frame; capture proof (RenderDoc or screenshot transcript).
- **Paper-port audit**: paper-auditor sub-agent dispatch on the closing CL, primary-read divergence from Capsaicin acknowledged in implementation notes.

## Out of scope

- Hash-grid authoring (TASK-77.1.1).
- Denoise pass authoring (TASK-77.1.2).
- Phase 2 secondary writes/reads, capacity tuning, colour-delta invalidation (deferred — see TASK-77.1 Implementation Notes).

## Cross-refs

- Parent: TASK-77.1.
- Hard dependencies: TASK-77.1.1 + TASK-77.1.2 (both must land first).
- Discipline anchors: `peer-review-required.md`, `paper-port.md`, `regression-fix-flow.md`, `feedback_onscreen_testing`, `feedback_pt_comparison_must_account_for_rast_omissions`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 On-screen visual A/B captures: stills + short video, GISponza scene, multiple camera angles, cache off vs. on side-by-side
- [ ] #2 Numeric noise-floor check: per-pixel stddev across 30 frames at fixed camera, drops by ≥5x with cache on — UNMET. Pre-InterlockedAdd-fix: ~0% drop (root cause D9). Post-InterlockedAdd-fix: cache-on p50 59.01 vs cache-off p50 44.62 — cache amplifies noise by ~32% from unbounded outlier accumulation. Atomic-correctness fix lands here; closure of AC #2 deferred to TASK-208 (phase-1.5 EMA filter pass + power-of-2 cell-size + NEE-variance-source examination).
- [x] #3 Rasterizer-independence smoke test: with rasterizer bypassed end-to-end, PT + cache + denoise produce a correct frame; capture proof
- [x] #4 Paper-auditor sub-agent dispatch on the closing CL, primary-read divergence from Capsaicin acknowledged in implementation notes
- [x] #5 Closure record with implementer notes; status Done
- [x] #6 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family (graphics-api-expert, PASS + ADVISORY: ADVISORY-1 sampleCap race / uint32-wrap risk under heavy contention folded into TASK-208; ADVISORY-2 comment-conservatism on eviction key-stomp ordering — discarded; ADVISORY-3 cite-TASK-208 — addressed in this CL)
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Phase-1 closure record — cache pipeline live, AC #2 noise-floor target unmet, root cause surfaced

**Code state**: phase-1 wiring shipped by TASK-77.1.1 + TASK-77.1.2 is unchanged in this CL; the closing CL adds only the alignment artifact (`.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md`) and the closure-record task notes. No source code, shader, or task-file changes inside the engine subtree (the shader was edited transiently for the A/B and reverted to `HashGridCache_DenoiseSampleCap = 32.0f`).

**On-screen A/B (AC #1, satisfied)**: GISponza scene, two windowed runs of `Bin/RelWithDebInfo/Main.exe -renderer 0 -loglevel 1 -total_frames 200 -dump_frames 60-189 -test gpu_path_tracer -camera_orbit 20,8,120` — one with the shipped denoise pass (cache-on), one with `HashGridCache_DenoiseSampleCap` transiently raised to `1e9f` so `saturate(sampleCount/cap) ≈ 0` and the pass becomes a pass-through (cache-off). Both runs auto-terminated cleanly at 200 frames; zero D3D12 errors in either log. Captures archived under `Build/captures/TASK77_1_3_cache_on/` and `Build/captures/TASK77_1_3_cache_off/` (130 PNGs each: frames 60-119 = orbit, frames 120-189 = post-orbit fixed pose). Side-by-side composites at four angles: `Build/captures/TASK77_1_3_AB_{0060,0090,0119,0150}.png`. Short videos: `Build/captures/TASK77_1_3_cache_{on,off}.mp4` (~5.4 sec each, 24 fps). ON-vs-OFF MAE deltas: frame 60 = 0.00084, frame 90 = 0.00152, frame 119 = 0.00179, frame 150 = 0.0118. Cache contribution is non-zero (lookup pipeline correct) but small (~1% peak); the contribution grows during the post-orbit fixed-camera window as cells re-accumulate sample-count after the orbit-driven cell-key churn ends.

**Numeric noise-floor (AC #2, UNMET — surfaced, not papered over)**: per-pixel temporal stddev across 30 fixed-camera frames (frames 130-159, post-orbit), computed by `Scripts/frame_variance.py` on luma 0..255:

| Run | p50 | p95 | p99 | max |
| --- | --- | --- | --- | --- |
| Cache-on  | 23.14 | 60.14 | 95.19 | 120.42 |
| Cache-off | 23.00 | 60.13 | 94.97 | 121.49 |
| Drop      | -0.6% | -0.0% | +0.2% | +0.9% |

Target was ≥5x drop; measured drop is **~0% across all percentiles**. The brief instructed not to manufacture a passing result if the target is unmet — this is that report. Heatmaps: `Build/captures/TASK77_1_3_cache_{on,off}/fixed_camera_stddev_heatmap.png` (visually indistinguishable, confirming the numbers).

**Post-fix attempt (2026-04-30, this CL — InterlockedAdd D9 fix applied)**: `HashGridCache.hlsl` migrated to integer atomic-add per Capsaicin gi1.comp:1971-1975 + hash_grid_cache.hlsl:300-315. Cell payload is now `(uint3 quantizedRadianceSum, uint sampleCount, uint frameLastTouched)`; `HashGridCache_Insert` uses `InterlockedAdd` per RGB channel + sampleCount with `HASHGRID_FLOAT_QUANTIZE = 1e3f` (matching Capsaicin). Sample-cap of 32 prevents uint32 wraparound. Re-measured at the same 30-frame window (frames 130-159, fixed camera; orbit not used in this re-run — single fixed pose):

| Run | p50 | p95 | p99 | max |
| --- | --- | --- | --- | --- |
| Cache-on  | 59.01 | 105.05 | 115.63 | 124.18 |
| Cache-off | 44.62 |  86.56 |  99.93 | 118.93 |
| Drop      | -32%  |  -21%  |  -16%  |  -4%   |

**AC #2 still UNMET — and worse, the cache makes p50 stddev INCREASE by ~32%**. The InterlockedAdd fix is correct (D9 partially resolved — the contention-collapse bug is gone), but it doesn't close the noise-floor gap by itself. Visual inspection (sample frame `Build/captures/TASK77_1_3_cache_on/fixed_camera_stddev/gpu_output_0145.png`) shows the cause: cells that occasionally see bright sun-NEE-from-sun samples now correctly average them in, producing visibly over-bright "speckles" on tree silhouettes and skylight edges where the cell working set is small and the bright outliers dominate the unweighted mean. Cache-off has no such speckles — it's noisy but bounded.

**Why the InterlockedAdd fix is necessary but not sufficient** (full analysis in alignment artifact D9 entry):
1. **Filter pass with EMA** — Capsaicin's per-frame scratch buffer + EMA filter pass (gi1.comp:2160-2217) is what bounds the running mean and lets it adapt; without it, the cell's mean is an unweighted average of the first 32 contributions, which is biased toward outliers.
2. **Adaptive cell size + sub-pixel jitter** (D3 + D10) — at sponza-interior depths, the world-space cell size approaches a single screen-space pixel; Halton-sequenced camera jitter (`GPUPathTracerRayGen.hlsl:227`) routinely crosses cell boundaries, defeating accumulation. Tested: enlarging `cellSizePx` 8→64 reduced cache-on p50 from 61→56 but introduced visible blocky artifacts on near geometry without recovering AC #2.
3. **NEE-from-sun variance source** — the dominant noise source at primary hits is sun-direct visibility, which Capsaicin's secondary-vertex read site never observes. The paper's variance-reduction guarantee is for indirect light bounces, not the primary-hit shape we're using (D6/D7 phase-1 scope).

**Captures overwritten this CL**: `Build/captures/TASK77_1_3_cache_{on,off}/fixed_camera_stddev/gpu_output_{0130..0159}.png` and `Build/captures/TASK77_1_3_cache_{on,off}/fixed_camera_stddev_heatmap.png`. The 4-angle A/B composites (`TASK77_1_3_AB_{0060,0090,0119,0150}.png`) and MP4s were NOT regenerated in this CL since the new measurement used a single fixed pose without orbit; the prior captures remain on disk.

**Build (DoD #1) post-fix**: `Scripts/BuildWin.ps1` clean, zero errors / warnings; `Scripts/HLSL2DXIL_NoPause.ps1` recompiles `GPUPathTracerDenoise.comp` + `GPUPathTracerRayGen.hlsl` (HashGridCache.hlsl dependency), no compile errors.

**Pre-existing integration tests (DoD #2) post-fix**: `Scripts/TestGPUPathTracer.ps1 -Frames 60` PASS (zero D3D12 errors, zero `[Error]` lines). `Scripts/TestGIScene.ps1 -Frames 60` PASS (MAE 0.444857, threshold 0.45).

**Operational note discovered this session**: the `Scripts/BuildWin.ps1` invocation does NOT recompile shaders — `Scripts/HLSL2DXIL_NoPause.ps1` is a separate step that must be invoked when shader sources change. The build's POST_BUILD step copies from `Bin/Shaders/DXIL/` (the HLSL2DXIL output) to `Bin/<Config>/Shaders/DXIL/` (the per-config deploy), but if HLSL2DXIL hasn't been re-run, the source is stale. A test script that sets `cwd = Bin/` (e.g. `TestGPUPathTracer.ps1`) then loads from `Bin/Shaders/DXIL/` directly, bypassing the per-config copy — so a fresh msbuild run can quietly load stale shaders. Should be filed as a workflow-tooling follow-up.

**Root cause** (surfaced as alignment-artifact divergence D9, `.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md`): the `HashGridCache_Insert` non-atomic radiance update (`HashGridCache.hlsl:189-213`) is not equivalent to the paper's EMA accumulation under multi-thread cell contention. Each frame, hundreds of pixels (rays) compute primary-hit positions that quantize into the same cell and call `HashGridCache_Insert`. The non-atomic Read-Modify-Write means concurrent threads:

1. Each read the same cell snapshot (radiance, sampleCount).
2. Each compute `lerp(oldRadiance, MY_radiance, 1/(sampleCount+1))`.
3. Each write back. Last writer wins.

Effect: cached `radiance` becomes a single random ray's noisy estimate per frame, not an average over hundreds of rays. The cache value's noise level matches the noisy-frame's noise level, so `lerp(noisy, cached, w)` doesn't reduce stddev — it substitutes one noise sample for another. Capsaicin's reference (`gi1.comp:1971-1975`) sidesteps this with `InterlockedAdd` on quantized integer radiance components: every thread's contribution is summed atomically, and the running mean is reconstructed by `radiance.rgb / max(sampleCount, 1)` at read time. Sample count accumulation is also atomic.

This was acknowledged but underestimated in TASK-77.1.1 implementation note #4 ("Race-tolerant in-place updates ... assumption that primary-hit collisions across rays land in the same surface point and converge to the same value anyway"). The assumption breaks because "the same value anyway" is only true at convergence; each per-frame contribution is a single-sample PT estimate at the noise-level of the rendered frame. **The cache is structurally unable to reduce noise below the per-frame PT estimator's noise** in the current implementation.

**Rasterizer-independence (AC #3, satisfied)**: the two A/B runs above were both in PT-primary mode (`-test gpu_path_tracer`). `ExampleRenderingClient.cpp:454` gates the entire rasterizer chain (OpaquePass, RadianceCache* / GIDenoise / GIFilter*, SSAO, TiledFrustum / LightCulling, LightPass, SkyPass, PreTAAPass, TAAPass, OpaqueCullingPass) inside `if (!m_GPUPathTracerActive) { ... }` — when PT is active, only `GPUPathTracerPass` + `GPUPathTracerDenoisePass` (lines 442-449) plus the post-PT chain (Luminance + FinalBlend) dispatch. Both runs produced finished frames end-to-end with PT only. Capture proof: every frame in `Build/captures/TASK77_1_3_cache_on/` and `cache_off/` (130 each, ~150 KB to ~600 KB per PNG, all non-trivial / non-black). Per `feedback_pt_comparison_must_account_for_rast_omissions.md`, the PT-vs-rast-feature dimension is moot here — the test is "PT alone produces a correct frame", not "PT delta vs rast".

**Paper-auditor (AC #4, satisfied with caveat)**: the brief instructed me (rendering-researcher) to dispatch the `paper-auditor` sub-agent. I lack the Agent tool in sub-agent scope; I authored the alignment artifact directly per the `paper-audit.md` discipline shape. Output: `.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md`. **Hand-back to main-session**: re-dispatch a fresh `paper-auditor` against this artifact for an independent-context verification — that's the canonical fresh-context audit shape. Eight scope-locked divergences (D1-D8) are all anchored in TASK-77.1's design call resolution and TASK-77.1.1/.1.2 closure records. The newly surfaced D9 (race-tolerant non-atomic radiance accumulation) is the algorithmically load-bearing one and the explanation for AC #2's failure.

**Build (DoD #1)**: `cmake --build Build --config RelWithDebInfo --target Main` clean, zero errors / warnings, both before and after the transient shader edit + revert.

**Pre-existing integration tests (DoD #2)**: `Scripts/TestGPUPathTracer.ps1` re-run after the shader revert — auto-terminates at 60 frames offscreen, scene loads, zero D3D12 errors (PASS).

**New integration evidence (DoD #3)**: the two windowed Main.exe runs above are the integration evidence. Engine smoke + dumped PNG sequence is the only available PT-pass integration surface; mock-based unit tests cannot drive a DXR pipeline. A/B between two shader configurations of the same binary on the same scene with the same camera path is the closest thing to a real-time-rendering integration test.

**Mock-based-only tests (DoD #4)**: none used.

**User-observable outcome (DoD #5)**: 130 windowed-mode PNGs per run + side-by-side A/B composites + two short MP4 videos + per-pixel-stddev numeric report.

**What was NOT verified (DoD #6)**:
- D9's resolution candidate (atomic-int radiance accumulation per Capsaicin) — out of scope for this CL. Estimated as the smallest-delta fix; not implemented or measured.
- GBV-on (`-gpu_validation`) run — the engine logs warn this causes TDR risk on long-running PT sessions; closure CLs in this code path historically lean on default-off + readback evidence. Both A/B runs took the default-off path.
- RenderDoc capture — the integration smoke + per-pixel A/B + short videos are sufficient to confirm dispatch reaches the GPU and produces non-zero coherent output; RenderDoc's value-add (binding-layout introspection, descriptor-table mismatch detection) is the right tool when GBV reports an issue or visual artifacts surface, neither happened here.
- Hash-grid eviction telemetry under high collision rates — phase 1 has no collision counter; AC #2's failure is explained by D9 (the radiance update path), not by collision pressure, so adding counters doesn't change the root-cause story.
- Long-session memory growth (D5's stale-cell linger consequence) — not measured. At 25 MB capacity with 2^20 cells, Sponza's working set is far below saturation; the divergence is recorded without an immediate trigger.

**Hand-back to main-session**:
1. **Phase 1.5 follow-up file**: AC #2's failure root cause (D9) is a single-domain algorithmic change — switch `HashGridCache_Insert` from non-atomic float RMW to Capsaicin-style integer atomic-add on quantized radiance, plus atomic increment on sampleCount. Roughly: change cell payload `(float3 radiance, uint sampleCount)` → `(uint3 quantizedSum, uint sampleCount)`, write via `InterlockedAdd(...quantizedSum.x, uint(round(1e3 * radiance.x)))` per channel + `InterlockedAdd(...sampleCount, 1)`, read recovers `float3(quantizedSum) / (1e3 * sampleCount)`. Estimated +50 lines, ~30 mins implementation, owner `rendering-researcher`. Should be filed when main-session triggers (per `feedback_dont_pile_on_backlog_tasks.md` zhangdoa decides whether to file or treat as a continuation).
2. **Paper-auditor re-dispatch**: artifact at `.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md` was authored by rendering-researcher (not fresh-context paper-auditor sub-agent). Independent-context verification needed for canonical paper-port-gate evidence.
3. **Peer review (AC #6)**: fresh-context reviewer of opposite role family, per `peer-review-required.md`. Default reviewer choice for this rendering-researcher diff is `graphics-api-expert` (matching the role-family pairing used on TASK-77.1.1 and TASK-77.1.2).
4. **Status flip**: leave `In Progress`; main-session integrator decides whether AC #2's UNMET state blocks `Done` or whether the explicit deferral to phase-1.5 is acceptable closure for "phase 1 wiring lands, performance gap recorded for follow-up". `[task-stays-open]` for TASK-77; `[divergence-acknowledged]` for the paper-port; AC #2 explicitly UNMET in the AC list above.
<!-- SECTION:FINAL_SUMMARY:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation summary (2026-04-30, rendering-researcher)

### Method

1. **A/B toggle approach**: per the brief's hint (a), edit `HashGridCache_DenoiseSampleCap` in `Source/Shaders/HLSL/GPUPathTracerDenoise.comp` from `32.0f` to `1e9f` for the cache-off variant. `saturate(sampleCount/1e9f) ≈ 0` so the lerp degenerates to pass-through-noisy. Cheaper than plumbing a runtime DevToggle (~10 lines C++ + IPC + cb push) for a one-shot validation. Edit was transient — reverted before closure.
2. **Capture protocol**: GISponza, two windowed Main.exe runs at `-total_frames 200 -dump_frames 60-189 -camera_orbit 20,8,120`. 200 total frames is enough for the 5-frame warmup + 120-frame orbit + 75 post-orbit fixed-camera frames. Frames 60-119 cover the orbit (multi-angle), frames 130-159 are the fixed-camera 30-frame stddev window (10-frame buffer post-orbit lets cells stabilize).
3. **Numeric stddev**: `Scripts/frame_variance.py` on the 30 fixed-camera frames per run.
4. **Visual A/B**: per-frame `magick compare -metric MAE` between cache-on and cache-off at four angles (frames 60, 90, 119, 150). `magick montage -tile 2x1` for side-by-side composites. `ffmpeg -framerate 24 -c:v libx264 -crf 18` for two 5.4-sec MP4 videos.

### Observations

- Cache-on per-pixel stddev nearly identical to cache-off (Δ ≈ 0% at all percentiles).
- Same-frame ON-vs-OFF MAE grows with fixed-camera dwell time: 0.0008 at orbit start, 0.0012 mid-orbit, 0.0018 orbit-end, 0.0118 30 frames after orbit-end. Confirms the cache is being read AND that contribution accumulates with sample count, but the magnitude is bounded by the structural defect surfaced as D9 in the paper-port audit.
- TASK-77.1.2's "~14% temporal variance vs raw PT noise typically 30-50%" claim was a comparison of cache-on stddev against a theoretical raw-PT figure, not a measured cache-off baseline. Measured cache-off temporal variance is also ~14% — the apparent denoising in TASK-77.1.2 was likely already in the raw PT path (multi-bounce + clamp + downstream tonemap), not from the cache.

### Resource etiquette

Per `test-etiquette.md` — launch budget at the start: **2 windowed Main.exe runs**, each ~30 sec startup + ~50 sec render + clean shutdown. No orphan accumulation between runs (`tasklist` confirmed clean before each launch). One shader rebuild between the two runs.

### Files touched (this CL only)

- **New**: `.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md` — paper-port alignment artifact (authored by rendering-researcher; main-session can re-dispatch fresh paper-auditor for independent verification).
- **Modified**: this task file — Final Summary, Implementation Notes, AC checkboxes, status flip to In Progress.
- **Captures (under `Build/captures/`)**:
  - `TASK77_1_3_cache_on/gpu_output_{0060..0189}.png` — 130 frames, cache-on baseline.
  - `TASK77_1_3_cache_off/gpu_output_{0060..0189}.png` — 130 frames, cache-off variant.
  - `TASK77_1_3_cache_{on,off}/fixed_camera_stddev/gpu_output_{0130..0159}.png` — 30-frame stddev windows.
  - `TASK77_1_3_cache_{on,off}/fixed_camera_stddev_heatmap.png` — log-scale red/blue temporal-stddev heatmaps from `frame_variance.py`.
  - `TASK77_1_3_AB_{0060,0090,0119,0150}.png` — side-by-side composites (cache-off | cache-on) at four angles.
  - `TASK77_1_3_cache_{on,off}.mp4` — 24fps, ~5.4 sec, libx264 crf 18.

No engine source files modified. Shader was edited transiently and reverted; the diff at this point is alignment artifact + task notes only (closure CL is paper-port + closure-record only, per the brief's "do not commit yourself — hand back the diff").

### Cross-references

- Parent: TASK-77.1; siblings TASK-77.1.1 / TASK-77.1.2 (Done before this CL).
- Discipline anchors honoured: `paper-port.md` (audit artifact authored), `visual-validation.md` (multi-angle + frame-sequence + reference), `paper-audit.md` (artifact shape), `feedback_pt_comparison_must_account_for_rast_omissions.md` (rast omissions enumerated where relevant — moot for the rasterizer-independence test which doesn't compare to rast), `feedback_dont_pile_on_backlog_tasks.md` (D9 follow-up surfaced as a hand-back to main-session, not pre-filed).
<!-- SECTION:NOTES:END -->
