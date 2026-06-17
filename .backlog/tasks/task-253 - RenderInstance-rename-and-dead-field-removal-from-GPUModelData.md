---
id: TASK-253
title: >-
  Rename GPUModelData → RenderInstance; remove 5 dead fields (TASK-252 prerequisite)
status: Done
assignee:
  - code-impl
created_date: '2026-06-17'
labels:
  - rendering
  - refactor
  - cleanup
  - follow-up
dependencies:
  - TASK-252
priority: high
---

## Description

Smaller-scope refactor that ships first as a prerequisite for
TASK-252 (the full MeshGeometry / RenderInstance / IndirectDrawCommand
decomposition). Rename `GPUModelData` to `RenderInstance` and delete
the 5 dead fields that the validator / manual inspection surfaced.

### The 5 dead fields (write-only, never read by HLSL)

| Field | C++ offset | HLSL usage | Status |
|---|---|---|---|
| `m_ShaderProgramIndex` | 36-39 | struct-only, never read | DEAD |
| `m_UUID` (float) | 40-43 | struct-only, never read; also **corrupts** for EntityIDs > 2^24 | DEAD |
| `m_RenderPassIndex` | 44-47 | struct-only, never read | DEAD |
| `m_VisibilityMask` | 48-51 | struct-only, never read | DEAD |
| `m_MeshUsage` | 52-55 | struct-only, never read | DEAD |

All five are produced (the producer writes them every frame, mostly
to 0 or to a static enum) but consumed by **nothing**. The producer's
writes are pure overhead. Removing them:

- Shrinks the struct from 160B to 144B (5 × 4B removed, then rounded
  to a multiple of 16B by the existing `alignas(16)`).
- Cuts the per-frame upload size by 10% (for 56 instances/frame: 56 ×
  16B = ~900B saved per frame).
- Eliminates a whole class of "field is poison, but HLSL never
  reads it anyway" noise from the validator log.
- Makes the remaining field layout **read-once from the HLSL** — no
  fields that are write-only-on-the-CPU-side.

### The rename

`GPUModelData` is vague — it doesn't say what the struct *is* in
the dataflow. After this CL it becomes `RenderInstance`, which is
what it actually is: a per-visible-instance record consumed by the
cull pass. (In TASK-252 it gets split further into `MeshGeometry` +
`RenderInstance`; this CL does the rename only.)

Renames required:
- C++ struct definition: `Source/Engine/Common/GPUDataStructure.h`
- C++ producer: `Source/Engine/Services/DrawCallService.cpp`
- C++ service API: `DrawCallService::GetGPUModelData()` →
  `GetRenderInstances()`, `GetGPUModelDataBuffer()` →
  `GetRenderInstanceBuffer()`
- C++ buffer component names: `m_GPUModelDataBufferComp` →
  `m_RenderInstanceBufferComp`; the runtime label `"GPUModelDataBuffer"`
  → `"RenderInstanceBuffer"`
- C++ consumer: `PerFrameDataService_Impl.cpp:160` (uses
  `GetGPUModelData().size()` for `modelCount`; the field name on
  the CB is unchanged but the call site is)
- C++ consumer: `DX12FrameManagementService_Draw.cpp:168` (uses
  `GetGPUModelData().size()` for indirect-draw budget calc)
- C++ consumer: `RenderGraphPassRecorder.cpp:27` (uses
  `GetGPUModelData().size()` for tile dispatch)
- HLSL struct: `Source/Shaders/HLSL/common/common.hlsl:530` →
  `RenderInstance` (with the same field set minus the 5 dead ones)
- HLSL `StructuredBuffer<GPUModelData>` → `StructuredBuffer<RenderInstance>`
  in `opaqueGPUCulling.comp:13` and `opaqueGeometryProcessPass.frag:23`

The HLSL rename triggers a recompile of every DXIL that uses the
struct (~2 shaders). The runtime "buffer by name" wiring uses string
labels, so the rename is mechanical.

### Acceptance Criteria

- [ ] C++ struct `GPUModelData` renamed to `RenderInstance` in
      `Source/Engine/Common/GPUDataStructure.h`.
- [ ] Five dead fields removed from C++ struct:
      `m_ShaderProgramIndex`, `m_UUID`, `m_RenderPassIndex`,
      `m_VisibilityMask`, `m_MeshUsage`.
- [ ] C++ producer in `DrawCallService.cpp` no longer references
      the 5 dead fields; the producer body shrinks by ~5 lines.
- [ ] C++ service API renamed: `GetGPUModelData()` →
      `GetRenderInstances()`, `GetGPUModelDataBuffer()` →
      `GetRenderInstanceBuffer()`.
- [ ] HLSL struct `GPUModelData` renamed to `RenderInstance` in
      `Source/Shaders/HLSL/common/common.hlsl`.
- [ ] HLSL `StructuredBuffer<GPUModelData>` → `StructuredBuffer<RenderInstance>`
      in `opaqueGPUCulling.comp` and `opaqueGeometryProcessPass.frag`.
