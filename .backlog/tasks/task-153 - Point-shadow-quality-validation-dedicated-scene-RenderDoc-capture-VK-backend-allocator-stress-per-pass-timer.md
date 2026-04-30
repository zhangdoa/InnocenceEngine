---
id: TASK-153
title: >-
  Point shadow quality validation: dedicated scene + RenderDoc capture + VK
  backend + allocator stress + per-pass timer
status: Done
assignee: []
created_date: '2026-04-27 11:00'
updated_date: '2026-04-30 19:10'
labels:
  - rendering
  - lighting
  - shadows
  - rasterizer
  - quality
  - validation
dependencies:
  - TASK-148
references:
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Engine/Services/LightDataService.cpp
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Data/ExampleProject/Scenes/
parent_task_id: TASK-66
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Quality follow-up for TASK-66 / TASK-148** (point/sphere cube shadows in rasterized pipeline). All technical correctness landed in commits `055b5c9d`, `e09031c8`, `52903ce6`, `71817f3a`, `2eefa0f8`. The gaps below are deferred-quality-work surfaced in TASK-148's closure — not technical bugs, just unexercised validation surface.

### What this task delivers

1. **Dedicated `PointShadowTest.InnoScene`** — minimal scene with a point light + occluder wall (and ideally a second occluder geometry for a stronger A/B signal). The existing `DEBUG_POINT_SHADOW_BYPASS` toggle in `lightPassDirectLighting.hlsl` produces a clear visual diff on this scene (unlike GISponza, where GI + sun dominate the lighting budget and the point-shadow signal is ~3% mean-luminance delta). Closes parent TASK-66 AC#4 with windowed screenshot evidence.

2. **Per-pass GPU timer capture** under `-total_frames` budget — TASK-140 GPU-timer infrastructure exists, but `GetGpuTimings` Verbose-readback timing didn't fire within the 30-frame smoke run TASK-148 used. Need to either lengthen the smoke window or fix the timing-readback latency so per-pass cost is captured for `PointShadowGeometryProcessPass`. Expected ~0.5–1 ms/frame at 256² × 48 slices for typical scene content; verify the budget. Closes parent TASK-66 AC#5 with documented per-pass cost.

3. **RenderDoc capture of the cube atlas** — TASK-148 wrote `audit_03b_PointShadowAtlas.hdr` from offscreen audit dump but did not produce a RenderDoc frame capture. The cube-atlas slice contents are bisect-grade evidence for any future "shadow looks wrong" symptom (recall the TASK-145 / TASK-146 phantom-regression chain — capture is the discipline). Capture under DX12 + windowed Main.exe + GISponza or the new test scene; all 6 cube-face slices for at least one shadow-casting light should show occlusion-shaped depth contents.

4. **Allocator stress at `>maxPointShadows=8`** — TASK-147's slot allocator has a guard `if (l_NextSlot >= l_MaxPointShadows) continue;` that emits a Warning and skips. This path was not exercised because no current scene authors >8 shadow-casting positional lights. Author a stress scene (or temporarily lower `maxPointShadows` to 2 against a 4-shadow-caster scene) and verify (a) the Warning fires, (b) extra lights render unshadowed instead of crashing, (c) which lights win the slot allocation is deterministic.

5. **VK backend validation** — TASK-148 was tested only on DX12 (`-renderer 0`). The Vulkan path needs (a) the GS-fan-out shader to compile via DXIL→SPIRV, (b) the `RG32F` `Texture2DArray` atlas to bind correctly under VK descriptor semantics, (c) the same on-screen A/B test under `-renderer 1`. If the VK backend is currently disabled or known-broken for unrelated reasons, document that and gate the AC accordingly.

### Why medium priority

Not blocking — the technical implementation is correct and integration-tested under DX12 on GISponza. These items are quality-polish that strengthens validation evidence and exercises currently-untested code paths. Useful before any future work that depends on point shadows (sphere-light angular jittering, LRU eviction, cross-cube-face PCSS seam fixes).

### Owner

- Items 1, 4 (scene authoring): `software-architect` or `test-expert`.
- Items 2, 3 (timing + RenderDoc): `graphics-api-expert` or `rendering-researcher`.
- Item 5 (VK): `graphics-api-expert`.

