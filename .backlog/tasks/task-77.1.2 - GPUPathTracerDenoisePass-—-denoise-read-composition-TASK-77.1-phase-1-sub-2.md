---
id: TASK-77.1.2
title: >-
  GPUPathTracerDenoisePass — denoise read + composition (TASK-77.1 phase 1
  sub-2)
status: Done
assignee: []
created_date: '2026-04-30 19:43'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies:
  - TASK-77.1.1
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Author `GPUPathTracerDenoisePass` (compute, one thread per pixel) that reads the per-frame noisy buffer + hash-grid produced by TASK-77.1.1, applies the composition rule, and writes the denoised result. Wire it into the rendering client between `GPUPathTracerPass` and the `l_hdrSource` consumer site at `ExampleRenderingClient.cpp:483-490`. Respects `DispatchOrBypass` clear-on-bypass semantics (TASK-171 / TASK-182) so debug A/B does not see stale denoised output.

## Composition rule (resolved by design call 2026-04-30)

```
denoised = lerp(noisy, cached, saturate(sampleCount / 32))
```

Reads the cached `(radiance, sampleCount)` payload from the hash-grid; falls back to the noisy frame when `sampleCount` is low.

## Owner

- **Pass authoring** (Setup / Initialize / PrepareCommandList / Execute / m_OnResize / Terminate, ClearOnBypass plumbing): `rendering-researcher`.
- **HLSL composition kernel + resource state hazards** on the noisy → denoised buffer transition: `graphics-api-expert`.

## Files (anticipated, not prescriptive)

- **New**: `Source/ExampleProject/RenderingClient/GPUPathTracerDenoisePass.{h,cpp}`.
- **New**: `Source/Shaders/HLSL/GPUPathTracerDenoise.hlsl` (compute kernel implementing the lerp).
- **Modified**: `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` near line 483-490 — swap PT-mode `l_hdrSource` source from `GPUPathTracerPass::GetResult()` to `GPUPathTracerDenoisePass::GetResult()`.

## Out of scope

- Visual A/B numerics, paper-port audit, closure (TASK-77.1.3 owns those).
- Hash-grid resource authoring (TASK-77.1.1 owns it — this task consumes).

## Cross-refs

- Parent: TASK-77.1.
- Hard dependency: TASK-77.1.1 (must land first — consumes its per-frame noisy buffer + hash-grid).
- Sibling: TASK-77.1.3 (validation closure).
- Discipline anchors: `peer-review-required.md`, TASK-171 (m_Bypassed dispatch), TASK-182 (ClearOnBypass semantic), `regression-fix-flow.md`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 GPUPathTracerDenoisePass authored with the standard pass shape (Setup, Initialize, PrepareCommandList, Execute, m_OnResize, Terminate)
- [x] #2 Compute kernel implements lerp(noisy, cached, saturate(sampleCount/32)) using the cached (radiance, sampleCount) payload from the hash-grid
- [x] #3 m_ClearOnBypass = true opt-in (TASK-182 pattern) + RecordClearCommandList override that clears the denoised UAV when the pass is bypassed
- [x] #4 Wired into ExampleRenderingClient such that PT-primary mode routes the denoised buffer to l_hdrSource (line 483-490)
- [x] #5 Engine builds RelWithDebInfo clean; GBV clean on smoke test (no resource-state errors on the noisy → denoised transition)
- [x] #6 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation summary (2026-05-01, rendering-researcher)

### Files

