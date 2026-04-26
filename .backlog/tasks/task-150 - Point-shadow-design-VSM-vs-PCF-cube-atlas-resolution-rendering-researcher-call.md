---
id: TASK-150
title: 'Point shadow design: VSM vs PCF + cube atlas resolution (rendering-researcher call)'
status: Done
assignee:
  - rendering-researcher
created_date: '2026-04-26 22:30'
updated_date: '2026-04-26 22:30'
labels:
  - design
  - rendering
  - lighting
  - shadows
parent_task_id: TASK-66
priority: high
references:
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
  - Source/Engine/Services/RenderingConfigurationService.h
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Pre-implementation audit subtask of TASK-66.** Owner: `rendering-researcher`. Output: a written design call recorded in this task's Implementation Notes that unblocks TASK-147 and TASK-148.

This is a SHORT audit dispatch (~200-word recommendation, citing files), not implementation. Per `feedback_audit_first_when_scope_is_paper.md` and the surfacing agent's guidance: the cube-atlas filter choice and resolution depend on engine-internal trade-offs that the rendering-researcher resolves from prior art (sun-shadow PCSS implementation, RenderingCapability conventions) — NOT user-facing.

### Two open design calls

#### 1. Filter choice on cube atlas — VSM/PCSS vs hardware PCF

**Sun precedent**: the sun shadow uses a packed `(depth, depth², 0, 1)` color RT and PCSS in `shadowResolver.hlsl::SunShadowResolver`. This gives soft shadows but requires depth + depth² and a per-pixel rotated Poisson kernel.

**Cube-shadow trade-off**:
- **Hardware PCF on D32**: cheap per-sample, no cube-seam math complications, well-understood. Cost: hard shadow edges (or dithered/4-tap PCF only).
- **Moment-PCSS on packed-depth color (parity with sun)**: matches sun visual quality. Cost: cube-seam blocker-search / penumbra spread is non-trivial (kernel crosses face boundaries).

Surfacing producer's MVP recommendation: hardware-PCF on D32 for the first cut; defer cube-PCSS to a follow-up subtask if visual mismatch with sun shadows is unacceptable. Rendering-researcher confirms or overrides.

#### 2. Atlas resolution + slot count

**Math anchor**:
- `32 lights × 6 faces × 1024² × D32 ≈ 750 MB` — too much.
- `32 lights × 6 faces × 256² × D32 ≈ 12 MB` — surfacing producer's suggested default.

`maxPointShadows` ≤ `maxPointLights + maxSphereLights` (capacity in `RenderingConfigurationService.h`). Most lights in a typical scene won't cast shadow; LRU/budget-driven slot allocation is a follow-up. The default capacity should fit a typical worst-case authored scene.

`rendering-researcher` decides:
- Atlas resolution per face (256², 512², adaptive?).
- `maxPointShadows` default in `RenderingCapability`.
- VRAM budget acknowledgment (`graphics-api-expert` confirms fit when implementing TASK-147).

### Output requirements

The design call lands as an edit to this task's Implementation Notes: a 100-200 word recommendation with citations. TASK-147 and TASK-148 are unblocked once the call lands.

This task closes when the recommendation is published. No code change.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Filter recommendation (PCF vs PCSS-cube) documented with rationale in Implementation Notes
- [x] #2 Atlas resolution + `maxPointShadows` default documented with VRAM budget calculation
- [x] #3 Output format for caster `frag` documented (depth-only D32 vs packed-depth color)
- [x] #4 TASK-147 and TASK-148 unblocked — both can begin implementation against the documented contract
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Design call (rendering-researcher, 2026-04-26):**

**1. Filter — PCSS on packed-depth color, mirroring sun.** Override producer's MVP. Rationale: hardware PCF needs `SamplerComparisonState`, which has zero precedent in `Source/Shaders/HLSL` (grep returns only `Source/External/...` hits) and would require new abstraction across `SamplerResourceService` + every backend — a cross-cutting addition for a single pass violates "fix at the right layer". Sun's `(depth, depth², 0, 1)` Float32 `Texture2DArray` path (`SunShadowGeometryProcessPass.cpp:44-59`, `shadowResolver.hlsl::PCSS:105-128`) is already wired through indirect-draw, GS array-slice fan-out, and `SamplerState`-based sampling. `PointShadowResolver` reuses `FindBlockerDepth`/`PCSS`/`ComputeShadowBias` verbatim per cube face. Cube-seam kernel-crossing is real but acceptable at v1: PCSS UV taps that fall outside `[0,1]` per face will read the wrong slice; we accept the seam artifact as a known follow-up rather than introduce a sampler abstraction that doesn't exist.

**2. Atlas — 256² per face, `maxPointShadows = 8` default.** VRAM: `8 × 6 × 256² × Float32 (4B) = 12 MB`. Producer's 32-light figure assumes future LRU eviction; for the no-eviction v1 (TASK-147 explicitly defers it), 8 simultaneous shadow-casters is a realistic authored-scene worst case (Sponza-class), well under `maxPointLights=1024`/`maxSphereLights=128` (`RenderingConfigurationService.cpp:21-22`). Quality at 256² matches typical mid-budget point shadows; bumping to 512² is a one-line config change costing 48 MB if needed.

**3. Caster `frag` output — packed `(depth, depth², 0, 1)` Float32 color, no DSV.** Forced by call (1): PCSS reads `.r` for blocker depth. Atlas resource: `Texture2DArray`, `DepthOrArraySize = maxPointShadows * 6 = 48`, `PixelDataType = TexturePixelDataType::Float32`, `Sampler = TextureSampler::Sampler2DArray`, clear `(1, 1, 0, 1)` — exact mirror of `SunShadowGeometryProcessPass.cpp:44-59`. GS fan-out: per-light invocation with `maxvertexcount(18)` (6 faces × 3 verts), already proven feasible by `voxelGeometryProcessPass.geom`/GI-bake `[maxvertexcount(18)]`; multi-light single-invocation `3*6*N` exceeds GS limits and is rejected.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Recommendation written and committed in this task's Implementation Notes
- [x] #2 No code change in this task — design-only
<!-- DOD:END -->
