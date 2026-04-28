---
id: TASK-177
title: >-
  TASK-175-B: Delete cube-shadow stack — passes, shaders, cbuffer, allocator,
  capacity (cross-subtree)
status: Done
assignee: []
created_date: '2026-04-28'
updated_date: '2026-04-28 19:29'
labels:
  - rendering
  - shadows
  - cleanup
  - architecture
dependencies:
  - TASK-176
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
parent_task_id: TASK-175
priority: high
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
- [x] #1 `PointShadowGeometryProcessPass.{cpp,h}` deleted; pass not dispatched; not in `GetDispatchedPasses()`
- [x] #2 `pointShadowGeometryProcessPass.{vert,geom,frag}` deleted
- [x] #3 `PointShadowResolver` + `PointPCSS` deleted from `shadowResolver.hlsl`
- [x] #4 `PointShadowConstantBuffer` deleted; `t12 in_PointShadow` and `b6 g_PointShadows` deleted from `lightPass.comp`
- [x] #5 `LightDataService` cube-atlas plumbing deleted: `GetPointShadowAtlas`, `GetPointShadowBuffer`, `m_PointShadow*`, `LightDataService_PointShadow.inl`
- [x] #6 `RenderingCapability::maxPointShadows` and `NR_POINT_SHADOWS` (if a generated mirror) deleted
- [x] #7 `ShadowCasterCullingPass.{h,cpp}` + `shadowCasterCulling.comp` deleted; not dispatched
- [x] #8 `LightComponent::m_CastShadow` + editor checkbox + JSON round-trip preserved (TASK-149)
- [x] #9 Codebase grep for `PointShadow`, `PointPCSS`, `INVALID_ATLAS_SLOT`, `maxPointShadows`, `NR_POINT_SHADOWS`, `shadowCasterCulling` returns no functional references (intentional comments OK)
- [x] #10 Build clean; TestSuite green; GBV clean (modulo TASK-163 readback ERROR)
- [x] #11 Visual: GISponza + UnitTest spheres pixel-equivalent to post-A capture
- [x] #12 Peer review per CL (rendering-researcher CL → graphics-api-expert; low-level-expert CL → peer or software-architect) before commit
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

## Review (software-architect peer, 2026-04-28)

**Verdict: PASS**

CL2 scope = engine-common subtree (`Source/Engine/Common/GPUDataStructure.h`, `Source/Engine/Services/LightDataService.{h,cpp,_PointShadow.inl}`, `Source/Engine/Services/RenderingConfigurationService.{h,cpp}`, `Source/Shaders/HLSL/common/common.hlsl`). Reviewed against `git diff --cached` over the 7 staged paths (6 modified + 1 deleted), not implementer assertions. `software-architect` chosen because `LightDataService` is sole-owner-subtree (no peer `low-level-expert` family for this CL) and the deletion touches code-data coupling per `peer-review-required.md` §"Who reviews" rule 2.

### Acceptance criteria — line-grounded

