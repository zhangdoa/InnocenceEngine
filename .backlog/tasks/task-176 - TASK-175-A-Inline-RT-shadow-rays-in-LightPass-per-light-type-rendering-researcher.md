---
id: TASK-176
title: 'TASK-175-A: Inline RT shadow rays in LightPass per light type (rendering-researcher)'
status: To Do
assignee: []
created_date: '2026-04-28'
labels:
  - rendering
  - shadows
  - raytracing
  - performance
dependencies: []
parent_task_id: TASK-175
priority: high
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp
  - Source/ExampleProject/RenderingClient/LightPass.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-175 (unify shadow paths under RT).** Owner: `rendering-researcher`.

Lands the **replacement** before TASK-175-B deletes the cube path. Hard sequencing: A must visually validate before B deletes anything. Visual regression hides cost wins.

### What this subtask delivers

Extend `lightPass.comp` to trace shadow rays inline during light evaluation, per active light per pixel, replacing the `PointShadowResolver` cube-atlas sample path in `lightPassDirectLighting.hlsl::EvaluateTiledPointLighting`.

1. **Inline RT setup in `lightPass.comp`** — RaytracingAccelerationStructure SRV is already bound post-TASK-138 (used by `EvaluateSunLighting`'s SunShadowRT consumer); reuse the same TLAS. If a separate binding slot is needed, coordinate with `graphics-api-expert` (see "Cross-role coordination" below).

2. **Per-light-type sampling in `lightPassDirectLighting.hlsl::EvaluateTiledPointLighting`**:
   - **Point light** (`l_PointLight.luminousFlux.w == AttenuationRadius`, no sphere radius): single shadow ray from `in_PositionWS + EPSILON*in_NormalWS` toward `light.position`. `TMin = EPSILON`, `TMax = length(L_unnormalized)`. Binary visibility 0/1 via the existing `RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` shadow-ray pattern from `SunShadowRTRayGen.hlsl`.
   - **Sphere light** (TODO: distinguish from point — see open question below): cone-jittered ray. Cone half-angle = `asin(saturate(sphere.radius / distance))`. Single jittered sample per frame; rely on TAA to accumulate the penumbra (same pattern as `SunShadowRTRayGen.hlsl` cone-jittered sun disc). Sample direction within cone using the existing TBN+disk-jitter helper or the SunShadowRT jitter primitive — cite-prior-art, do not invent a new sampling routine.
   - **Spot light**: existing cone gate stays (already in `lightPassDirectLighting.hlsl` if present, else add); shadow ray same as point. (Verify before implementing — current LightPass evaluators may not yet branch spot/point/sphere; see TASK-175 spec.)
   - **Sun light**: NO CHANGE in this subtask. `EvaluateSunLighting` keeps consuming `in_SunShadowRTVisibility` from the existing SunShadowRTPass. Folding sun into the inline loop is explicitly out of scope here — folding/deletion of SunShadowRTPass is a TASK-175-C decision once perf data is in hand.

3. **Honour `LightComponent::m_CastShadow` (TASK-149 invariant)** — the cbuffer field that today drives `INVALID_ATLAS_SLOT` stamping must continue to gate the inline trace. Pre-trace check: skip the ray entirely when the flag is off; visibility = 1.0. Wire-up: `LightDataService` already stamps an "is shadow casting" signal per-light (currently via the slot allocator + sentinel). Either reinterpret that bit, or add a dedicated `bool m_CastShadow` field on `PointLight_CB` — the latter is cleaner and is the recommended path. Coordinate with the cbuffer schema in `Source/Engine/Common/GPUDataStructure.h`; do NOT touch `m_PointShadow*` plumbing (TASK-175-B owns that deletion).

4. **Intersection self-shadow handling**: nudge by `EPSILON * in_NormalWS` is the project convention (see `SunShadowRTRayGen.hlsl`); do not invent a new offset constant. If acne shows on UnitTest spheres, the offset is the lever — do NOT chase it via the BSDF or normal smoothing.

5. **Bindings cleanup at the LightPass C++ side** — remove the `t12 PointShadow` SRV binding from `LightPass.cpp` only if you are confident TASK-175-B will delete the resource in the next CL. If unsure, leave it bound but unused this CL; B handles the deletion. Producer's call: leave-bound is the safer sequence, since A's visual cross-check then has zero impact from binding-table changes.

### SOTA tech-choice anchor (mandatory)

Per recent harness lesson (user 2026-04-28): justify the inline-RT shape against (a) training-default ("trace one ray per light per pixel"), (b) current SOTA (denoised area-light sampling, MIS, ReSTIR DI), (c) what the project does for adjacent problems (SunShadowRTPass single-jittered cone ray + TAA temporal accumulation).

Option (c) is what we already do for the only other RT shadow consumer in the engine, and it works. Pick (c) unless you can produce concrete evidence that (b) is necessary at our light count (Sponza ~8 lights, post-tile-cull 1-2 per pixel). Document the choice and the justification in the implementation notes.

### Project invariants (anchor — read before implementing)

1. **TASK-149**: `LightComponent::m_CastShadow`, editor checkbox, JSON serialization stay correct end-to-end. Inline trace must consume the same flag the cube path consumed.
2. **TASK-138**: `SunShadowRTPass` stays. Sun visibility already correct via `in_SunShadowRTVisibility` at `t13`.
3. **TASK-161**: `OpaquePass::m_PostCLState = CrossQueueExit::ToCommon` — sun RT still consumes GBuffer cross-queue. LightPass adding inline RT does not change OpaquePass's exit barrier; verify the same cross-queue contract still holds for the new inline trace's GBuffer reads.
4. **60-FPS bar (rendering-researcher manifest)** — Sponza windowed must hit ≥60 FPS post-CL. This is the load-bearing acceptance criterion; measurement is part of the closure evidence (use `perf-measurement-frame-budget.md` for N — likely `N=30` once we're at 60+ FPS, currently `N=10` for 20-FPS smokes).
5. **No log-spam**: Verbose logs added during development must be runtime-gated or removed before commit (TASK-165 lesson).

### Cross-role coordination

If the inline trace requires a new SRV/UAV/sampler binding that does NOT already exist on `lightPass.comp` post-TASK-138, dispatch a sequenced sub-fix to `graphics-api-expert` for the C++ pipeline-state setup BEFORE landing the shader change. The current expectation: TLAS SRV at the existing post-TASK-138 slot is sufficient. Verify before assuming.

### What this subtask does NOT do

- No deletion of `PointShadowGeometryProcessPass.{cpp,h}`, `pointShadowGeometryProcessPass.{vert,geom,frag}`, `PointShadowResolver`/`PointPCSS`, `PointShadowConstantBuffer`, `LightDataService` point-shadow hooks, `RenderingCapability::maxPointShadows`, `ShadowCasterCullingPass`, or `shadowCasterCulling.comp`. **All deletions belong to TASK-175-B.** Leave the cube path *bound but unused* this CL so visual A/B comparison is one cbuffer flag away.
- No changes to `SunShadowRTPass` or `EvaluateSunLighting`.
- No FPS measurement beyond the closure smoke (TASK-175-C consumes the full perf cross-check vs PT reference).

### Validation

- Engine builds clean (HLSL + C++).
- GISponza launches with all point/sphere lights shadow-casting via inline RT; cube atlas path unused (verify by RenderDoc capture or a temporary `printf` in the cube resolver — should never fire when the new path is enabled).
- UnitTest sphere geometry: visual cross-check vs reference. Soft shadows for sphere lights match the cone-jittered shape. No acne, no peter-panning.
- GPU timer: `PointShadowGeometryProcessPass` cost unchanged (still running, just unused); `LightPass` cost rises from current ~0.37 ms to expected 1-2 ms range. **Net frame should drop from ~101 ms toward ~10 ms** — the cube pass still costs 93.5 ms here because B hasn't deleted it yet, but A's perf delta on LightPass alone should be visible.
- GBV clean (no new ERROR/WARNING; pre-existing TASK-163 readback ERROR acceptable).

### Peer review

Per `peer-review-required.md`: after rendering-researcher implements, dispatch **`graphics-api-expert`** as peer reviewer. Rationale: cross-role catches RT pipeline-state hazards (TLAS lifetime, RT SRV state contracts, descriptor-table layout), shader-side correctness on inline-RT semantics (`RAY_FLAG_*`, payload sizing, `TraceRay` parameter ordering against the existing SunShadowRT call site).

If `graphics-api-expert` is unavailable, fall back to a peer `rendering-researcher` invocation with explicit instruction to read `SunShadowRTRayGen.hlsl` first and compare invariants against the new inline trace.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `lightPass.comp` traces inline shadow rays in `EvaluateTiledPointLighting` per active light per pixel; tiled-culled light list still consumed
- [ ] #2 Per-light-type sampling: point binary visibility, sphere cone-jittered (TAA-resolved), spot cone-gated; sun unchanged
- [ ] #3 `LightComponent::m_CastShadow` honoured by the inline trace — non-shadow-casters skip the ray and get visibility=1.0
- [ ] #4 No changes to `SunShadowRTPass`, `EvaluateSunLighting`, or `OpaquePass` cross-queue exit barrier
- [ ] #5 Cube-shadow stack still exists and is bypassable (visual A/B-able) — TASK-175-B deletes it after A is validated
- [ ] #6 GISponza: GBuffer-state contract still holds (no DX12 GBV ERROR/WARNING beyond pre-existing)
- [ ] #7 Visual cross-check: UnitTest sphere geometry shadows match cone-jittered RT pattern; no acne, no peter-panning
- [ ] #8 SOTA tech-choice justification recorded in Implementation Notes (a/b/c framing per CLAUDE.md)
- [ ] #9 Peer review by `graphics-api-expert` (PASS or PASS+ADVISORY) before commit
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
<!-- SECTION:NOTES:END -->
