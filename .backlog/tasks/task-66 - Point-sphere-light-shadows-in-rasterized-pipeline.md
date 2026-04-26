---
id: TASK-66
title: Point / sphere light shadows in rasterized pipeline
status: To Do
assignee: []
created_date: '2026-04-18 14:56'
updated_date: '2026-04-26 21:36'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Add shadowing for point lights and sphere lights in the rasterized pipeline. Today only the sun (directional) casts shadows; point / sphere lights contribute direct illumination but pass through geometry.

## Scope

- Cube shadow maps for point lights (6 faces per light), or depth-atlas with per-face view matrices. Allocate on demand from a pooled atlas so 32+ lights don't balloon VRAM.
- Sphere-light shadows: sample a few points on the sphere per frame (temporally jittered) against the same cube-map representation; the sphere light evaluation already pseudo-soft-shadows via angular integration, so reusing the point cube map at the sphere center is the minimum-viable start.
- PCF / VSM filter to match the existing sun-shadow quality.
- Per-light enable flag + atlas slot index on `PointLightComponent` / `SphereLightComponent` so passes can cull shadowed-vs-unshadowed.

## Non-goals

- Ray-traced shadows (that's the path tracer pipeline — TASK-67).
- Full cascaded point shadows or moving-light temporal stability (incremental follow-ups).

## References

- `SunShadowGeometryProcessPass` is the existing shadow-caster pass pattern.
- `LightPass` applies sun shadow via `shadowResolver.hlsl`; point/sphere lights are currently evaluated in the same pass without shadow term.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Cube shadow map atlas allocation for point lights
- [ ] #2 Shadow caster pass populates the atlas for active point/sphere lights
- [ ] #3 LightPass samples the atlas when evaluating point/sphere light contribution
- [ ] #4 Scene with a point light behind a wall produces a correct shadow
- [ ] #5 No perf regression on GISponza auto-test (or documented budget)
<!-- AC:END -->
