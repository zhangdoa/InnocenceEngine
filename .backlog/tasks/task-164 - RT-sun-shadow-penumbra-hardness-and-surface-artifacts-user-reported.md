---
id: TASK-164
title: 'RT sun shadow: penumbra hardness + surface artifacts (user-reported)'
status: To Do
assignee: []
created_date: '2026-04-27 19:30'
labels:
  - rendering
  - shadows
  - bug
dependencies: []
priority: high
references:
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/Shaders/HLSL/common/sunSampling.hlsl
  - Source/Shaders/HLSL/SunShadowRTAnyHit.hlsl
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-reported visual issues with TASK-138 RT sun shadows after running the engine:**

1. **Sharp edges / no penumbra**: shadows are too hard. The cone-jittered shadow ray pattern is wired correctly in `SunShadowRTRayGen.hlsl:104-105` (`PixelJitter2D` × `SampleSunDirection` with `SUN_ANGULAR_RADIUS`), but the user observes hard edges in real-time. Soft penumbra was supposed to emerge from TAA accumulating the frame-to-frame jittered samples.
2. **Surface artifacts (peter-panning-ish)**: visible on object surfaces. Reminiscent of CSM peter-panning, even though RT shouldn't have that bias-tuning class of bug.

### Investigation directions

For (1):
- Verify TAA is actually running and is accumulating the shadow visibility result. If TAA only runs on the LightPass *output* and not on the visibility texture itself, single-sample-per-pixel binary visibility can produce hard edges where TAA can't recover penumbra (the noise bandwidth doesn't fit the temporal blend).
- Check if `g_Frame.frameIndex` is actually incrementing (per-pixel jitter depends on it).
- Single sample / pixel may simply be insufficient. Bump to 4-8 samples or add a small spatial filter pre-LightPass.
- Compare against the GPUPathTracer reference at `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` — PT uses the same `SampleSunDirection`, soft penumbra is the expected result. Diff the in-flight RT shadow visibility texture against PT's converged result.

For (2):
- `RAY_EPSILON = 0.001` (`SunShadowRTRayGen.hlsl:114`) — origin offset along surface normal. At grazing angles, may be insufficient (self-intersection from neighboring triangles). At small geometry scales, may be too large (creates a visible gap == peter-panning).
- Consider TMin offset along ray direction instead of origin offset along normal.
- `RAY_FLAG_FORCE_OPAQUE` (`SunShadowRTRayGen.hlsl:123`) makes alpha-tested geometry cast solid shadows (e.g. Sponza foliage / curtains). If GISponza has alpha-tested materials, those will produce visibly wrong solid shadows. Audit `Data/ExampleProject/Components/*.MaterialComponent.json` for `m_AlphaTested` materials and decide whether to drop the flag or implement an any-hit alpha test.

### Reference artifacts

- `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` — PT reference.
- TASK-138 phase 1/2 design notes and prior commits (`c34df1f3`, `a47efda3`).
- GPUPathTracerRayGen.hlsl uses the same `SampleSunDirection`; visual diff between PT and RT-shadow on the same scene is the diagnostic.

### Owner

`rendering-researcher` (shaders + visual quality).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Penumbra softness investigated; root cause identified (TAA wiring, sample count, jitter source) with cited file:line evidence
- [ ] #2 Penumbra fix lands; soft shadows visible on GISponza windowed capture matching PT reference quality (or documented quality-budget delta)
- [ ] #3 Surface-artifact root cause identified (origin-offset, alpha-test, ray-flag, or other) with cited file:line evidence
- [ ] #4 Surface-artifact fix lands; GITestBox + GISponza windowed captures clean of acne/peter-panning
- [ ] #5 No new GBV ERROR / WARNING from the fix
<!-- AC:END -->
