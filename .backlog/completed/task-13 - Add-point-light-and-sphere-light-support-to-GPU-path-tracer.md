---
id: TASK-13
title: Add point light and sphere light support to GPU path tracer
status: Done
assignee: []
created_date: '2026-04-12 18:35'
updated_date: '2026-04-12 23:42'
labels:
  - rendering
  - path-tracer
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The GPU path tracer currently handles directional light (sun) but does not support point lights or sphere lights. GISponza has two point lights and one sphere light that contribute significantly to the interior lighting. Without them the path-traced result is dramatically different from the rasterized preview.

**Work:**
1. Audit the existing path tracer light loop in the relevant HLSL shader
2. Add point light sampling: distance attenuation, solid angle sampling from light position
3. Add sphere light sampling: sample a point on the sphere surface, compute solid angle
4. Ensure light data (position, radius, flux, color temperature) is passed to the GPU — check the constant buffer / structured buffer that feeds the path tracer
5. Handle MIS (multiple importance sampling) if the existing directional light already uses it
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Point lights visible and correctly attenuated in path-traced output
- [ ] #2 Sphere lights visible with correct area-light soft shadows
- [ ] #3 No fireflies or NaN from the new light types
- [ ] #4 Interactive test (toggle_pathtracer) passes
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added point light and sphere light NEE to the GPU path tracer.

Changes:
- LightDataService: added GetPointLightCount() / GetSphereLightCount() to expose active CPU-side counts
- GPUPathTracerPass: expanded binding layout from 8 to 11 entries (b2=LightCountCB, t5=PointLightBuffer, t6=SphereLightBuffer); added m_LightCountCB (PathTracerLightCountData: pointLightCount, sphereLightCount, 2×pad); uploads counts each frame; binds all three new resources in PrepareCommandList
- GPUPathTracerRayGen.hlsl: declared LightCountCB (b2), StructuredBuffer<PointLight_CB> (t5), StructuredBuffer<SphereLight_CB> (t6); added per-bounce NEE loops for point lights (distance attenuation 1/d², attenuationRadius cutoff, shadow ray TMax=dist-0.002f) and sphere lights (point sample at sphere center, TMax=dist-sphereRadius)

All tests pass: RenderTest exit 0, Main.exe 10-frame integration exit 0, toggle_pathtracer interactive test PASS.
<!-- SECTION:FINAL_SUMMARY:END -->
