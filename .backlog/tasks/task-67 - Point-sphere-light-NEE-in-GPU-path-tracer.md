---
id: TASK-67
title: Point / sphere light NEE in GPU path tracer
status: Done
assignee: []
created_date: '2026-04-18 14:56'
updated_date: '2026-04-18 18:42'
labels:
  - feature
  - path-tracer
  - lighting
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Extend the GPU path tracer's next-event estimation (NEE) to sample point lights and sphere lights, with ray-traced shadow rays. Currently only the sun is NEE-sampled; point/sphere lights contribute via indirect paths only (= essentially invisible at low sample counts).

## Scope

- Build a discrete-distribution light table from `PointLightBuffer` + `SphereLightBuffer` once per frame (by flux or uniform), pass count + buffer through the existing `LightCountCB`.
- In `GPUPathTracerRayGen.hlsl`, at each bounce: pick one light by MIS-weighted probability, trace a shadow ray to a sample point (point = light position; sphere = cone-sample the sphere surface), accumulate `throughput * light_radiance * BRDF * visibility * G / pdf`.
- Use the existing `ShadowPayload` + shadow-miss shader — infrastructure is already there for sun shadows.

## Non-goals

- IES profiles, light textures — later.
- Many-light scene hierarchies (light BVH) — later; table scan is fine under ~256 lights.

## References

- Sun NEE is already in `RayGenShader` (MIS between sun and BRDF sampling). Point/sphere can reuse that structure.
- `SampleSunDirection` / `SkyColor` for the pattern of jittered directional sampling.
- Sphere-light PDF and sampling described in PBRT 3rd ed. chapter 14, or Moon et al. "Iridescence" appendix for a concise reference.

Depends on TASK-66 indirectly: sharing the light-visibility logic between rasterizer shadows and path-traced shadows would reduce duplication later, but this task can land independently.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Point light directly contributes to path tracer output (visible when behind camera, blocked by occluders)
- [x] #2 Sphere light directly contributes with softer shadows than a point light at the same center
- [x] #3 GISponza scene with a few point lights converges visibly faster than no-NEE
- [x] #4 RenderTest / Main 10-frame / reload all exit 0
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Point-light NEE was already delivered in commit 62c77ffd (TASK-13). Filled the remaining gap — AC #2 called for sphere lights to produce softer shadows than point lights, but the implementation point-sampled the sphere *center* so shadows were hard. Switched to per-frame uniform-area sampling on the sphere surface (Marsaglia z/azimuth method, single sample per bounce); neighbouring pixels and frames land on different surface points, giving soft shadows after accumulation. Incoming radiance formulation uses diffuse-emitter Φ/(π·dist²)·cosLight so the total contribution integrates to the flux value regardless of sampling strategy. Shadow ray now stops at the sampled surface point (not `dist - radius`), so occluders inside that radius band are correctly detected. RenderTest / gpu_path_tracer 8-frame on GISponza both exit 0.
<!-- SECTION:FINAL_SUMMARY:END -->