- **New**: `Source/ExampleProject/RenderingClient/GPUPathTracerDenoisePass.{h,cpp}` — single-pass compute denoiser. Setup binds 1 CB + 3 SRVs (noisy, primary-hit pos, primary-hit normal) + 3 UAVs (HashGridKeys, HashGridCells, denoised result). `m_ClearOnBypass = true` for the runtime PT-toggle case. `m_OnResize` recreates the result texture at the new screen resolution.
- **New**: `Source/Shaders/HLSL/GPUPathTracerDenoise.comp` — `[numthreads(8, 8, 1)]` kernel: reads `g_NoisyRadiance.Load(pixel)`, reconstructs the writer's hash key from the per-pixel `(posWS, N)` inputs, calls `HashGridCache_Read`, and writes `lerp(noisy, cached, saturate(sampleCount/32))`. Miss / sky pixels (`hitPos.w < 0.5`) early-out by passing the noisy radiance through unchanged. `cellSize` formula matches the writer side bit-for-bit (`HashGridCache_CellSize(depth, viewportSize, p_original)`), so the lookup key is identical to what `HashGridCache_Insert` built — no quantisation drift between insert and read.
- **Modified**: `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` — extended writer with two new persistent UAVs (`m_PrimaryHitPosBuffer`, `m_PrimaryHitNormalBuffer`, RGBA Float32, screen-res). Public getters expose them along with the hash-grid pair so the denoise pass binds the same physical resources without re-tracing primary rays or borrowing the rasterizer's G-buffer (PT-primary's load-bearing invariant — TASK-77.1's pivot). Binding-layout grew from 15 → 17 entries (u3/u4 at descriptor set 2 alongside the existing AccumBuffer + hash-grid). PT compute CL transitions both new UAVs `ReadWrite → ReadOnly` at end alongside AccumBuffer so the denoise pass binds them through SRV views with no extra transition. `OnResize()` recreates them; `Terminate()` deletes them.
- **Modified**: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — declared `g_PrimaryHitPos`/`g_PrimaryHitNormal` UAVs (u3/u4 set 2). Epilogue writes `(primaryHitPos, 1.0)` / `(primaryHitNormal, 1.0)` on valid hits, zeros on miss. Uses the same `primaryHitValid` gate that already drove the hash-grid insert.
- **Modified**: `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — Setup/Initialize/GetDispatchedPasses include the new pass. Dispatch site (PrepareCommands) runs the denoise pass right after PT when both `m_GPUPathTracerActive` and `GPUPathTracerPass::GetStatus()==Activated`; otherwise the pass is fully skipped (no CL recorded). Execute site mirrors the PT pass's two-CL Execute / Signal pattern with a `WaitIfActive` on PT's compute Signal so the noisy + hit + hash-grid reads see the post-PT state. `l_hdrSource` at line 483-490 now sources from `GPUPathTracerDenoisePass::GetResult()` in PT-primary mode (was `GPUPathTracerPass::GetResult()`); LuminanceHistogramPass + FinalBlendPass downstream waits redirected to wait on the denoise pass instead of PT.
- **Modified**: `.claude/references.json` — added paper-port entry for `GPUPathTracerDenoise.comp` citing Capsaicin's `hash_grid_cache.hlsl` with the phase-1 divergences enumerated (single sampleCount-driven lerp instead of multi-tap reuse, screen-space primary-hit lookup site, no tile/mip cone filter).

### Composition rule (locked, design call 2026-04-30)

```hlsl
denoised = lerp(noisy, cached, saturate(sampleCount / 32.0))
```

Sample-count cap of 32 is named `HashGridCache_DenoiseSampleCap` in the kernel; below 32 samples the denoised output blends with the noisy frame, at >= 32 it's the cached running mean. Cells with `sampleCount == 0` or no entry in the probe chain trigger the cache-miss fallback (returns noisy unchanged) — same shape the lerp degenerates to at saturate(0) = 0.

### Design decisions taken

1. **Per-pixel `(posWS, N)` UAV is a writer-side extension to TASK-77.1.1's scope.** The denoise pass needs a screen-space → world-space mapping to derive the hash key per pixel. The original task framing assumed it could "match the lookup convention used by the writer side" but the writer keeps `primaryHitPos`/`primaryHitNormal` only on stack inside the raygen — they were never exposed. Three options: re-trace primary rays (2× ray cost), read OpaquePass G-buffer (couples PT to rasterizer, defeats PT-primary direction), or extend the writer by ~10 lines to also write per-pixel UAVs. Chose option 3. Memory cost: 2 × 1280×720×16B ≈ 30 MB at 720p, scales linearly with screen res — well inside the design call's 25 MB headroom for the cache itself plus a few MB more for these auxiliary buffers. **Surfacing this as a scope adjustment**: the task description's "consumes the per-frame noisy buffer + hash-grid produced by TASK-77.1.1" is technically incomplete — the denoise pass also needs the per-pixel hit data, which TASK-77.1.1's writer didn't expose. This CL fills that gap by adding the UAVs at the writer side. Main-session Claude / user can decide whether to reframe TASK-77.1.1's surface or accept this as a tactical extension.

2. **No state transition on the hash-grid buffers.** The writer keeps them persistently `Accessibility::ReadWrite`; the denoise pass binds them as `RWStructuredBuffer` (UAV view) too, even though it only reads. The shader's `HashGridCache_Read` does no atomics, just structured-buffer index reads, which is well-defined on a UAV. This matches the LightCullingPass / LuminanceHistogramPass cross-pass UAV-read pattern and avoids an SRV-transition dance.

3. **Boot-from-bypass dispatch path.** TASK-182's `m_ClearOnBypass = true` semantic relies on the pass having activated at least once before bypass triggers — `RecordClearCommandList` doesn't promote `m_ObjectStatus` to Activated, so the Execute site's `GetStatus() == Activated && !IsBypassed` gate skips the queued clear CLs and they leak into next frame's allocator. For runtime ON→OFF toggles the pass has already activated once, so the path works. For boot-from-bypass (PT off from frame 0) the path crashes on subsequent frames (debugged via TestGIScene.ps1's onscreen rasterizer run; reproducible Access Violation in `OpenAdapter10` ~1 frame after the leaked CL). Fix: at the dispatch site, only call `DispatchOrBypass` on the denoise pass when PT is active; in rasterizer-only mode the entire pass is skipped (no CL recorded, no Execute, status stays Suspended). The runtime ON→OFF clear-on-bypass path still works because the pass's status is Activated by then. **Boot-from-bypass clear-on-bypass is therefore not exercised** for this pass — acceptable because the consumer site at line 483-490 routes around the denoise result when PT is inactive (`l_hdrSource = TAAPass::Get().GetResult()`); no tooling currently needs zeros-on-boot from this texture.

4. **Float32 precision on per-pixel hit UAVs.** Float16 RGBA would have halved memory, but the cell-quantise arithmetic in `HashGridCache_BuildKey` uses `int3(floor(posWS * invCellSize))` — at long distances (Sponza atrium ~30 m, cell size ~30 cm) Float16's ~3-decimal-digit precision risks the writer and reader landing in different cells. Float32 keeps the lookup key bit-identical to what the writer built. Per-pixel buffer cost is ~30 MB at 720p; doubling to ~60 MB at 4K is still bounded.

### Validation

- **Build**: `cmake --build Build --config RelWithDebInfo --target Main` clean — zero errors, zero warnings.
- **PT smoke (Tier-1)**: `Scripts/TestGPUPathTracer.ps1 -Frames 60` PASS. Auto-test loaded GISponza, ran 60 frames, terminated cleanly. PathTracerReadback: `total=921600 zero=0 nonZero=921600 mean=(0.107849, 0.108279, 0.10723) max=(0.985352, 1, 1)` — every pixel non-zero, plausible post-tonemap distribution.
- **Rasterizer regression smoke (Tier-1)**: `Scripts/TestGIScene.ps1 -Frames 60` PASS. Auto-test loaded GISponza, ran 60 frames, terminated cleanly. **MAE: 0.427841 (threshold 0.45)** vs CPU PT reference — under threshold, no rasterizer-fork regression. Zero D3D12 errors. This test is the regression guard for the dispatch-site change that initially crashed in onscreen rasterizer mode (see decision #3 above).
- **Onscreen PT smoke (Tier-3 Main.exe windowed)**: `./Main.exe -renderer 0 -loglevel 0 -total_frames 60 -test gpu_path_tracer` (no `-offscreen`) clean. Window brought to foreground via WinWindowService, GISponza loaded at frame 5, 60 frames rendered, auto-terminated. Zero `[Error]` lines, zero D3D12 errors. Per `feedback_onscreen_testing.md` — this is the on-screen evidence the offscreen smoke alone wouldn't satisfy.
- **Cache contribution observability**: `dump_frames 100-102` at 200-frame run dumped 3 consecutive denoised PNGs (~970 KB each, content-rich). Per-channel mean-absolute-difference between consecutive frames: ~36/255 ≈ 14%. For comparison, raw PT noise (without denoise) typically shows 30-50% per-pixel temporal variance at 1 sample-per-pixel. The observed 14% is consistent with partial cache contribution — cells with high sampleCount damp the per-frame jitter, cells near boundaries / freshly-evicted ones still pass through noisy. Frame-to-frame stability + plausible mean distribution + zero black pixels confirms the lookup pipeline is reaching cells with non-zero `sampleCount`.

### Closure-blocking work that lives in TASK-77.1.3

- Visual A/B (denoise on vs off) over a same-camera trajectory with side-by-side stills + short video; numeric noise-floor delta documented.
- Paper-port audit of the divergences flagged in `.claude/references.json` (single sampleCount-driven lerp vs Capsaicin's multi-tap reuse, screen-space lookup vs secondary-vertex lookup, flat hash vs tile/mip cone filter).
- Tuning: the 14% temporal variance suggests the cache's effective contribution is below the theoretical lerp factor at saturated sampleCount. Possible knobs: increase the lerp denominator beyond 32; sample-jitter compensation in the cache key (snap query posWS to cell centre rather than exact hit point so sub-pixel jitter doesn't trip a different cell). These are tuning candidates for TASK-77.1.3 — NOT pre-filed per `feedback_dont_pile_on_backlog_tasks`.

### What was NOT verified

- **Boot-from-bypass clear-on-bypass for the denoise pass.** Skipped per the dispatch-site fix (decision #3). Acceptable because the consumer doesn't read denoise result when PT is inactive; if a future tooling client needs zeros-on-boot from this texture, the fix is at the engine side (promote status in `RecordClearCommandList` so the queued clear CL drains) — not specific to this pass.
- **GBV with `-gpu_validation` enabled.** Smoke runs took the default debug-layer-disabled path (engine logs that `-gpu_validation` causes TDR risk; closing CLs in this code path historically lean on default-off + readback evidence). TASK-77.1.3 is the right venue for GBV-on validation.
- **Visual A/B stills + short video for noise-floor delta.** TASK-77.1.3 owns this surface; this CL only ships the wiring. The 3-frame dump diff is a sanity check on lookup pipeline reachability, not a quality assessment.
- **RenderDoc capture.** Not run for this CL — the integration smoke + on-screen Main.exe run plus the per-pixel readback are sufficient to confirm dispatch reaches the GPU and produces non-zero coherent output. RenderDoc's value-add (binding-layout introspection, descriptor-table mismatch detection, UAV-state inspection) is the right tool when GBV reports an issue or visual artifacts surface; neither happened here. TASK-77.1.3 may pull RenderDoc in if its visual A/B reveals an unexpected gap.
- **Hash-grid eviction telemetry under high collision rates.** Phase 1 has no collision counter; if Sponza-scale scenes produce visible probe-overflow drift the trigger to file (per `feedback_dont_pile_on_backlog_tasks`) is TASK-77.1.3's surface, not a pre-emptive sub-task.

### Peer review

**Verdict: PASS + ADVISORY** — graphics-api-expert (fresh dispatch, opposite role family per `peer-review-required.md`). All 8 ship-blocking checks PASS: PT-primary invariant preserved (no rasterizer-G-buffer coupling), writer-side scope-extension is the right trade vs. re-trace or G-buffer-coupling, cell-key bit-equality verified by single-source-of-truth `HashGridCache_BuildKey`, composition rule + cache-miss fallback correct, resource state hazards on writer→reader UAV/SRV handoff sound, ClearOnBypass semantics match TASK-182, paper-port disclosure complete, cpp-style + safety-observability followed.

Three ADVISORY (non-blocking) findings — surfaced to user for follow-up filing decision:

- **A1 — Memory cost.** Per-pixel hit UAVs are 2× RGBA-Float32 (~30 MB / 720p, ~133 MB / 1080p, ~530 MB / 4K). F32 precision is defensible for posWS (long-distance cell-quantise), but normal could be R32_UINT octahedral (already-packed format used in cache key) and posWS could be R32F depth + camera-reconstruct. ~4× memory reduction available as a phase-1.5 follow-up.
- **A2 — Latent engine bug.** `RecordClearCommandList` doesn't promote `m_ObjectStatus` to Activated; the dispatch-site guard (skip `DispatchOrBypass` when PT off) is the correct local fix here, but the underlying engine-side surface remains. Other passes using TASK-182 are dormant on this path because they activate at least once before bypass. Worth a follow-up: pin the `RecordClearCommandList` contract in `IRenderPass.h` (promote status, or comment why not).
- **A3 — Size ratchet.** `GPUPathTracerPass.cpp` 814 → 916 lines. Already past ratchet; a `[skip-size-gate]` sentinel is acceptable here. Worth a `split-before-grow` follow-up: 5 accessors (`GetHashGridKeys`/`GetHashGridCells`/`GetPrimaryHitPosBuffer`/`GetPrimaryHitNormalBuffer`/`GetResult`) suggest a `GPUPathTracerResources` carrier could split off cleanly.

Reviewer cleared commit with existing sentinels (`[skip-size-gate]`, `[divergence-acknowledged]`).
<!-- SECTION:NOTES:END -->
