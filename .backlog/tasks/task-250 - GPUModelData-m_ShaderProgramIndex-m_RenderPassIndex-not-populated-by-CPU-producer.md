---
id: TASK-250
title: >-
  GPUModelData m_ShaderProgramIndex / m_RenderPassIndex: not populated by CPU producer
status: To Do
assignee:
  - code-impl
created_date: '2026-06-17'
labels:
  - rendering
  - bug
  - gpu-data
  - follow-up
dependencies:
  - TASK-249
priority: medium
---

## Description

Surfaced by TASK-249 CL 4 (commit `75566da2`): when `GPUModelData`
was migrated to `GPUUploadable<>` with `PoisonInit()`, the
pre-upload validator immediately surfaced a TASK-243-class bug
on two fields the CPU producer never wrote.

### The defect

`Source/Engine/Common/GPUDataStructure.h:276-278`:

```cpp
uint32_t m_MaterialIndex = 0;
uint32_t m_ShaderProgramIndex = 0;       // <-- never written
float m_UUID = 0.0f;
uint32_t m_RenderPassIndex = 0;         // <-- never written
```

`Source/Engine/Services/DrawCallService.cpp:154-213`
(`DrawCallServiceImpl::UpdateDrawCalls`): the producer writes
`m_MaterialIndex`, `m_UUID`, `m_VisibilityMask`, `m_MeshUsage`,
`m_BoundingBoxMin`, `m_BoundingBoxMax`, `m_InstanceCount`,
`m_FirstInstance`, etc., but never `m_ShaderProgramIndex` or
`m_RenderPassIndex`. The pre-`PoisonInit` `= {}` zero-init masked
this; the in-class default `= 0` further masked it.

`Source/Shaders/HLSL/common/common.hlsl:540-543` reads both
fields. So the GPU has been getting 0 (from the in-class default)
for both, and the HLSL has been consuming 0 as a real index. The
result is silent-wrong behavior, not a crash.

### Why the migration preserved the bug

In commit `75566da2` the producer was updated to write
`m_ShaderProgramIndex = 0;` and `m_RenderPassIndex = 0;`
explicitly. This preserves the prior on-GPU value (0) so the
migration does not regress the render. **The proper fix is
to populate these from the real `ShaderProgramComponent` and
`RenderPassComponent` indices** the mesh owner holds. Neither
of those indices is on the `MeshComponent` itself in the current
data model — they are looked up via separate `ShaderProgram`
and `RenderPass` services by asset name.

### Acceptance Criteria

- [ ] `DrawCallServiceImpl::UpdateDrawCalls` populates
      `l_gpuModelData.m_ShaderProgramIndex` from the actual
      `ShaderProgramComponent` associated with the mesh's
      shader asset (NOT a literal 0).
- [ ] `DrawCallServiceImpl::UpdateDrawCalls` populates
      `l_gpuModelData.m_RenderPassIndex` from the actual
      `RenderPassComponent` associated with the mesh's render
      pass (NOT a literal 0).
- [ ] Both fields removed from the GPUUploadable's
      `SkipByteRanges` (the offsets they live at are not
      padding; the producer now writes them).
- [ ] Live-engine smoke (`Main.exe -c Audit.json`) is clean for
      the migration; no new Error lines.
- [ ] Test: write a small unit test that constructs a
      `GPUModelData`, calls `PoisonInit()`, and asserts
      `FirstUnwritten()` is `SIZE_MAX` after the producer
      writes every field. (Locks the contract.)

### Investigation hints

- `Source/Engine/AssetService_ShaderProgramRegistry.{cpp,h}`
  and `Source/Engine/AssetService_RenderPassRegistry.{cpp,h}`
  are the natural sources of truth; see how `MaterialComponent`
  is looked up by name in the current producer for the
  `l_materialAsset` pattern.
- Alternatively: extend `MeshComponent` to carry a
  `ShaderProgramHandle` + `RenderPassHandle` directly (cleaner
  but a wider schema change).

### Out of scope

- TASK-249.B: the C++-HLSL std140 mismatch on
  `m_BoundingBoxMin/Max` (separate task).
- Removing the now-unnecessary `m_MaterialType` from
  `MaterialConstantBuffer` (also dead code; tracked separately).

### Related

- TASK-243 (GBuffer-black regression: same class of bug — CPU
  field never written, GPU consumes 0, render goes black; that
  one was for `posWSNormalizer`).
- TASK-246 (GPUUploadable CRTP base; the validator is what
  caught this defect).

## Session log

- **2026-06-17**: filed by TASK-249 CL 4 (`75566da2`). Migration
  landed with explicit `= 0` writes for both fields to preserve
  on-GPU behavior; the real fix is pending.
