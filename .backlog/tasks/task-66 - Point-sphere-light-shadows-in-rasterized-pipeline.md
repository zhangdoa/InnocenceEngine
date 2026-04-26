---
id: TASK-66
title: Point / sphere light shadows in rasterized pipeline
status: In Progress
assignee:
  - producer
created_date: '2026-04-18 14:56'
updated_date: '2026-04-26 22:30'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
  - parent-task
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
- [ ] #1 Cube shadow map atlas allocation for point lights — owned by TASK-147
- [ ] #2 Shadow caster pass populates the atlas for active point/sphere lights — owned by TASK-148
- [ ] #3 LightPass samples the atlas when evaluating point/sphere light contribution — owned by TASK-148
- [ ] #4 Scene with a point light behind a wall produces a correct shadow — verification milestone, owned by TASK-148
- [ ] #5 No perf regression on GISponza auto-test (or documented budget) — verification milestone, owned by TASK-148
- [ ] #6 LightComponent `m_CastShadow` flag + serialization — owned by TASK-149
- [ ] #7 Cube-atlas filter + resolution design call — owned by TASK-150 (rendering-researcher)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Decomposition (2026-04-26)** — single-agent dispatch was correctly surfaced back as out-of-scope; this work spans `graphics-api-expert`, `rendering-researcher`, `software-architect`. Producer decomposition:

| Subtask | Owner | Description | Depends on |
|---|---|---|---|
| TASK-150 | rendering-researcher | Design audit: VSM vs PCF on cube atlas + atlas resolution + `maxPointShadows` default | — |
| TASK-147 | graphics-api-expert | Foundation: cube atlas allocation + `PointShadowConstantBuffer` schema + `LightDataService::GetPointShadowBuffer()` + slot allocator + `RenderingCapability::maxPointShadows` | TASK-150 |
| TASK-149 | software-architect (+ editor-tooling-expert) | LightComponent `m_CastShadow` flag + serialization + editor inspector + scene migration | TASK-150 |
| TASK-148 | rendering-researcher | Caster pass + shaders + LightPass integration + `shadowResolver.hlsl::PointShadowResolver` + scene authoring for AC#4 | TASK-147, TASK-149 |

**Dispatch sequence**:
1. TASK-150 (design call, short audit dispatch) — UNBLOCKS the rest.
2. TASK-147 (foundation) + TASK-149 (component flag) — parallel; both depend only on TASK-150's contract.
3. TASK-148 (caster pass + shaders + integration + verification) — depends on both #2 subtasks.
4. Verification milestone (AC#4, AC#5) lives inside TASK-148.
5. Producer closes parent TASK-66 with structural retrospective per `.claude/disciplines/structural-retrospective.md`.

**Coordination with TASK-146 (build-cache mirror semantics)**: `ci-build-expert` is concurrently fixing the additive shader-deploy chain. Until TASK-146 lands, every shader-touching iteration on TASK-148 must manually nuke `Bin/Shaders/DXIL/` + `Bin/RelWithDebInfo/Shaders/DXIL/` per `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination". Sequence ideally has TASK-148 begin AFTER TASK-146 lands so this hygiene step disappears.

**Universal anchors for every dispatched agent**:
- `.claude/disciplines/regression-fix-flow.md` (bisect-first; nuke DXIL between bisect steps until TASK-146 lands)
- `.claude/disciplines/agent-dispatch.md` (background-by-default)
- `feedback_anchor_invariants_in_dispatch.md` (each subtask brief anchors project invariants)
- `feedback_audit_first_when_scope_is_paper.md` (TASK-150 audit-first)
- `feedback_onscreen_testing.md` (TASK-148 windowed verification)

**Sun-shadow prior art** that subtasks must mirror:
- `Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp` (caster pass shape, render-target descriptor, comparison function, transparency-aware alpha test)
- `Source/Shaders/HLSL/sunShadowGeometryProcessPass.{vert,geom,frag}` (geometry-shader fan-out + packed depth output)
- `Source/Shaders/HLSL/common/shadowResolver.hlsl::SunShadowResolver` (resolver convention: returns shadow factor, 1=shadowed)
- `Source/Engine/Services/LightDataService.cpp::UpdateCSMData` (per-frame matrix-vector populate idiom)
- `Source/ExampleProject/RenderingClient/LightPass.cpp:289` (state-transition pattern for shadow RT before compute consumption)

**Convention parity warning**: `SunShadowResolver` returns `shadow ∈ [0,1]` where 1 = fully shadowed; `EvaluateSunLighting` applies `Visibility = 1 - shadow`. `PointShadowResolver` MUST match this convention. Inversion of this contract was hypothesis #2 of the TASK-145 phantom regression diagnostic.
<!-- SECTION:NOTES:END -->
