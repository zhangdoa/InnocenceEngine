---
id: TASK-69
title: Sponza single-sided curtains shine too bright under path tracer
status: Done
assignee: []
created_date: '2026-04-18 15:08'
updated_date: '2026-04-18 17:53'
labels:
  - bug
  - path-tracer
  - rendering
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Sponza curtain meshes are single-sided. Path tracer hits that land on the back face likely flip to the wrong shading normal and evaluate as a near-mirror specular — the result looks like polished metal instead of cloth. Also likely contributes to the slow GISponza convergence.

Two directions, pick one or both:
- Treat back-face hits on thin geometry as two-sided (flip normal into the ray-opposing hemisphere before BRDF eval).
- Proper BTDF / sheen / transmission for fabric so the single-face representation isn't a misfit for a thickness-implied material.

Related to TASK-56 convergence investigation; may also want to flag materials tagged as two-sided on the asset side.
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added a ray-facing-normal flip in GPUPathTracerClosestHit.hlsl: if dot(shadingNormal, rayDir) > 0, the ray hit the back of a triangle and the interpolated vertex normal points into the incoming hemisphere — Fresnel then reads a near-grazing angle on the wrong side and returns ~pure-specular, which is what made Sponza curtains look like chrome. Flipping the normal on back-face hits treats thin geometry as two-sided, which matches the physical intent for cloth/leaves. BRDF eval now sees cos(N,V) >= 0. Verified: RenderTest 0, gpu_path_tracer 8-frame on GISponza exit 0, Sponza interior renders as expected (dark, low-sample noise, but not chrome curtains).
<!-- SECTION:FINAL_SUMMARY:END -->