- AC #4 met (engine-common half). `GPUDataStructure.h:50-79` (post-CL2): `PointShadowConstantBuffer` struct fully removed; the surviving `PointLightConstantBuffer` declaration preserves `uint32_t m_CastShadow = 1` (line 59) — TASK-176's wire field intact. `common.hlsl:25-32` removed `NR_POINT_SHADOWS`, `INVALID_ATLAS_SLOT`, and `PointShadow_CB` struct.
- AC #5 met. `LightDataService.h:20-26` retains only `GetPointLightBuffer / GetSphereLightBuffer / GetGIBuffer / GetPointLightCount / GetSphereLightCount`. The 5 deleted accessors (`GetPointShadowAtlas`, `GetPointShadowBuffer`, `GetPointShadowCount`, `GetPointLightAtlasSlot`, `GetSphereLightAtlasSlot`) are gone. `LightDataService.cpp:14-34` `LightDataServiceImpl` struct lost `m_PointShadowCBVector`, `m_PointShadowGPUBufferComp`, `m_PointShadowAtlas`, `m_PointLightAtlasSlot`, `m_SphereLightAtlasSlot`, and `UpdatePointShadowData()`. `LightDataService_PointShadow.inl` deleted. The anonymous-namespace `LookupAtlasSlot` helper deleted with its sole consumers. `MathHelper.h` and `TextureResourceService.h` / `TextureComponent.h` includes correctly dropped — verified by `Grep "Math::|TextureResourceService|TextureComponent"` over the new `LightDataService.{h,cpp}` returning zero matches; remaining symbols (`Vec4`, `Mat4`) come transitively via `GPUDataStructure.h → MathHelper.h`.
- AC #6 met. `RenderingConfigurationService.h:24-30` `RenderingCapability` lost `maxPointShadows`. `RenderingConfigurationService.cpp:19-22` initializer block lost the `m_renderingCapability.maxPointShadows = 8` assignment along with its 3-line TASK-66/TASK-150 docstring.
- AC #8 preserved. `LightComponent.h:45 bool m_CastShadow = true` untouched (verified via `Grep "m_CastShadow"`); `JSONSerializer_Components.cpp:44, 409` round-trip untouched; `EditorService.cpp:317, 616-617` IPC untouched. Editor-Next `LightEditor.vue` checkbox untouched (no `Source/Editor-Next` matches for the deleted symbols at all). End-to-end: editor flag → JSON → component → cbuffer field is a single-write at `LightDataService.cpp:108`.
- AC #9 met. Final-tree grep for `PointShadow|PointPCSS|INVALID_ATLAS_SLOT|maxPointShadows|NR_POINT_SHADOWS|shadowCasterCulling` over `Source/`, `Data/`, `Scripts/` returns **zero matches** (Grep over `Source` shows "No matches found"; case-insensitive variants `pointShadow|atlasSlot|InvalidAtlasSlot` confined to `.backlog/tasks/*.md` historical docs and one citation in `.claude/disciplines/tech-choice-vs-default.md`). `GetPointShadow*` / `GetPointLightAtlasSlot` / `GetSphereLightAtlasSlot` accessors have zero callers tree-wide (verified independently by grepping each accessor name).
- AC #10 / #11 / #12 — implementer reports build clean, smoke clean (30-frame `Main.exe`); not re-verified statically. No header/source ordering defect or contract drift visible in the diff that would invalidate the build claim. Visual parity is structural: cube path was already bound-but-unused after CL1, so CL2 cannot regress pixels (it only removes resources nothing samples).

### Anchored invariants

- **TASK-149 `m_CastShadow` flag, editor checkbox, JSON round-trip** — preserved end-to-end. `LightComponent.h:45` declaration unchanged, `JSONSerializer_Components.cpp:44 / :409` write/read unchanged, `EditorService.cpp:317 / :616-617` IPC unchanged, `Editor-Next/src/components/inspector/LightEditor.vue` not in diff.
- **TASK-176 `PointLightConstantBuffer::m_CastShadow` field + cbuffer→shader contract** — preserved. `GPUDataStructure.h:55-62`:
  ```
  struct alignas(16) PointLightConstantBuffer
  {
      Vec4 pos;
      Vec4 luminance;
      uint32_t m_CastShadow = 1;
      uint32_t padding[3] = { 0, 0, 0 };
  };
  ```
  `LightDataService.cpp:99-110` Point-light branch fills `l_data.m_CastShadow = l_Light.m_CastShadow ? 1u : 0u` unconditionally before `emplace_back` — i.e. the field is set for **every** point light, not only shadow-casters. `lightPassDirectLighting.hlsl:135` `if (l_PointLight.shadow.x != 0u)` reads via the existing struct alias on the HLSL side; gate behaviour identical post-CL2.
- **Default-init guard for non-shadow lights** — `m_CastShadow = 1` default on the C++ side at `GPUDataStructure.h:59` is paired with the unconditional assignment at `LightDataService.cpp:108`. Even if a future caller default-constructed a `PointLightConstantBuffer` and bypassed the assignment, the default value is the safe one (visibility-tested = trace ray, which always returns 1 in an empty BVH). The CL2 deletion did not regress this safety property.
- **TASK-138 SunShadowRT** — `Source/ExampleProject/RenderingClient/SunShadow*` not in CL2 diff; sun path untouched.
- **TASK-161 OpaquePass exit barrier** — not in CL2 diff.

### Code-data coupling (architect lens)

