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

## Review (graphics-api-expert peer, 2026-04-28)

**Verdict: PASS**

CL1 scope = rendering-researcher subtree (`Source/ExampleProject/RenderingClient/*`, `Source/Shaders/HLSL/*`). Engine-common deletions (AC #5/#6 partial) are CL2 territory by brief design. Verified against the staged diff (`git diff --cached`), not implementer's assertions.

### Acceptance criteria — line-grounded

- AC #1 met. `PointShadowGeometryProcessPass.{cpp,h}` deleted; `ExampleRenderingClient.cpp` Setup/Initialize/PrepareCommands/ExecuteCommands/Terminate sites removed (5 dispatch sites + GetDispatchedPasses entry). `ExampleRenderingClient.cpp:170-178, 234-241, 312-318, 437-457, 657-664, 974-986, 1062-1067, 1135-1141`.
- AC #2 met. `pointShadowGeometryProcessPass.{vert,geom,frag}` shown deleted by `git status`.
- AC #3 met (effectively superseded). Implementer deleted the entire `shadowResolver.hlsl` rather than just `PointShadowResolver`/`PointPCSS` blocks. Verified no surviving consumers: `Grep "shadowResolver"` across `Source/Shaders` returns zero matches. The two WIP shaders (`voxelGeometryProcessPass.frag`, `volumetricIrraidanceInjectionPass.comp`) had their dead `#include "common/shadowResolver.hlsl"` removed in the same CL — `volumetricIrraidanceInjectionPass.comp:25` keeps a commented-out `SunShadowResolver` *call* but the include is gone, and that file is WIP-shelved per directory naming; no compile dependency.
- AC #4 met. `lightPass.comp:59-69` removed `cbuffer PointShadowCBuffer : register(b6)` and `Texture2DArray in_PointShadow : register(t12)`. C++ side removed the corresponding `[20]`/`[21]` slots and `register(s0) in_samplerTypeLinear`.
- AC #5 deferred to CL2 (engine-common). `Grep` confirms `LightDataService.{cpp,h}`, `LightDataService_PointShadow.inl`, `GPUDataStructure.h` still hold the cube-atlas plumbing — explicit CL2 territory per brief.
- AC #6 deferred to CL2.
- AC #7 met. `ShadowCasterCullingPass.h` deleted (no `.cpp` existed — header-only stub); `shadowCasterCulling.comp` deleted; dispatch + Setup/Initialize/Terminate/GetDispatchedPasses entries removed in `ExampleRenderingClient.cpp`.
- AC #8 met. `LightComponent.h:45 bool m_CastShadow = true;` untouched. `JSONSerializer_Components.cpp:44, 409` round-trip untouched. `Editor-Next/src/components/inspector/LightEditor.vue:20-95` `castShadow` checkbox + IPC binding untouched.
- AC #9 met within CL1 scope. `Grep "PointShadow|shadowCasterCulling|shadowResolver"` over implementation files (`*.cpp,h,hlsl,comp,vert,frag,geom,inl`) returns 7 files — all under `Source/Engine/*` or `Source/Shaders/HLSL/common/common.hlsl` (which mirrors `PointShadowConstantBuffer`). All 7 match the CL2 deletion list verbatim. CL1 subtree is clean.
- AC #10 reported met by implementer (build clean, GBV clean modulo TASK-163, TestSuite). Not re-verified by reviewer (no code execution in single-pass review); no new build dependency or contract drift visible in the diff that would invalidate the smoke claim.
- AC #11 visual parity — outside reviewer's static-read scope; the diff removes only the cube path and the inline RT path was validated under TASK-176, so no rendered-output delta is expected from CL1 alone.
- AC #12 — this review.

### Anchored invariants

- **TASK-138 SunShadowRT** — untouched. `lightPass.comp:87-88` `register(t13) in_SunShadowRTVisibility` and `LightPass.cpp:175-180` slot `[19]` (set=1, idx=13) preserved. `EvaluateSunLighting` signature in `lightPassDirectLighting.hlsl:19-30` reads visibility via `Load(int3(in_ScreenCoord, 0))` directly — no resolver dependency, validates the whole-file shadowResolver.hlsl deletion.
- **TASK-161 OpaquePass exit barrier** — `OpaquePass.{cpp,h}` not in staged diff.
- **TASK-149 m_CastShadow flag** — preserved end-to-end (component, JSON, editor) per AC #8 above.
- **TASK-176 inline RayQuery body** — `lightPassDirectLighting.hlsl:77-156` `EvaluateTiledPointLighting` signature and inline RT body unchanged. The `RAY_FLAG_FORCE_OPAQUE | ACCEPT_FIRST_HIT_AND_END_SEARCH | SKIP_CLOSEST_HIT_SHADER` triplet, `RAY_EPSILON` TMin, and `l_PointLight.shadow.x != 0u` gate all intact.

### Binding-layout cross-check (descriptor-set discipline)

The renumbering 24→21 with shifts `[19→18]`, `[22→19]`, `[23→20]` was the highest-risk surface — verified by reading the new C++ slot table side-by-side with `lightPass.comp` register declarations:

| C++ slot | Set:Index | HLSL register | Symbol |
|---|---|---|---|
| `[18]` | 2:1 | `s1` | `in_samplerTypePoint` |
| `[19]` | 1:13 | `t13` | `in_SunShadowRTVisibility` |
| `[20]` | 1:14 | `t14` | `SceneAS` |

All three agree. No descriptor-set-layout drift between C++ and HLSL.

### Universal disciplines

- **`coding-principles.md` — fix at the right layer.** Whole-file `shadowResolver.hlsl` deletion is correct: deleting only the `Point*` blocks would have left a stub file with one consumer (sun) that doesn't actually need a resolver indirection (sun visibility is now a direct texture load). Removing the whole file is the structurally honest deletion.
- **`comment-discipline.md` — no history narration.** New comment at `LightPass.cpp:168-170` ("HLSL register s0 unused after the cube-atlas linear sampler was retired; keeping the register vacant avoids cascading renumbers across the surviving sampler set") follows the *existing* `b3` / `t7` keep-vacant pattern in the same file (`LightPass.cpp:64-66, 120-121`). It's a precedent-citing rationale comment, not a "what changed" narration — same shape as the comments it joins. PASS.
- **`safety-observability.md` — guard clauses log loudly.** Existing `Log(Warning, ...)` guards at `LightPass.cpp:251-279` untouched; no new silent guards introduced.
- **`tech-choice-vs-default.md`.** Brief mandates option (c) "TASK-138 precedent — delete outright, no fallback retained" — implementer followed it. `lightPass.comp` carries the precedent comment at `:43-45` and `:73-74` for the b3/t7 vacancies; the new s0 comment extends the same pattern.

### Defects implementer is least primed to see

- **`audit_03b_PointShadowAtlas.hdr` removal at `ExampleRenderingClient.cpp:980-986`.** Replaced with a one-line comment ("3: Sun shadow R8 visibility texture from SunShadowRTPass."). The audit dump was the only consumer of `PointShadowGeometryProcessPass::GetResult()` outside LightPass — no orphan helper code remains.
- **No other consumers of cube atlas.** `Grep` over the full source tree: zero non-CL2-territory references to `PointShadow` outside the deleted files. No debug visualizer, no editor inspector panel reads atlas slot, no scene `.json` references it.
- **WIP shader `volumetricIrraidanceInjectionPass.comp:45`** retains a `//Lo *= 1.0 - SunShadowResolver(...)` commented-out call with no enclosing function definition (the file is shelved). Not a compile dependency. Advisory only — see below.

### Advisory (non-blocking)

- **A1.** `Source/Shaders/HLSL/WIP/volumetricIrraidanceInjectionPass.comp:45` and `Source/Shaders/HLSL/WIP/GIResolveSurfelPass.comp:23, 127` reference `SunShadowResolver` (former resident of the just-deleted `shadowResolver.hlsl`). These WIP files are shelved (not built into a binary today), so it is not a compile defect. Worth a follow-up grep cleanup when the WIP shaders are revived; out of CL1 scope.
- **A2.** Comment at `LightPass.cpp:168` says "s1 - Sampler point (HLSL register s0 unused…)". Reads as if the slot label has changed; in fact the binding *descriptor-index* is 1 (s1) and the *HLSL register* s0 is unused — same wording the existing `b3 unused`/`t7 unused` comments use, so it is consistent with prior art. No action.

### Rolled-up verdict

Diff is a clean structural deletion. CL1 scope completes; CL2 (engine-common) is correctly deferred. Binding-layout integrity verified C++ ↔ HLSL. No anchored-invariant drift. Implementer may commit; record `Reviewed-By: graphics-api-expert` in the commit message.
<!-- SECTION:NOTES:END -->
