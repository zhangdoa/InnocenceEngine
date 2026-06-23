---
id: TASK-250
title: >-
  GPUModelData m_ShaderProgramIndex / m_RenderPassIndex: not populated by CPU producer
status: Done
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
priority: low
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
uint32_t m_ShaderProgramIndex = 0;       // <-- never written by producer
float m_UUID = 0.0f;
uint32_t m_RenderPassIndex = 0;         // <-- never written by producer
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

### Why the priority is low (severity re-assess 2026-06-17c)

The HLSL side reads both fields, but the GPU has been consuming
0 in both pre-CL and post-CL states (in-class default `= 0` vs
explicit `= 0`). **No behavior change from the CLs.** The bug
is real (silent-wrong; the GPU is treating 0 as a valid index)
but is not a regression caused by the GPUUploadable rollout. The
user's report of black triangles after the CLs is unlikely to be
caused by this; the actual root cause is being investigated
under TASK-251 (re-scoped 2026-06-17c to a TBD investigation).

### Acceptance Criteria

- [ ] `DrawCallServiceImpl::UpdateDrawCalls` populates
      `l_gpuModelData.m_ShaderProgramIndex` from the actual
      `ShaderProgramComponent` associated with the mesh's
      shader asset (NOT a literal 0).
- [ ] `DrawCallServiceImpl::UpdateDrawCalls` populates
      `l_gpuModelData.m_RenderPassIndex` from the actual
      `RenderPassComponent` associated with the mesh's render
      pass (NOT a literal 0).
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
- Note: even with the fix, the HLSL reads the same offsets; the
  HLSL `StructuredBuffer<GPUModelData>` uses basic type
  alignment (4B aligned `float4`, no 16-byte row-alignment)
  per Microsoft DXC Buffer Packing wiki. The C++ and HLSL
  layouts match (verified 2026-06-17c); this task is purely
  about populating the index values, not about layout.

### Out of scope

- The C++/HLSL std140-mismatch claim for `m_BoundingBoxMin/Max`
  was FALSIFIED on 2026-06-17c. `GPUModelData` is used as
  `StructuredBuffer<>` on the HLSL side (basic type alignment),
  NOT as a cbuffer member (16-byte row-alignment). The C++ and
  HLSL layouts match. See TASK-251 (re-scoped to actual black
  triangle root-cause investigation).
- Removing the now-unnecessary `m_MaterialType` from
  `MaterialConstantBuffer` (also dead code; tracked separately
  if at all).

### Related

- TASK-243 (GBuffer-black regression: same class of bug — CPU
  field never written, GPU consumes 0, render goes black; that
  one was for `posWSNormalizer`).
- TASK-246 (GPUUploadable CRTP base; the validator is what
  caught this defect).
- TASK-251 (re-scoped 2026-06-17c; the black-triangle root
  cause investigation that surfaced this severity re-assess).

## Session log

- **2026-06-17 (filed):** original finding. TASK-249 CL 4
  (`75566da2`) added `PoisonInit()` to the producer; the
  pre-upload validator flagged the two unwritten fields. The
  migration preserved the prior on-GPU value (0) by writing the
  fields explicitly. The proper fix (populate from real
  `ShaderProgramComponent` and `RenderPassComponent`) was filed
  as this task at medium priority.
- **2026-06-17c (severity re-assess):** priority dropped
  medium → low. The HLSL uses `StructuredBuffer<GPUModelData>`
  (basic type alignment, NOT cbuffer). Both `m_ShaderProgramIndex`
  and `m_RenderPassIndex` are read by the HLSL (verified at
  `Source/Shaders/HLSL/common/common.hlsl:541, 543`), but the
  GPU has been consuming 0 in both pre-CL and post-CL states
  (in-class default `= 0` vs explicit `= 0`). **No behavior
  change from the CLs.** The bug is real (silent-wrong) but is
  not a regression. The user's report of black triangles after
  the CLs is unlikely to be caused by this; the actual root
  cause is being investigated under TASK-251.
- **2026-06-23 (closure — obsolete):** Both target fields no longer
  exist. TASK-253 (commit `c7cc9a72`) removed `m_ShaderProgramIndex`
  and `m_RenderPassIndex` from the struct as dead code (write-only,
  no HLSL reader — confirmed by grep: zero matches in `Source/` for
  either symbol; the current `RenderInstance` in
  `GPUDataStructure.h:282-301` holds only `m_MaterialIndex`,
  `m_meshID`, `m_BoundingBoxMin/Max`, `padding`). The proper fix this
  task proposed (populate the indices from real
  `ShaderProgramComponent` / `RenderPassComponent`) is moot — there is
  nothing to populate. Closed as obsolete; superseded by TASK-253 /
  TASK-252. AC #4's "every-field-written contract" unit-test idea is
  now satisfied by the `PoisonInit` validator over the live
  `RenderInstance` (per TASK-252 closure: validators silent).