- [ ] Buffer name `"GPUModelDataBuffer"` → `"RenderInstanceBuffer"`
      in the resource service registration.
- [ ] `static_assert(sizeof(RenderInstance) == 144)` (down from
      160 for `GPUModelData`).
- [ ] All consumers updated: `PerFrameDataService_Impl.cpp`,
      `DX12FrameManagementService_Draw.cpp`,
      `RenderGraphPassRecorder.cpp`.
- [ ] Build green. TestSuite unit tests pass. Live smoke reaches
      steady state (56 instances, deferred queue empty).
- [ ] Validator is silent for `RenderInstance` (no unwritten fields).
- [ ] No "GPUModelData" identifier remains in the codebase
      (grep -r "GPUModelData" Source/ returns 0 matches).
- [ ] No "m_UUID" reference to a float in the engine CBs
      (the `Object::m_UUID` uint64 is unrelated; only the float
      one in `GPUDataStructure.h` is the target).

### What is NOT in scope (and why)

- **TASK-252 (the full mesh/instance/draw-args split)** is the
  follow-up. This task only does the rename + dead-field removal;
  the struct still has 9 fields of mixed-lifetime data after this CL.
- **The functional-core / imperative-shell refactor** for the
  producer (no conditionals at the build site) is part of TASK-252.
  This CL keeps the producer's `if (l_world)` fallback, just
  renames the struct.
- **The all-black frame** is a separate bug (TASK-163 / TASK-241).
  This CL does not fix it.
- **The float-cast `m_UUID` precision loss for EntityIDs > 2^24** is
  resolved by removing the field. The CPU-side debug path that
  used `m_UUID` (if any) needs to use `Object::m_UUID` (uint64) or
  the EntityID directly. Audit the call sites during the rename.

### Investigation hints

- `grep -rn "GPUModelData" Source/` will list every callsite.
  Expected hits: `GPUDataStructure.h`, `DrawCallService.h`,
  `DrawCallService.cpp`, `PerFrameDataService_Impl.cpp`,
  `DX12FrameManagementService_Draw.cpp`, `RenderGraphPassRecorder.cpp`,
  `common.hlsl`, `opaqueGPUCulling.comp`,
  `opaqueGeometryProcessPass.frag`.
- `grep -rn "m_UUID" Source/Engine/Common/GPUDataStructure.h`
  should now be 0 (was 1).
- The `m_GPUModelDataBufferComp` buffer registration uses
  `l_rsService->Add("GPUModelDataBuffer")` — string label. The
  rename here matters for the resource service's name-based
  lookup.
- The `maxMeshes` buffer capacity is unchanged. The
  `m_ElementSize = sizeof(GPUModelData)` line becomes
  `sizeof(RenderInstance) = 144`.

### Related

- TASK-249 (the GPUUploadable rollout; the validator that
  surfaced the dead fields).
- TASK-250 (`m_ShaderProgramIndex` / `m_RenderPassIndex` not
  populated; this task *removes* those fields entirely instead
  of fixing them).
- TASK-252 (the full mesh/instance/draw-args split; this task is
  its prerequisite).
- Session 2026-06-17c (the user feedback that the god-struct and
  the name "GPUModelData" needed a redesign).

## Session log

- **2026-06-17 (filed):** Filed as TASK-252's prerequisite. The 5
  dead fields surfaced by the validator in TASK-249 are the
  immediate cleanup target; the rename to `RenderInstance` is
  the visible signal that this is a focused per-frame record, not
  a generic GPU dump. Ships as one CL.
- **2026-06-17 (landed):** rename + dead-field removal shipped.
  `RenderInstance` is 144B (was `GPUModelData` 160B); the 5 dead
  fields removed from both C++ and HLSL. **Critical missed-callsite
  learning: the buffer name lives in THREE places, not one.**
  (1) C++ `DrawCallService::Setup` (`l_rsService->Add("...")`),
  (2) the HLSL struct + `StructuredBuffer<...>` declarations, and
  (3) **`Data/ExampleProject/RenderGraph/ExampleRenderGraph.json`**
  — the render graph imports the buffer by name (`"Imported": true`)
  and binds it to two passes. Missing #3 produced a runtime crash:
  `RenderGraphService::ResolveImportedResource: imported resource
  [GPUModelDataBuffer] not found` → null `GPUResourceComponent` →
  `As<GPUBufferComponent>()` deref of null at GPUResourceCast.h:20.
  ALSO: the JSON deploy is a CMake POST_BUILD step on a runtime
  target; a JSON-only edit does NOT trigger it, so the
  `Bin/Data/.../ExampleRenderGraph.json` must be redeployed (rebuild
  a runtime target or copy manually). After fixing #3 + redeploy,
  `Main.exe -c Audit.json` reaches steady state at frame 4 (56
  instances), validator silent, 268KB log, no Access Violation.
  The grep checklist for a buffer rename: `grep -rn "<OldName>"
  Source/ Data/` (NOT just Source/).