Coordinate via producer; can be split into smaller subtasks if a single agent dispatch is too broad.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `Data/ExampleProject/Scenes/PointShadowTest.InnoScene` authored with point light + occluder wall; loads cleanly via `Main.exe -mode 0 -renderer 0`
- [ ] #2 Windowed before/after screenshots (DEBUG_POINT_SHADOW_BYPASS toggle) on PointShadowTest.InnoScene show clear visible shadow on the wall's far side
- [ ] #3 RenderDoc frame capture of cube atlas — all 6 face slices visible with occlusion-shaped depth contents for at least one shadow-casting light
- [ ] #4 Per-pass GPU timer readback for `PointShadowGeometryProcessPass` recorded under `-total_frames` budget; cost documented (expected ~0.5–1 ms/frame at 256² × 48 slices)
- [ ] #5 Allocator overflow path exercised: scene with `>maxPointShadows` shadow-casting lights produces Warning + deterministic slot assignment + no crash
- [ ] #6 VK backend (`-renderer 1`) validates: shaders compile through DXIL→SPIRV, atlas binds, on-screen A/B test passes — OR backend is documented as gated-off with rationale
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Surfaced from TASK-148 closure (producer, 2026-04-27). All AC items are deferred-quality-work, not technical bugs. The TASK-148 implementation chain (`52903ce6` + `71817f3a`) is the integration baseline; this task adds validation evidence on top.

**Useful prior art**:
- `DEBUG_POINT_SHADOW_BYPASS` toggle in `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` for A/B comparison.
- `audit_03b_PointShadowAtlas.hdr` offscreen dump path already wired (TASK-148) — the RenderDoc capture replaces this with a real frame capture under windowed Main.exe.
- TASK-140 GPU-timer infrastructure under `GraphicsHardwareService` — re-read its Verbose-readback timing assumptions before extending the smoke window.
- TASK-147 slot allocator (`LightDataService::UpdatePointShadowData`) guard — `Warning` + `continue` on `l_NextSlot >= l_MaxPointShadows`.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Obsolete under PT-primary direction (2026-04-30)

Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

User's directive (2026-04-30): "we won't need to do any shadowmaps anyway with PT, so what cube atlas?" The cube-atlas pipeline (TASK-66 / TASK-148 closure surface) is precisely the shadowmap-trick scaffolding that PT renders unnecessary. Validation work that exercises currently-untested code paths in a soon-to-be-fallback subsystem is not load-bearing for any direction the project is taking.

ACs left unticked — they were not done; they are now irrelevant:
- AC #1 (PointShadowTest scene) — no longer needed; PT shadows are correct by construction.
- AC #2 (windowed A/B screenshots) — RT shadow rays via TASK-138 / TASK-175 already validated visually on GISponza; the cube-atlas-specific A/B is unnecessary.
- AC #3 (RenderDoc capture of cube atlas) — atlas is fallback only; capture-grade evidence not justified.
- AC #4 (per-pass GPU timer) — superseded by TASK-181 attenuation gate around LightPass inline-RT shadow + TASK-206 follow-up; PointShadow cost was already deleted by TASK-177 per TASK-181's perf framing.
- AC #5 (allocator overflow stress) — fallback codepath, low-value to harden.
- AC #6 (VK backend validation) — same; if VK is ever exercised against the cube atlas, it is a fallback verifying-the-fallback test.

**No follow-up task filed.** The cube-atlas validation surface is itself on the demotion path. The right place for any "did the rasterizer fallback regress?" check is a future debug-comparison harness once the PT-primary pipeline is the user-facing default; this task does not survive into that shape.

**Cross-ref**: TASK-66 parent (cube-atlas effort), TASK-148 closure (technical correctness in DX12), TASK-77 (PT-primary direction approval), TASK-138 / TASK-175 / TASK-181 (RT-shadow chain that supersedes the rasterizer's PointShadow path).
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code / scene-data compiles + deploys cleanly
- [ ] #2 Pre-existing integration tests re-run and green
- [ ] #3 Windowed A/B screenshots committed under `Build/captures/task-153/` (gitignored — link in Final Summary)
- [ ] #4 RenderDoc capture artifact preserved (path documented in Final Summary, .rdc file gitignored)
- [ ] #5 GPU-timer log line for `PointShadowGeometryProcessPass` quoted in Final Summary
- [ ] #6 Final Summary lists what was NOT verified honestly
<!-- DOD:END -->