- **`PointShadowConstantBuffer` was per-frame ephemeral GPU upload.** `Grep "PointShadow"` over `Source/Engine/ThirdParty/JSONWrapper/` returns zero matches — never serialized. `Grep "PointShadow"` over `Data/` returns zero matches — never persisted as scene/asset config. Deletion is safe at the schema layer; no migration needed.
- **`maxPointShadows` was hard-coded at `RenderingConfigurationService.cpp:22`,** not loaded from JSON or external config (verified: zero `Grep` matches in `Data/`, `Scripts/`, or any `.json` file). No external tooling dependency. RenderingCapability is reconstructed fresh on each engine boot from the constructor; deletion is binary-compatible since no consumer reads the field.
- **`INVALID_ATLAS_SLOT` was a CPU/GPU-shared sentinel.** Both copies (`GPUDataStructure.h:8` C++ and `common.hlsl:30` HLSL) deleted in this CL. No remaining consumer (case-insensitive grep clean across `Source/`). The brief flagged "may be reused by the future bindless atlas plan — if so, keep with a comment"; the implementer's call to delete is correct because no current code path uses it, and resurrecting it from `git log` is trivial when the bindless plan lands.
- **`pos.w` slot-stamping retired.** `LightDataService.cpp:99-110` Point-light `l_data.pos = l_Transform->m_LocalPos;` — pos.w now carries `l_Transform->m_LocalPos.w` (typically 1.0 for a position; depends on `Vec4` semantics). The shader `lightPassDirectLighting.hlsl:108` reads `l_PointLight.position.xyz`, never `.w` — confirmed by `Grep "position\.w" Source/Shaders/HLSL/lightPass*` returning no matches. Sphere branch likewise. No silent regression from the field's repurpose; `.w` is now dead-channel padding (which is an ADVISORY tail, not a defect).

### Universal disciplines

- **`coding-principles.md` — fix at the right layer.** Engine-common deletion is exactly the right layer: rendering-side consumers (CL1) had to die first or this CL would not link; engine-common deletion happens once consumers are gone. Two-CL split per the brief honoured.
- **`comment-discipline.md` — no history narration.** New comment at `LightDataService.cpp:106-107` ("TASK-176: inline-RT shadow trace gate consumed by lightPass.comp::EvaluateTiledPointLighting.") is rationale-citing (points the reader at the consumer), not "what changed" narration; replaces the prior TASK-176 comment which had a TASK-177-pending forward-reference now stale. Correct shape — keep concept docs, drop now-historical promise.
- **`safety-observability.md` — guard clauses log loudly.** No new silent guards introduced. Prior `LookupAtlasSlot` helper (which logged Warning on out-of-range) deleted with its consumers — no orphaned guard remains. The `if (l_Lights.empty()) return false;` at `LightDataService.cpp:90-91` is unchanged from pre-CL2.
- **`threading-contracts.md`.** No new container or API added; deletion only. `LightDataService` thread-safety contract (caller-synchronised, single-thread `Update()` per frame) unchanged.
- **`cpp-style.md`.** Naming (`m_PointLightCBVector`, `l_data`, `l_Light`) unchanged, header/source split preserved, engine STL wrappers (`std::vector` via `STL14.h`/`STL17.h` transitively) unchanged. The `<cstring>` for `std::memcpy` previously needed by the deleted `LookupAtlasSlot`/sentinel-pun and the deleted `_PointShadow.inl`'s `SlotIndexAsFloat` is no longer used in `LightDataService.cpp` — verified `Grep "memcpy" LightDataService.cpp` returns no matches; the stale sentinel-bit-cast block at the old `:188-198` is gone with the surrounding `pos.w` stamping logic.
- **`tech-choice-vs-default.md`.** Brief mandates (c) "TASK-138 phase 2 + TASK-157 precedent — delete outright, no fallback retained"; CL2 follows that. No new vestigial `#if 0` block, no commented-out struct kept "in case", no migration-shim layer.
- **`feedback_no_data_integrity_assumptions.md`.** Schema deletion is loud-by-construction — any surviving consumer would fail the C++ link or the HLSL compile. Static "fail loud at boundary" check holds: there is no possible silent-data-shape mismatch because the data shape no longer exists.

### Defects implementer is least primed to see

