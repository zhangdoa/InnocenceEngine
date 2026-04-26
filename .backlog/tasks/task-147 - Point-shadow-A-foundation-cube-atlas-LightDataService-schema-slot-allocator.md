---
id: TASK-147
title: 'Point shadow A: foundation — cube atlas + LightDataService schema + slot allocator'
status: To Do
assignee: []
created_date: '2026-04-26 22:30'
updated_date: '2026-04-26 22:30'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
  - infrastructure
dependencies:
  - TASK-66-design
parent_task_id: TASK-66
priority: high
references:
  - Source/Engine/Services/LightDataService.cpp
  - Source/Engine/Services/LightDataService.h
  - Source/Engine/Services/RenderingConfigurationService.h
  - Source/Engine/Common/GPUDataStructure.h
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-66 (point/sphere light shadows in rasterized pipeline).** Owner: `graphics-api-expert`.

Foundation infrastructure that unblocks the caster pass (TASK-148) and the component flag work (TASK-149). No rendering output by itself; this is plumbing.

### What this subtask delivers

1. **Cube-array shadow atlas allocation** — a single `Texture2DArray` with `DepthOrArraySize = maxPointShadows * 6` slices, mirroring the sun-shadow color-target idiom (`SunShadowGeometryProcessPass.cpp:44-48`). This is allocated as part of the new `PointShadowGeometryProcessPass` render-pass setup, but the atlas resource lifetime + descriptor wiring + barrier policy is the graphics-api scope.
2. **`PointShadowConstantBuffer` schema** in `Source/Engine/Common/GPUDataStructure.h`. Exact shape decided by the design call (see Implementation Notes — block until rendering-researcher publishes their recommendation), but minimally: per-shadowed-light, the 6 cube-face view+proj matrices and the atlas base slot index. Mirror the existing `CSMConstantBuffer` alignment idiom (`alignas(16)`, padding to canonical size).
3. **`LightDataService::GetPointShadowBuffer()`** + an internal `m_PointShadowCBVector` populated each frame from the LightComponent storage. Mirrors `m_CSMCBVector` / `GetCSMBuffer()` in `LightDataService.cpp:51,55,360-363`.
4. **Atlas slot allocator** — simple per-frame pack: walk shadow-casting point/sphere lights in deterministic order, assign slots `[0..N-1]`. No persistence across frames in this subtask (defer LRU / temporal coherence to a follow-up).
5. **`RenderingCapability::maxPointShadows`** field in `RenderingConfigurationService.h` next to `maxPointLights/maxCSMSplits`. Default value comes from the design call; the surfacing producer's analysis of "32 × 6 × 256² D32 ≈ 12 MB" is the budget anchor — `rendering-researcher` confirms quality target, this subtask confirms VRAM fit and lands the constant.

### What this subtask does NOT do

- No caster shaders, no LightPass binding, no shadowResolver extension — those are TASK-148.
- No `m_CastShadow` flag on `LightComponent` — that is TASK-149. This subtask exposes a contract ("the slot allocator reads `lightCfg.castShadow` if available, otherwise treats all point/sphere lights as shadow-casters") that TASK-149 then satisfies.
- No editor / serialization changes.

### Project invariants (anchor — read before designing)

- **Sun shadow precedent**: VSM-style packed `(depth, depth², 0, 1)` color attachment (`SunShadowGeometryProcessPass.cpp:53-59`, `sunShadowGeometryProcessPass.frag:61`). `Texture2DArray` is the canonical multi-slice shadow representation in this codebase; do not introduce TextureCube unless the design call explicitly mandates it.
- **Atlas resource type** depends on the design call (depth-only D32 for hardware-PCF vs. color-target VSM for moment-PCSS). Whichever it is, it must follow the existing `RenderPassDesc` idiom (`m_RenderTargetDesc` / `m_UseDepthBuffer`).
- **Service Update() ordering**: `LightDataService::Update()` runs each frame and uploads light + CSM buffers (`LightDataService.cpp:276-305`). Point-shadow buffer upload must follow the same pattern; if the cbuffer needs the camera frustum (e.g. for slot prioritization later), check `CameraService` ordering.
- **No magic numbers**: `maxPointShadows` is the canonical capacity; no inline `6` for face count except in geometry-shader code where it is structurally clear.

### Why this is foundation-first

Caster pass (TASK-148) needs the constant buffer to know face matrices + atlas base slot. Component flag (TASK-149) needs the contract that "shadow-casting lights have an atlas slot index". Sequencing this subtask first lets the other two go in parallel.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `RenderingCapability::maxPointShadows` field present, default value matches the design call from TASK-66 design audit
- [ ] #2 `PointShadowConstantBuffer` declared in `Source/Engine/Common/GPUDataStructure.h` with shape consistent with the design call (face matrices + atlas base slot at minimum)
- [ ] #3 `LightDataService::GetPointShadowBuffer()` returns a populated `GPUBufferComponent`; populated each frame from the light storage; size ≤ `maxPointShadows`
- [ ] #4 Cube atlas resource allocated (resource type per design call: D32 depth or float2 color); descriptor / barrier wiring verified — engine starts without GBV errors
- [ ] #5 Engine launches GISponza windowed without regression; no shadow output yet (no caster pass), but no crash, no GBV warnings
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Sequencing**: BLOCKED on the TASK-66 design call (VSM vs PCF on cube atlas; atlas resolution + slot count). `rendering-researcher` resolves both and amends this task before implementation begins. Until then the AC#1/AC#2 details are placeholders.

**Build-cache hygiene**: TASK-146 is in flight (mirror-semantic shader deploy). Until it lands, every iteration on this subtask must manually nuke `Bin/Shaders/DXIL/` + `Bin/RelWithDebInfo/Shaders/DXIL/` and rerun `Build/HLSL2DXIL_NoPause.ps1` before `cmake --build`. See `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination — bisect prerequisite".

**Prior art to mirror**:
- `SunShadowGeometryProcessPass::Setup()` for render-target descriptor shape (texture-array, packed depth color, viewport sizing, comparison function).
- `LightDataServiceImpl::UpdateCSMData()` for the per-frame matrix-vector populate idiom and `Upload()` call.
- The 6-face matrix array of `GIConstantBuffer.r[6]` / `v_inv[6]` (`GPUDataStructure.h:113,116`) as cbuffer-shape precedent for "6 face transforms in one constant".
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (engine + shader tier)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — engine launches GISponza windowed without regression; screenshot / RenderDoc capture proving no crash and no GBV warnings; atlas resource visible in capture
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
