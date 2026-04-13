---
id: TASK-19
title: >-
  Add path tracer texture sampling — sample material texture maps in GPU path
  tracer
status: To Do
assignee: []
created_date: '2026-04-13 08:05'
labels:
  - rendering
  - path-tracer
  - textures
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The GPU path tracer currently renders geometry and PBR scalars correctly but does not sample texture maps (albedo, normal, metallic, roughness, AO). Add texture sampling support so that textured materials render correctly in path tracer mode.

Work needed:
- Pass texture descriptors/SRV indices to the path tracer shader (bindless or per-material table)
- Sample albedo/metallic/roughness/normal in the closest-hit shader before evaluating BRDF
- Normal map tangent-space transform in the ray tracer
- Handle the BC-compressed textures added in TASK-10 (BC1/BC3/BC4/BC5 — all are samplable in DX12)
<!-- SECTION:DESCRIPTION:END -->
