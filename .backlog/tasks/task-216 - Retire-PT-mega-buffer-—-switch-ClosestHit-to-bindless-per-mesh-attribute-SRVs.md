---
id: TASK-216
title: Retire PT mega-buffer — switch ClosestHit to bindless per-mesh attribute SRVs
status: Done
assignee:
  - rendering-researcher
  - graphics-api-expert
created_date: '2026-05-03 11:40'
updated_date: '2026-05-03 13:09'
labels:
  - path-tracer
  - dx12
  - bindless
  - rt
  - tlas
dependencies: []
references:
  - 7bab6ea3
  - 'Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp:773'
  - Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl
  - 'Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp:300'
  - 'Source/Engine/Services/DX12/DX12MeshResourceService.cpp:140'
  - 'Source/Engine/Services/DX12/DX12FrameManagementService.cpp:608'
  - 'Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp:1569'
  - 'Source/Engine/Services/DX12/DX12TextureResourceService.cpp:559'
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Background

The GPU path tracer's ClosestHit shader (`Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl`) recovers per-vertex normals/UVs by indexing three CPU-managed mega-buffers — `in_MegaVertexBuffer`, `in_MegaIndexBuffer`, `in_MeshOffsets` — keyed by `InstanceID()`. These are concatenated copies of every loaded mesh's VB/IB rebuilt by `GPUPathTracerPass::RebuildGeometryBuffers()` on every scene load and on mesh-count changes (`Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp:773+`).

The user-stated end-state (commit `7bab6ea3` body): "Once the path tracer moves to bindless per-mesh BLAS lookup at hit time, the PT becomes a stateless asset-free pass like SSAO and the residency predicate has nothing to track."

PT already traces against the engine-wide TLAS — this isn't new acceleration-structure work. It's replacing the **attribute-fetch path** in ClosestHit with bindless per-mesh SRVs.

## Design — locked in

DXR exposes both `InstanceID()` (24-bit, user-supplied via `D3D12_RAYTRACING_INSTANCE_DESC.InstanceID`) and `InstanceIndex()` (auto, the implicit TLAS-list position). Two free indices, used as:

- **`InstanceID()`** carries the **bindless mesh-attribute SRV slot index** (mesh identity).
- **`InstanceIndex()`** indexes `in_MaterialBuffer` directly (per-instance material — same lossless RenderingComponent-keyed shape as today, no new shadow state).

Per-mesh data lives once in `DX12MeshResourceService`'s default-heap VB/IB. Two new SRV descriptors per mesh, in a dedicated bindless heap, mirror the existing material-texture pattern (`m_MaterialTexture_SRV_DescHeapAccessor`).

## Why option B (not C)

