---
id: TASK-177
title: 'TASK-175-B: Delete cube-shadow stack — passes, shaders, cbuffer, allocator, capacity (cross-subtree)'
status: To Do
assignee: []
created_date: '2026-04-28'
labels:
  - rendering
  - shadows
  - cleanup
  - architecture
dependencies:
  - TASK-176
parent_task_id: TASK-175
priority: high
references:
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.h
  - Source/Shaders/HLSL/pointShadowGeometryProcessPass.vert
  - Source/Shaders/HLSL/pointShadowGeometryProcessPass.geom
  - Source/Shaders/HLSL/pointShadowGeometryProcessPass.frag
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Engine/Common/GPUDataStructure.h
  - Source/Engine/Services/LightDataService.cpp
  - Source/Engine/Services/LightDataService_PointShadow.inl
  - Source/Engine/Common/RenderingCapability.h
  - Source/ExampleProject/RenderingClient/ShadowCasterCullingPass.h
  - Source/ExampleProject/RenderingClient/ShadowCasterCullingPass.cpp
  - Source/Shaders/HLSL/shadowCasterCulling.comp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask B of TASK-175 (unify shadow paths under RT).** Cross-subtree cleanup. **Strictly depends on TASK-176 visually validating** — do NOT start until A is committed and FPS/visual evidence is on the parent task.

### Cross-role ownership split

This deletion spans two role families. The producer files this as a single backlog task to keep the deletion atomic (no half-deleted intermediate state in master); the dispatcher invokes the agents sequentially in the order below.

- **`rendering-researcher`** — owns `Source/ExampleProject/RenderingClient/*` and `Source/Shaders/HLSL/*`:
  - Delete `PointShadowGeometryProcessPass.{cpp,h}`.
  - Delete `pointShadowGeometryProcessPass.{vert,geom,frag}`.
  - Delete `PointShadowResolver` + `PointPCSS` blocks in `Source/Shaders/HLSL/common/shadowResolver.hlsl` (keep the file; sun/CSM resolvers may still live there if any survive — verify before deleting the file).
  - Delete `ShadowCasterCullingPass.{h,cpp}` and `Source/Shaders/HLSL/shadowCasterCulling.comp`. (TASK-156 retained these because PointShadow reused the indirect-draw command buffer; orphaned post-A.)
  - Delete the `t12` cube-atlas SRV binding in `LightPass.cpp` and the `register(t12) Texture2DArray in_PointShadow` declaration in `lightPass.comp`. Same for the `register(b6) PointShadow_CB g_PointShadows[NR_POINT_SHADOWS]` cbuffer if A no longer reads it.
  - Remove the `in_PointShadow` and `in_LinearSampler` parameters from `EvaluateTiledPointLighting` if A's signature no longer needs them.
  - Remove the `ExampleRenderingClient`-side dispatch sites for `PointShadowGeometryProcessPass` and `ShadowCasterCullingPass` (the `PrepareCommandList`/`Execute`/`SignalOnGPU`/`WaitOnGPU` wiring per pass, plus the entries in `GetDispatchedPasses()` from TASK-171).

