---
id: TASK-179
title: 'Sphere-light + extended-light-shape shadow integration (tile-culling extension)'
status: To Do
assignee: []
created_date: '2026-04-28 14:30'
labels:
  - rendering
  - shadows
  - lighting
  - raytracing
dependencies:
  - TASK-176
priority: medium
references:
  - Source/Shaders/HLSL/lightCulling.comp
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Engine/Component/LightComponent.h
  - Source/Engine/Services/LightDataService.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by TASK-176 audit, 2026-04-28. User-deferred from TASK-175 phase A.**

When TASK-176 (inline RayQuery in `lightPass.comp` for point + spot shadows) lands, sphere lights and any future extended light shapes (rect, disk, area) are NOT yet shadow-cast. The cube-path was suspect for sphere-light shadows even before TASK-175 (sphere data uploaded to `g_SphereLights` b2 but never in `in_LightIndexList` — `EvaluateTiledPointLighting` does not iterate sphere lights at all today).

This task picks up sphere shadows correctly under the new RT-shadow architecture, plus extends to additional light shapes the user wants supported (rect, area, disk — exact list to be enumerated during scoping).

### Two architectural shapes

1. **Extend tile culling** — `LightCullingPass` (`lightCulling.comp:105`) currently culls only `g_PointLights`. Add a sphere-light loop (and any new light-type loops). Each tile gets a per-type index list. `EvaluateTiledPointLighting` (rename: `EvaluateTiledLighting`) iterates each per-type list and traces RT shadows inline. **Cleanest unification; mirrors the point-light path post-TASK-176.**
2. **Brute-force sphere/area loop in LightPass** — skip tile culling for sphere/area lights; iterate them all per pixel with a maxN cap. Cheaper to ship, scales poorly with light count, but sphere/area lights are typically few (decorative fills, key area lights).

Pick during scoping based on expected light counts and cost budget. Path 1 is the SOTA-tech-choice answer if there are >4-8 sphere/area lights typical; path 2 if 1-2 typical.

### What this delivers

1. Sphere light shadow integration: cone-jittered RT shadow ray (cone half-angle from `asin(saturate(sphereRadius / distance))`), single jittered sample, TAA accumulation. Mirrors the `SunShadowRTRayGen.hlsl` cone-jitter pattern. Per-light `m_CastShadow` honoured.
2. Rect / area / disk light support — exact list TBD during scoping. Each gets its own sampling distribution (rect: solid-angle sample over the rect surface; disk: similar; area: Monte Carlo).
3. Tile-culling extension OR brute-force loop, whichever the scoping picks.
4. `LightCullingPass` and `LightPass` consistency — both must agree on which light types are tile-culled vs brute-forced.

### Project invariants (anchor)

- **TASK-138** sun RT path stays.
- **TASK-149** `LightComponent::m_CastShadow` honoured for all light types (not just point/spot).
- **TASK-176** point/spot inline RT path stays as the reference shape — sphere just adds cone-jittering on top.
- **60-FPS bar** — the additional rays must fit in the frame budget. If the brute-force loop puts a typical pixel over budget, extend tile culling first.
- **`tech-choice-vs-default.md` (a/b/c)** — applies to the tile-culling-extension vs brute-force decision. (a) is "extend tile culling because that's how 2010s-era DOOM did light culling"; (b) SOTA is something like ReSTIR DI handling all light types uniformly; (c) project precedent is the existing `LightCullingPass` which proves tile-culling works at scale. Pick (c) unless the SOTA case is clear.

### What this does NOT do

- No deletion of any TASK-175-B residue (that already happened or is happening in parallel).
- No new sampling SOTA (no ReSTIR, no MIS) unless the perf budget mandates.
- No editor authoring of new light shapes — that's a UX/editor task TBD.

### Owner

`rendering-researcher` (shader + tile-culling logic). Coordinate with `software-architect` on light-shape schema if new fields are added to `LightComponent`. Coordinate with `editor-tooling-expert` if light-shape authoring UI is added.

### Why medium priority

Real visual gap (sphere lights cast no shadow). But the immediate user-facing FPS fix is TASK-176 (point + spot inline RT). Once that lands and Sponza is at ≥60 FPS, this task closes the visual gap.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Sphere lights cast shadow via RT (cone-jittered ray, TAA-resolved penumbra)
- [ ] #2 Extended light shapes (list TBD during scoping) integrated
- [ ] #3 Tile-culling vs brute-force decision documented with a/b/c justification per `tech-choice-vs-default.md`
- [ ] #4 `LightComponent::m_CastShadow` honoured for all light types
- [ ] #5 60-FPS bar maintained on Sponza
- [ ] #6 Visual cross-check: sphere-light shadows match PT reference
- [ ] #7 GBV clean
<!-- AC:END -->