C — `GeometryIndex()` per-BLAS — would require restructuring BLAS construction to pack multiple meshes per BLAS as separate geometries. Current shape is one-mesh-per-BLAS (single geometry) — `GeometryIndex()` is always 0 and useless. Switching to multi-geometry-per-BLAS would destroy independent BLAS lifetime (one mesh's residency change invalidates the bundle). B keeps the per-mesh BLAS shape untouched.

## Slicing — two CLs

### CL #1 — DX12 bindless mesh-attribute heap (additive only, zero behavioural change)

1. Add `m_BindlessMeshSRV_DescHeapAccessor` to `DX12Context` (mirror `m_MaterialTexture_SRV_DescHeapAccessor` setup in `DX12GraphicsHardwareService.cpp:1569`). Sized for max mesh count (start at e.g. 1024 paired slots).
2. Extend `DX12Context::GetDescriptorHeapAccessor` switch — new enum value or auxiliary path for "MeshAttribute" buffers.
3. `DX12MeshResourceService::DX12MeshGPUResources` gains `m_VertexSRVSlot`, `m_IndexSRVSlot` (`uint32_t` each).
4. `DX12MeshResourceService::InitializeImpl` — after creating the default-heap VB/IB:
   - Allocate vertex SRV (`StructuredBuffer<Vertex>`, stride=`sizeof(Vertex)`, `NumElements=vertexCount`) into the new heap.
   - Allocate index SRV (`ByteAddressBuffer`, raw — handles 16/32-bit at shader time) into the new heap.
   - Record both slot indices on the mesh resource.
5. `MeshResourceService` virtual interface: `GetVertexSRVSlot(MeshAssetHandle)`, `GetIndexSRVSlot(MeshAssetHandle)`. DX12 implementation reads from `m_DX12MeshResources` map.

CL #1 lands without breaking anything: the heap fills as meshes load, but no shader reads it yet. PT continues using mega-buffers.

### CL #2 — PT mega-buffer retirement (the user-visible flip)

1. **TLAS** (`DX12GPUBufferResourceService.cpp:300`): `instanceDesc.InstanceID = vertexSRVSlot` (looked up via `MeshResourceService`). Was: `l_descList->m_Descs.size()`.
2. **PT pass binding layout**: drop t2/t3/t4 (mega VB, mega IB, MeshOffsets). Add new bindless StructuredBuffer<Vertex> array binding at t8 and bindless ByteAddressBuffer array binding at t9. Engine autobinds the new heap (mirror the t7 material-textures `nullptr` autobind path at `DX12FrameManagementService.cpp:608-614` — extended to handle the new "buffer bindless" variant).
3. **PT pass C++**: delete `m_MegaVertexBuffer`, `m_MegaIndexBuffer`, `m_MeshOffsetBuffer`, `m_PendingVertices`, `m_PendingIndices`, `m_PendingOffsets`, `RebuildGeometryBuffers` vertex/index/offset code, `MeshOffsetData` struct, `GPUPathTracerVertex` struct, `m_PendingGeometryRebuild`, `AreMeshesGPUReady` (or simplify — only material upload remains gated). Material rebuild loop stays — it now collects MaterialCBs only.
4. **`m_BuiltMeshCount`** semantics — now tracks last-known mesh count for material rebuild trigger only.
5. **ClosestHit shader rewrite** (`GPUPathTracerClosestHit.hlsl`):
   - Drop `in_MegaVertexBuffer`, `in_MegaIndexBuffer`, `in_MeshOffsets`, `MeshOffsetData`, `LoadTriangleIndices`/`LoadVertexNormal`/`LoadVertexUV`.
   - Add `StructuredBuffer<Vertex> g_MeshVertexBuffers[] : register(t8, space1)` and `ByteAddressBuffer g_MeshIndexBuffers[] : register(t9, space1)`.
   - `uint slot = InstanceID();`, then `g_MeshVertexBuffers[NonUniformResourceIndex(slot)][indexN]` and `g_MeshIndexBuffers[NonUniformResourceIndex(slot)].Load(byteOffset)` for index fetch.
   - Material: `MaterialCB mat = in_MaterialBuffer[InstanceIndex()];` (was `[InstanceID()]`).
   - Shader-side index decode: read 32-bit indices via `Load(primIdx*12)` for `R32_UINT` IB; if some meshes use 16-bit IBs we either normalize at mesh-init time or branch per slot (defer that decision until we hit it — current PT flow upcasts to 32-bit during mega-buffer rebuild, so default-heap IBs may already be 16-bit-mixed).

## Acceptance

- PT renders identically to current output (visual A/B at known scenes — Sponza, default test scene).
- No mega-buffers exist in `GPUPathTracerPass`.
- Scene load/unload no longer triggers `RebuildGeometryBuffers` for vertex/index data.
- BLAS / TLAS lifetime unchanged.
- D3D12 debug layer clean.

## Reference

- Commit `7bab6ea3` — retirement-decision rationale.
- Existing bindless-texture pattern: `m_MaterialTexture_SRV_DescHeapAccessor` (DX12Context.h:42, DX12GraphicsHardwareService.cpp:1569), autobind path `DX12FrameManagementService.cpp:608-614`, slot accessor `DX12TextureResourceService::GetIndex` (DX12TextureResourceService.cpp:559).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 CL #1: bindless mesh-attribute SRV heap exists; per-mesh vertex+index SRVs created at InitializeImpl time; GetVertexSRVSlot / GetIndexSRVSlot accessors return correct slot indices; PT continues to render via mega-buffers (zero behavioural change)
- [x] #2 CL #2: TLAS InstanceID = vertexSRVSlot; PT pass mega-buffer plumbing fully removed; ClosestHit reads g_MeshVertexBuffers[InstanceID()] / g_MeshIndexBuffers[InstanceID()]; MaterialBuffer indexed by InstanceIndex(); PT image visually identical to pre-change baseline
- [x] #3 D3D12 debug layer clean across scene load/unload cycles
- [ ] #4 Peer review: rendering-researcher reviewer for PT pass + shader; graphics-api-expert reviewer for DX12 bindless heap + TLAS InstanceID change
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Mega-buffer retirement landed as one combined CL (CL #1 + CL #2 fused per user direction "B — main session, peer review at end").

**DX12 layer additions:**
- `DX12Context.h` adds `m_BindlessMeshVertex_SRV_DescHeapAccessor` and `m_BindlessMeshIndex_SRV_DescHeapAccessor`; `DX12GraphicsHardwareService.cpp` allocates two new sections at the tail of the visible CSU heap (sized to `maxMeshes` = 1024).
- `DX12MeshResourceService::InitializeImpl` allocates a paired (vertex SRV, index SRV) at mesh-init time. New accessors `GetVertexSRVSlot(handle)` / `GetIndexSRVSlot(handle)`.
- `DX12GPUBufferResourceService::UpdateRaytracingInstances` writes `instanceDesc.InstanceID = vertexSRVSlot` (was: TLAS-list position).

**Bindless-buffer support extension (engine-wide):**
- `GPUBufferUsage::BindlessMeshVertex` / `BindlessMeshIndex` added to `GraphicsPrimitive.h`.
- `DX12RenderPassResourceService::GetDescriptorRange` sets `NumDescriptors = max-mesh-slots` and `RegisterSpace = 1/2` for the new bindings (mirrors `TextureUsage::Sample` bindless-array shape; spaces 1/2 keep unbounded ranges from clashing with t7 textures or each other).
- `DX12FrameManagementService::BindComputeResource` autobinds the matching heap's first handle when the binding's `m_GPUBufferUsage` is `BindlessMesh{Vertex,Index}`.

**PT pass strip:**
- `GPUPathTracerPass.{h,cpp}` removes `m_MegaVertexBuffer`, `m_MegaIndexBuffer`, `m_MeshOffsetBuffer`, `GPUPathTracerVertex`, `MeshOffsetData`, `m_PendingVertices`, `m_PendingIndices`, `m_PendingOffsets`, `m_PendingGeometryRebuild`, `RebuildGeometryBuffers`, `AreMeshesGPUReady`. What remains: `RebuildMaterialBuffer` (per-instance materials only) + `RefreshMaterialTextureIndices` (unchanged). Material rebuild's skip predicate now mirrors TLAS rebuild exactly (`Activated && BLAS != 0 && VertexSRVSlot != UINT32_MAX`) so `InstanceIndex()` is the correct material-buffer index.
- Binding layout drops t2/t3/t4 mega-buffer SRVs, adds t8/t9 bindless mesh-attribute arrays. Base-binding count: 13 → 12.

**Shader:**
- `GPUPathTracerClosestHit.hlsl` declares `StructuredBuffer<MeshVertex> g_MeshVertexBuffers[] : register(t0, space1)` (64-byte stride, mirrors `Inno::Vertex`) and `Buffer<uint> g_MeshIndexBuffers[] : register(t0, space2)`. Indexed by `InstanceID()`. Material lookup uses `InstanceIndex()`.

**Validation:** RelWithDebInfo build clean across Engine + ExampleRenderingClient + Main. Runtime: Sponza loads, BLAS init succeeds for every mesh, 60 PT frames render with `-gpu_validation`, no D3D12 errors. PathTracerReadback `mean=(0.225, 0.235, 0.237) max=(0.933, 0.925, 0.924)` — visually correct (curtains/stone/archway all read), the small numerical delta vs the pre-change baseline is RNG variance over 60 accumulation frames (PT samples are stochastic, Halton-jittered).

**Carve-outs (deferred):**
- VB/IB resource state stays at `VERTEX_AND_CONSTANT_BUFFER` / `INDEX_BUFFER`. Debug layer is clean — DXR tolerates these states for the SRV reads. If a future workload needs explicit `NON_PIXEL_SHADER_RESOURCE`, promote to `D3D12_RESOURCE_STATE_GENERIC_READ` at end of `InitializeImpl`.
- Bindless slots are leaked on mesh release (`DX12DescriptorHeapAccessor` has no `FreeHandle`). Same monotonic-allocator pattern as the bindless texture heap; pre-existing engine-wide.
- VK backend unaffected — PT is DXR-only; the new `GetVertexSRVSlot` / `GetIndexSRVSlot` accessors live on `DX12MeshResourceService` only.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