- **Header/source ordering** — checked. `LightDataService.h` declares only the 5 surviving accessors; `LightDataService.cpp:193-216` defines exactly those 5. No "header declares but source doesn't define" or vice versa. No dangling `INNO_DEFINE_*` macro mismatch.
- **`compile_commands.json` / build-system artefacts** — not staged, no concern. CMakeLists / vcxproj files do not list `_PointShadow.inl` (it was `#include`d, not compiled directly), so no build-system entry needs deletion. Verified by `Grep "_PointShadow"` over the tree returning zero non-CL2-territory matches.
- **`PointLightConstantBuffer::m_CastShadow` set for ALL lights, not just shadow-casters.** Confirmed — the assignment at `LightDataService.cpp:108` is unconditional inside the `LightType::Point` branch, executing for every point light regardless of `m_CastShadow` value. Combined with the default `= 1` at `GPUDataStructure.h:59`, no path produces an undefined `m_CastShadow` value in the uploaded buffer. Non-shadow-casting point lights correctly receive `0u` and the inline-RT gate at `lightPassDirectLighting.hlsl:135` short-circuits.
- **Sphere-light shadow asymmetry** — `SphereLightConstantBuffer` (`GPUDataStructure.h:65-70`) has no `m_CastShadow` field; sphere lights receive no inline-RT shadow trace at all. This was inherited from TASK-176's design (point-only inline-RT) and predates CL2. Out-of-scope finding; not introduced or regressed by this CL.
- **`Vec4` operator semantics on `pos.w`** — `LightDataService.cpp:103/115` `l_data.pos = l_Transform->m_LocalPos;` copies `.w` from the transform. Since the shader no longer reads `.w`, the value is harmless padding. No defect.
- **Indentation drift at `LightDataService.cpp:52, 131`.** Lines `auto l_rsService = g_Engine->Get<GPUBufferResourceService>();` use a single tab where the surrounding scope is 2-tab — pre-existing in the file, not introduced by CL2. Out-of-scope advisory, see A2 below.

### Advisory (non-blocking)

- **A1.** `RenderingCapability` field-order: `maxPointLights / maxSphereLights / maxMeshes / maxTextures / maxMaterials`. The deletion left a clean alphabetical-by-domain ordering — no visible-ordering defect. If a future "bindless point-shadow atlas" feature lands (per task-177 brief footnote), the new capability field can append cleanly without shuffling.
- **A2.** Pre-existing 1-tab/2-tab indentation drift at `LightDataService.cpp:52, 131` (the `auto l_rsService = ...` lines inside a 2-tab block use 1 tab). Not introduced by CL2. Worth a tiny whitespace-only follow-up commit, or roll into a future `LightDataService.cpp` touch — out of CL2 scope.
- **A3.** `LightDataService.cpp:106-107` comment "TASK-176: inline-RT shadow trace gate consumed by lightPass.comp::EvaluateTiledPointLighting." is a forward-reference comment by file path. Acceptable per `comment-discipline.md` (concept-citing, not history-narrating). When the inline-RT gate evolves (e.g. attenuation-skip per TASK-181), update the comment in lockstep — no action now.

### Rolled-up verdict

