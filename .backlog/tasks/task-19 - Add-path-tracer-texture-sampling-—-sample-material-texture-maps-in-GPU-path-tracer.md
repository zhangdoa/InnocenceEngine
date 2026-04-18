---
id: TASK-19
title: >-
  Add path tracer texture sampling — sample material texture maps in GPU path
  tracer
status: In Progress
assignee: []
created_date: '2026-04-13 08:05'
updated_date: '2026-04-18 11:39'
labels:
  - rendering
  - path-tracer
  - textures
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The GPU path tracer currently renders geometry and PBR scalars correctly but does not sample texture maps (albedo, normal, metallic, roughness, AO). Add texture sampling support so that textured materials render correctly in path tracer mode.

Work needed:
- Pass texture descriptors/SRV indices to the path tracer shader (bindless or per-material table)
- Sample albedo/metallic/roughness/normal in the closest-hit shader before evaluating BRDF
- Normal map tangent-space transform in the ray tracer
- Handle the BC-compressed textures added in TASK-10 (BC1/BC3/BC4/BC5 — all are samplable in DX12)
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Progress 2026-04-18 (5e77b493)

UV plumbing complete — path now has everything downstream of the cast:
- `GPUPathTracerVertex` carries texU/texV (stride 6→8 floats)
- `RebuildGeometryBuffers` copies `m_texCoord` from source vertices
- `ClosestHitShader` interpolates UV barycentrically
- `PathTracerPayload.texCoord` carries the interpolated UV into RayGen
- DXR PSO `MaxPayloadSizeInBytes` bumped 48→64 so the larger payload fits

All three regression tiers exit 0 (no behavioural change yet — the sample is still scalar-only).

## Remaining work

- Bind a bindless `Texture2D[]` array + `SamplerState` to the raytracing PSO. The rasterizer's `OpaquePass` exposes the same heap at `register(t3)` with `SamplerState g_Sampler : register(s0)` — the pattern to mirror is in `OpaquePass.cpp` (descriptor bindings 6 and 7) and the engine's `TextureResourceService` bindless SRV heap.
- In `ClosestHitShader`, sample `in_MaterialBuffer[instanceID].TextureIndices[1]` (albedo), [0] (normal → tangent-space transform), [2] (metallic), [3] (roughness) when the index is not `INVALID_TEXTURE_INDEX`, else fall back to the scalar material attributes. Pattern: `opaqueGeometryProcessPass.frag` lines 60-105.
- Normal-map unpacking requires a per-vertex tangent, which `GPUPathTracerVertex` doesn't carry yet; either add tangent to the vertex struct or fall back to object-space normals when no tangent is available.
<!-- SECTION:NOTES:END -->