- **`low-level-expert`** (or `graphics-api-expert` if the boundary is closer to the GPU resource layer — dispatcher's call) — owns `Source/Engine/*`:
  - Delete `PointShadowConstantBuffer` struct from `Source/Engine/Common/GPUDataStructure.h`.
  - Delete `LightDataService::GetPointShadowAtlas`, `GetPointShadowBuffer`, the `m_PointShadow*` fields on `LightDataServiceImpl`, and the slot-allocator file `LightDataService_PointShadow.inl`.
  - Delete `RenderingCapability::maxPointShadows` (and the `NR_POINT_SHADOWS` shader-side mirror if it lives in a header generated from this).
  - Delete `INVALID_ATLAS_SLOT` IF nothing else still uses it (grep before deleting; the constant may be reused by the future bindless atlas plan — if so, keep with a comment).
  - Drop the `position.w` slot-stamping code in `LightDataService` PointLight upload path; the field becomes free for reuse (keep `position.xyz` semantic, leave `.w` zero or repurpose for `m_CastShadow` if A chose that wire path — coordinate with A's resolution).

### Sequencing within this subtask

1. `rendering-researcher` lands the pass+shader+client deletions in one CL (file deletions only; no logic change beyond removing dispatch wiring).
2. `low-level-expert` lands the engine-common deletions in a follow-up CL (after the rendering side stops referencing the symbols).
3. Both CLs include build-clean evidence and a smoke run (no GBV regressions; baseline visual unchanged because A already replaced the path).

Two CLs is the minimum; do not try to land both in a single agent dispatch. Each role's diff is reviewed separately by its peer reviewer.

### Project invariants (anchor — read before implementing)

1. **TASK-149 preserved**: `LightComponent::m_CastShadow`, editor checkbox, JSON serialization round-trip stay correct. Only the *cube-atlas-slot* plumbing dies; the user-facing flag stays.
2. **TASK-66 closure spirit preserved**: TASK-66 shipped *correctness* (the flag, the editor surface, the serialization). This subtask removes the *implementation* (cube atlas) without removing the *contract*.
3. **No silent regressions**: After both CLs, grep the codebase for `PointShadow`, `PointPCSS`, `INVALID_ATLAS_SLOT`, `maxPointShadows`, `NR_POINT_SHADOWS`, `shadowCasterCulling`. Any survivor is a defect — stale references mean the cleanup was incomplete.
4. **60-FPS bar (rendering-researcher manifest)** — Sponza windowed must hit ≥60 FPS post-CL. **This is the headline result of TASK-175 — measurement is mandatory for closure**. With cube path deleted, the 93.5 ms PointShadow cost should fully evaporate.
5. **GBV clean**: TASK-163 readback ERROR is the only acceptable pre-existing message. Anything new is a defect.

### SOTA tech-choice anchor (mandatory)

Even for a deletion task, justify against (a) training-default ("delete the old code"), (b) current SOTA ("keep the old code as a fallback for low-end GPUs without RT"), (c) what the project does for adjacent problems (TASK-138 deleted CSM+PCSS outright once RT sun proved out — no fallback retained). Pick (c) — same precedent, same engine, same hardware target. The cube path was a stopgap; we have one user (zhangdoa) on RT-capable hardware.

### What this subtask does NOT do

- Does not modify `lightPass.comp` direct-lighting logic — TASK-176 owns that. This is pure deletion + dispatch unwiring.
- Does not measure the final FPS — TASK-175-C closure measurement consumes that.
- Does not delete `SunShadowRTPass` or fold sun into LightPass.

### Validation

- Each CL: clean build (`MSBuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo`), TestSuite green.
- Final smoke after both CLs: `Main.exe -mode 0 -renderer 0 -gpu_validation -total_frames 30` on Sponza — engine terminates cleanly, GBV clean (modulo TASK-163), zero `PointShadow` references in any log channel.
- Final grep for stale references (above) — must come back empty (or only with intentional comments).
- Visual: GISponza + UnitTest spheres look identical to TASK-176's post-A capture. A is the visual reference; B should not change a pixel.

### Peer review

Two CLs, two reviews — each per `peer-review-required.md`:

- `rendering-researcher` CL → reviewer: peer `rendering-researcher` (or `graphics-api-expert` if dispatcher prefers a cross-role read on the LightPass binding-table edits — recommended given the binding-table change touches descriptor-set layout).
- `low-level-expert` CL → reviewer: peer `low-level-expert` (or `software-architect` if sole-owner of `LightDataService` — confirm in the manifest at dispatch time).

Reviewers: bias toward catching missed references. The deletion's failure mode is "missed a survivor that links but never runs" — grep-the-tree cross-checks are the primary review surface.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `PointShadowGeometryProcessPass.{cpp,h}` deleted; pass not dispatched; not in `GetDispatchedPasses()`
- [ ] #2 `pointShadowGeometryProcessPass.{vert,geom,frag}` deleted
- [ ] #3 `PointShadowResolver` + `PointPCSS` deleted from `shadowResolver.hlsl`
- [ ] #4 `PointShadowConstantBuffer` deleted; `t12 in_PointShadow` and `b6 g_PointShadows` deleted from `lightPass.comp`
- [ ] #5 `LightDataService` cube-atlas plumbing deleted: `GetPointShadowAtlas`, `GetPointShadowBuffer`, `m_PointShadow*`, `LightDataService_PointShadow.inl`
- [ ] #6 `RenderingCapability::maxPointShadows` and `NR_POINT_SHADOWS` (if a generated mirror) deleted
- [ ] #7 `ShadowCasterCullingPass.{h,cpp}` + `shadowCasterCulling.comp` deleted; not dispatched
- [ ] #8 `LightComponent::m_CastShadow` + editor checkbox + JSON round-trip preserved (TASK-149)
- [ ] #9 Codebase grep for `PointShadow`, `PointPCSS`, `INVALID_ATLAS_SLOT`, `maxPointShadows`, `NR_POINT_SHADOWS`, `shadowCasterCulling` returns no functional references (intentional comments OK)
- [ ] #10 Build clean; TestSuite green; GBV clean (modulo TASK-163 readback ERROR)
- [ ] #11 Visual: GISponza + UnitTest spheres pixel-equivalent to post-A capture
- [ ] #12 Peer review per CL (rendering-researcher CL → graphics-api-expert; low-level-expert CL → peer or software-architect) before commit
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
<!-- SECTION:NOTES:END -->
