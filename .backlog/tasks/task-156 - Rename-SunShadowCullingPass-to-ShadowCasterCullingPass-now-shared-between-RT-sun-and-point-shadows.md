---
id: TASK-156
title: 'Rename SunShadowCullingPass → ShadowCasterCullingPass (now shared between RT sun + point shadows)'
status: Done
assignee:
  - rendering-researcher
created_date: '2026-04-27 14:30'
updated_date: '2026-04-27 16:45'
labels:
  - rendering
  - cleanup
  - naming
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/ShadowCasterCullingPass.h
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/Shaders/HLSL/shadowCasterCulling.comp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-138 phase 2 closure, 2026-04-27.**

After TASK-138 deleted the sun CSM rasterizer path, `SunShadowCullingPass` retained because TASK-66's `PointShadowGeometryProcessPass.cpp:222` reuses its indirect-draw command buffer. The "Sun" prefix is now misleading — the class produces shadow-caster culling output consumed by point/sphere shadow passes, not by anything sun-related.

### Required fix

- Rename class + file: `SunShadowCullingPass` → `ShadowCasterCullingPass` (or similar — pick a name that reflects its actual role across the shadow types it now serves).
- Update consumers: only `PointShadowGeometryProcessPass.cpp:222` per current state; verify with grep.
- Update shader file `sunShadowCulling.comp` to a matching name if it exists.

### Why low priority

Pure naming hygiene; no behavioural change. Could be batched with any other shadow-related cleanup.

### Owner

`rendering-researcher` (render-pass C++ + shader naming).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Class + file renamed; consumers updated; build green
- [x] #2 Shader file (if applicable) renamed to match
- [x] #3 Grep for old name across repo returns zero hits in tracked source (WIP/* matches OK)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Files

- `Source/ExampleProject/RenderingClient/SunShadowCullingPass.h` → `ShadowCasterCullingPass.h`. Class name, `INNO_CLASS_SINGLETON`, `GetPassName()`, and `GetComputeShaderPath()` all renamed.
- `Source/Shaders/HLSL/sunShadowCulling.comp` → `shadowCasterCulling.comp`. Header banner comment updated to describe present role; shader body unchanged.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`: include + 7 call sites (Setup, Initialize, PrepareCommandList, status check, GetCommandListComp, GetRenderPassComp x2 in WaitOnGPU + Execute, Terminate).
- `Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp`: include + the `GetResult()` consumer at the indirect-draw command buffer reuse point.

## Comment cleanup (per `comment-discipline.md`)

Three historical comment blocks at the rename touchpoints described "follow-up cleanup" or referenced TASK-138 / TASK-148 archaeology. Now that the rename has landed, those blocks describe history rather than present invariants. Replaced with present-tense why-comments where a why-comment is still warranted (the cross-pass indirect-draw buffer reuse), removed where the code is now self-evident.

## Validation

1. **Build green** — `cmake --build Build --config RelWithDebInfo --target Main` (RelWithDebInfo, DX12 path). Two TUs recompiled (`ExampleRenderingClient.cpp`, `PointShadowGeometryProcessPass.cpp`), Main.exe linked.
2. **Mirror-semantic shader chain (TASK-146)** — `Compile-HLSL.psm1` auto-purged the orphan `Bin/Shaders/DXIL/sunShadowCulling.comp.dxil` and emitted the new `shadowCasterCulling.comp.dxil`. CMake `inno_deploy_runtime_payload` POST_BUILD wiped + recopied `Bin/RelWithDebInfo/Shaders/DXIL/`; deployed payload contains only the new artifact.
3. **Live-engine smoke** — `Main.exe -renderer 0 -total_frames 60` (DX12, default loglevel). UnitTest loaded → auto-test triggered GISponza load at frame 5 → 60 frames rendered → engine auto-terminated cleanly. 0 D3D12 errors / 0 corruption / 0 validation errors.
4. **Grep hygiene** — `git ls-files | xargs grep -l "SunShadowCulling\|sunShadowCulling"` returns zero hits outside `.backlog/`, `docs/`, and `.alignments/` (historical archives — left untouched per comment-discipline; they describe past state).

## Pre-existing infrastructure issue surfaced (not in scope for this CL)

`Scripts/TestGIScene.ps1` is internally inconsistent: it passes `-loglevel 2` (= Warning) but its grep success criteria look for Success-level markers (`"GITestBox.InnoScene has been loaded"`, `"Auto-test:.*terminating"`), which are suppressed at that loglevel. Additionally, the script passes `-frames` while the engine flag is `-total_frames`, so the Frames param is silently ignored and the engine never auto-terminates — the script then fails on the loglevel-suppressed scene-load grep. Both bugs are independent of TASK-156 and predate this CL. Filing follow-up may be warranted (`ci-build-expert` scope).

## Why-comments preserved

`PointShadowGeometryProcessPass.cpp` retains the why-comment at the buffer reuse: it documents the present invariant (point shadow shares the indirect-draw buffer because the visibility set is identical for the two projections) and the deferred optimization (per-light frustum culling). That's a present-state why-comment, not history.
<!-- SECTION:NOTES:END -->
