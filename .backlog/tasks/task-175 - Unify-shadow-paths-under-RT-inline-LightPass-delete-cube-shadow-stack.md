---
id: TASK-175
title: 'Unify shadow paths under RT (inline in LightPass; delete cube-shadow stack)'
status: To Do
assignee: []
created_date: '2026-04-28 13:30'
labels:
  - rendering
  - shadows
  - raytracing
  - performance
  - architecture
dependencies: []
priority: high
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/Engine/Services/LightDataService.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"this shadow approach is too prehistorical, as couldn't we do anything better with RT? also this is just point light, how about other light types?"*

TASK-66 shipped a cube-shadow-map path for point/sphere lights (8 lights × 6 faces × full-scene rasterize at 256² = 48 viewports per frame). Measurement (TASK-169 same-session): **PointShadow = 93.5 ms / frame** dominates GPU cost; Sponza ~10 FPS GPU-bound vs the new ≥60 FPS bar.

Meanwhile TASK-138 (same session) already proved hardware-RT for sun shadows: ~0.2 ms vs CSM+PCSS ~1.5 ms, ~7-14× cheaper. The same RT shadow-ray infrastructure handles every light type uniformly; cube atlases are a 2000s rasterizer-era trade that we no longer need.

### Architectural shape: option B — inline RT in LightPass

Extend `lightPass.comp` to trace shadow rays inline during light evaluation, per active light per pixel. Path-tracer-style direct lighting. Reuses the existing tiled light culling (`TiledFrustumGenerationPass` + `LightCullingPass`) so each pixel only traces for lights present in its tile.

Per light type:
- **Point**: trace toward `light.position`; binary visibility 0/1.
- **Sphere**: cone-jittered toward sphere center; angle from sphere radius / distance.
- **Spot**: trace toward light + cone test (existing).
- **Sun**: already done (TASK-138); kept as-is or folded into the same loop.

### What gets deleted

Once inline RT is wired:

- `Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.{cpp,h}`
- `Source/Shaders/HLSL/pointShadowGeometryProcessPass.{vert,geom,frag}`
- `Source/Shaders/HLSL/common/shadowResolver.hlsl::PointShadowResolver` + `PointPCSS`
- `Source/Engine/Common/GPUDataStructure.h::PointShadowConstantBuffer`
- `LightDataService::GetPointShadowAtlas`, `GetPointShadowBuffer`, slot allocator (`LightDataService_PointShadow.inl`)
- The `RenderingCapability::maxPointShadows` constant
- `Source/ExampleProject/RenderingClient/ShadowCasterCullingPass.{h,cpp}` + `Source/Shaders/HLSL/shadowCasterCulling.comp` — was retained in TASK-156 ONLY because PointShadow reused its indirect-draw command buffer; now truly orphaned
- `LightPass.cpp` t12 binding for the cube atlas + cbuffer entries that fed it
- `m_PointShadow*` fields on `LightDataServiceImpl`

Net deletion expected to mirror TASK-138 phase 2 (~1000 lines).

### What stays

- `LightComponent::m_CastShadow` (TASK-149) — still semantically valid; LightPass now consumes it directly to gate the inline trace.
- `INVALID_ATLAS_SLOT` constant — drop with the other point-shadow atlas plumbing.
- `OpaquePass m_PostCLState = CrossQueueExit::ToCommon` (TASK-161) — sun RT still consumes the GBuffer cross-queue; keep.
- Tiled light culling — load-bearing for the inline trace's per-tile light list.

### Cost estimate

Sponza typical pixel sees 1-2 lights post-tile-cull. 1080p × 1-2 rays/pixel ≈ 16-32M rays/frame, ~0.5-1.5 ms inline. LightPass total likely 1-2 ms (vs current 0.37 ms). **Net frame budget: ~10 ms vs current ~101 ms → ~100 FPS at the same visual fidelity.**

### Verification

- GPU timer shows PointShadow gone, LightPass cost in expected range.
- Sponza windowed sustained ≥60 FPS over 30s walkthrough (rendering-researcher 60-FPS bar).
- Visual: PT reference cross-check on GISponza + UnitTest sphere geometry. Soft shadows for sphere lights match PT.
- GBV clean (no new ERROR/WARNING; TASK-163 readback ERROR pre-existing).

### Owner

Multi-agent — producer to decompose. Likely:
- `rendering-researcher` for `lightPass.comp` extension + `lightPassDirectLighting.hlsl` per-light inline trace + visual validation.
- `graphics-api-expert` for inline-RT pipeline state if any new SRV/UAV/sampler bindings needed (RT acceleration structure SRV is already bound in lightPass.comp post-TASK-138).
- Coordinated cleanup of cube-shadow stack across multiple subtrees.

### TASK-66 closure note

This task does NOT undo TASK-66's deliverables. TASK-66 shipped the *correctness* (m_CastShadow flag, atlas-slot allocator, caster pass). The atlas-slot mechanism stays in spirit — inline RT just removes the need for the explicit cube atlas. The flag, the editor checkbox, the serialization round-trip — all preserved.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 lightPass.comp traces shadow rays inline per active light per pixel; tiled-culled light list consumed
- [ ] #2 Per-light-type sampling: point binary, sphere cone-jittered, spot cone-gated; sun unchanged or unified
- [ ] #3 Cube-shadow stack deleted (passes, shaders, cbuffer, atlas resource, slot allocator, capacity constant, orphaned culling pass)
- [ ] #4 LightComponent::m_CastShadow + editor checkbox + serialization preserved
- [ ] #5 GPU timer: PointShadow gone; LightPass within 1-2 ms expected range
- [ ] #6 Sponza windowed sustained ≥60 FPS over 30s walkthrough
- [ ] #7 Visual cross-check vs PT reference on GISponza + UnitTest spheres; soft shadows match
- [ ] #8 GBV clean; no new ERROR/WARNING
<!-- AC:END -->
