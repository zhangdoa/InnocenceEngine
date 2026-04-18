---
id: TASK-69
title: Sponza single-sided curtains shine too bright under path tracer
status: To Do
assignee: []
created_date: '2026-04-18 15:08'
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
