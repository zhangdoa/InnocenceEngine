---
id: TASK-18
title: Adopt mesh shaders for GPU-driven geometry (meshlet pipeline)
status: To Do
assignee: []
created_date: '2026-04-13 08:05'
updated_date: '2026-04-19 02:10'
labels:
  - rendering
  - performance
  - geometry
  - mesh-shaders
  - dx12
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the traditional vertex/index buffer + IA stage geometry path with the DX12 mesh-shader pipeline (amplification + mesh shaders, meshlet-based). LOD is becoming outdated — modern GPU-driven pipelines do per-meshlet visibility/cull/LOD selection on-GPU instead of CPU-side LOD swapping.

## Why mesh shaders over LOD

- One geometry path that handles culling, LOD, and visibility entirely on the GPU per draw — no CPU-side per-entity LOD selection.
- Per-meshlet frustum + backface + occlusion culling reduces wasted vertex work compared to whole-mesh draws.
- Aligns with where the engine is heading (path tracer is already GPU-driven, OpaquePass uses ExecuteIndirect — mesh shaders complete the picture).
- Fewer shader-permutation paths long-term: the same mesh shader can amplify to fewer meshlets at distance, making continuous LOD a natural side effect rather than a separate descriptor.

## Scope

- Add a meshlet baking step in the asset import pipeline (DirectXMesh / meshoptimizer) that emits meshlets + per-meshlet bounds alongside the existing vertex/index streams.
- New shader stages: amplification shader (cull) + mesh shader (emit primitives) for opaque geometry. Reuse the existing pixel shader.
- New RenderPass mode in `RenderPassResourceService` that wires AS+MS instead of VS+IA.
- Migrate `OpaquePass` first (most-touched path); keep VS+IA path as a build-time toggle so a hardware fallback exists for cards without mesh-shader support (compile-time gate, not runtime).
- Update the mega-buffer layout in path tracer's geometry rebuild to share meshlet metadata where useful (path tracer's TLAS still wants triangles, so meshlet bounds are reusable for AS/MS in raster but not directly in DXR — separate concern).

## Non-goals

- Continuous-detail / dynamic tessellation — mesh shaders enable it but adopting variable amplification is a follow-up.
- Backporting mesh shaders to Vulkan — DX12 only for the first pass; Vulkan VK_EXT_mesh_shader port is a separate task once the DX12 path is stable.
- Replacing the path tracer's BLAS/TLAS — DXR still consumes triangles via vertex/index buffers; meshlet metadata in the raster path is in addition to, not instead of, those buffers.

## References

- DirectX-Specs: D3D12 mesh shaders.
- DirectXMesh `ComputeMeshlets`.
- meshoptimizer `meshopt_buildMeshlets` (alternative source of meshlet packing).
- Existing OpaquePass (`Source/ExampleProject/RenderingClient/OpaquePass.cpp`) is the migration target.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Asset import emits per-mesh meshlet metadata (meshlet vertex/triangle remap + per-meshlet AABB) into the same `.innobin` family
- [ ] #2 New `OpaqueMeshShaderPass` (or a build-time toggle in `OpaquePass`) drives geometry via AS+MS instead of VS+IA on a Sponza load
- [ ] #3 Per-meshlet frustum cull in the amplification shader, validated visually (orbit camera, off-screen meshlets get skipped in a debug capture)
- [ ] #4 GISponza auto-test (Tier 2) passes with the mesh-shader path enabled
- [ ] #5 No correctness regression vs. the existing VS+IA path on the GISponza reference capture
<!-- AC:END -->