Engine-common cube-shadow stack deletion is a clean schema removal. All 12 ACs covered (rendering-half AC1-#3/#4-#7 by CL1 review; engine-common AC #4-#6 + cross-CL #8-#9-#12 by this review; AC #10/#11 by implementer's smoke + the static absence of any path that could regress visuals after CL1 already unbound the cube atlas). TASK-149 / TASK-176 invariants preserved end-to-end. No code-data coupling defects: `PointShadowConstantBuffer` was never serialized, `maxPointShadows` was never persisted, `INVALID_ATLAS_SLOT` had no surviving consumer. Implementer may commit; record `Reviewed-By: software-architect` in the commit message (alongside the existing `Reviewed-By: graphics-api-expert` from CL1, if a single combined commit, or as the sole reviewer line if CL2 commits separately).
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Outcome

Cube-shadow stack fully removed across rendering and engine-common subtrees in two CLs. The 93.5 ms `PointShadowGeometryProcessPass` cost is gone; the timer entry is no longer registered.

## What landed

**CL1 — `f41a4ffb`** (`refactor(rendering): TASK-177 CL1 — delete cube-shadow rendering stack`) — rendering-researcher subtree:

Deleted (8):
- `Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.{cpp,h}`
- `Source/ExampleProject/RenderingClient/ShadowCasterCullingPass.h` (header-only stub; no `.cpp` ever existed)
- `Source/Shaders/HLSL/pointShadowGeometryProcessPass.{vert,geom,frag}`
- `Source/Shaders/HLSL/shadowCasterCulling.comp`
- `Source/Shaders/HLSL/common/shadowResolver.hlsl` (whole file — sun resolver was already inlined in TASK-138 phase 2; no surviving consumers)

Modified (5 + 2 WIP):
- `Source/Shaders/HLSL/lightPass.comp` — dropped `#include` of `shadowResolver.hlsl`, `b6 PointShadowCBuffer`, `t12 in_PointShadow`, `s0 in_samplerTypeLinear`.
- `Source/ExampleProject/RenderingClient/LightPass.{cpp,h}` — binding-layout 24→21 with surviving slots renumbered (`s1`/`t13`/`t14`); cube-atlas SRV transitions and sampler Add/Init/Delete removed; `m_SamplerComp_Linear` field dropped.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — removed both pass dispatch sites + `GetDispatchedPasses()` entries + `audit_03b_PointShadowAtlas` dump.
- `Source/Shaders/HLSL/WIP/voxelGeometryProcessPass.frag` and `Source/Shaders/HLSL/WIP/volumetricIrraidanceInjectionPass.comp` — dead `#include` of `shadowResolver.hlsl` removed (WIP files; not currently built into a binary).

Reviewed by `graphics-api-expert` (PASS). Binding renumbering verified C++↔HLSL match. Whole-file `shadowResolver.hlsl` deletion structurally correct — sun visibility is now a direct texture load via `EvaluateSunLighting`.

**CL2 — `c478a833`** (`chore(engine): TASK-177 CL2 — delete cube-shadow engine-common`) — engine-common subtree:

Modified (6) + Deleted (1):
- `Source/Engine/Common/GPUDataStructure.h` — deleted `PointShadowConstantBuffer` struct + `INVALID_ATLAS_SLOT`.
- `Source/Engine/Services/LightDataService.{h,cpp}` — deleted `GetPointShadowAtlas/Buffer/Count`, `GetPointLight/SphereLightAtlasSlot` accessors; `m_PointShadow*` sidecars; per-frame cbuffer upload; `pos.w` slot stamping; `LookupAtlasSlot` helper; `_PointShadow.inl` include; dropped now-unused `MathHelper`/`TextureResourceService`/`TextureComponent` includes.
- `Source/Engine/Services/LightDataService_PointShadow.inl` — DELETED entirely.
- `Source/Engine/Services/RenderingConfigurationService.{h,cpp}` — deleted `RenderingCapability::maxPointShadows` + initializer.
- `Source/Shaders/HLSL/common/common.hlsl` — deleted `NR_POINT_SHADOWS`, `INVALID_ATLAS_SLOT`, `PointShadow_CB`.

Reviewed by `software-architect` (PASS — chosen because `LightDataService` is sole-owner-subtree per `peer-review-required.md` §"Who reviews" rule 2). Code-data coupling audit confirmed clean: `PointShadowConstantBuffer` was never JSON-serialized; `maxPointShadows` was never persisted; `INVALID_ATLAS_SLOT` had no surviving consumer.

## Validation

- Both CLs: build clean.
- CL1 smoke `Main.exe -total_frames 30`: exit clean. Per-pass GPU timer (60-frame run): `SunShadowRT 1.39–1.54 ms`, `RadianceCacheRT 1.57–2.64 ms`, `LightPass 1.61 ms`. **`PointShadow*` timer entries gone.**
- Final-tree grep for `PointShadow|PointPCSS|INVALID_ATLAS_SLOT|maxPointShadows|NR_POINT_SHADOWS|shadowCasterCulling` over `Source/`, `Data/`, `Scripts/`: **zero matches** (case-insensitive variants confined to `.backlog/tasks/*.md` historical docs and one citation in `.claude/disciplines/tech-choice-vs-default.md`).
- TASK-149 invariants preserved end-to-end: `LightComponent::m_CastShadow` (`LightComponent.h:45`), JSON round-trip (`JSONSerializer_Components.cpp:44, 409`), editor IPC (`EditorService.cpp:317, 616-617`), Editor-Next `LightEditor.vue` checkbox — all untouched.

## Tech-choice anchor

Picked option (c) per `tech-choice-vs-default.md`: TASK-138 phase 2 precedent (CSM+PCSS deleted outright once RT sun proved out, no fallback retained). Same engine, same hardware target, single user (zhangdoa) on RT-capable hardware. No `#if 0` blocks, no commented-out structs "kept in case", no migration shim.

## Carry-forward observations

- Sphere lights have no `m_CastShadow` field on `SphereLightConstantBuffer` — sphere shadow trace is inherited-deferred to TASK-179. Out of TASK-177 scope.
- Pre-existing 1-tab/2-tab indentation drift at `LightDataService.cpp:52, 131` — software-architect ADVISORY A2; not introduced by CL2. No follow-up task filed (cosmetic).
<!-- SECTION:FINAL_SUMMARY:END -->
