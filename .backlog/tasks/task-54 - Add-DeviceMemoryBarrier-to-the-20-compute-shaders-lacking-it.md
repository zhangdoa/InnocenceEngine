---
id: TASK-54
title: Add DeviceMemoryBarrier() to the 20 compute shaders lacking it
status: To Do
assignee: []
created_date: '2026-04-16 20:56'
labels:
  - GPU
  - DX12
  - shaders
  - reliability
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Context:** Audit done as part of TASK-46. 20 of 22 compute shaders currently write UAVs without calling `DeviceMemoryBarrier()`. Only `opaqueGPUCulling.comp` and `sunShadowCulling.comp` have it (added in TASK-44).

**Engine cross-queue handoff pattern** (observed in every compute pass): each pass owns a dual command-list pair — a Graphics CL for resource-state transitions (often `CrossQueueTransition`) and a Compute CL for the dispatch. This makes every UAV-writing compute shader a potential cross-queue producer.

**Rule (already in `Documents/code-standards.md` §8 "Cross-queue UAV writes require DeviceMemoryBarrier"):** Any compute shader whose UAV output is consumed by another queue must issue `DeviceMemoryBarrier()` after all writes.

**Shaders to update** (under `Source/Shaders/HLSL/`):

- BRDFLUTMSPass.comp (2 UAVs)
- BRDFLUTPass.comp (1)
- RadianceCacheFilter.comp (1)
- RadianceCacheFilterHorizontal.comp (1)
- RadianceCacheFilterVertical.comp (1)
- RadianceCacheIntegration.comp (1)
- RadianceCacheReprojection.comp (1)
- SSAONoisePass.comp (1)
- TAAPass.comp (1)
- finalBlendPass.comp (2)
- lightCulling.comp (5)
- lightPass.comp (2)
- luminanceAveragePass.comp (2)
- luminanceHistogramPass.comp (1)
- mipmapGenerator2D.comp (3)
- mipmapGenerator3D.comp (3)
- postTAAPass.comp (1)
- preTAAPass.comp (1)
- skyPass.comp (1)
- tileFrustum.comp (1)

**Approach requirements:**

1. Place `DeviceMemoryBarrier()` *after* all UAV writes on every control-flow path that reaches the end of `main()`. Early `return` paths that skipped UAV writes don't need it; paths that wrote need it before the return.
2. `DeviceMemoryBarrier()` is a memory-ordering fence, not a cross-thread sync — no uniform-control-flow requirement like `GroupMemoryBarrierWithGroupSync`. It is safe to insert in non-uniform branches, but only where a UAV write actually occurred on that path.
3. Visual validation required per `feedback_onscreen_testing.md` — run an on-screen test (interactive `InteractiveTest.ps1`) after each batch, not only `-offscreen`. Rendering regressions (ghosting, incorrect illuminance, banding) are the failure modes.
4. Build after each shader with `HLSL2DXIL.ps1` and confirm no DXC errors. RenderTest regression (exit 0) is the minimum quality gate.

**Dependencies:** TASK-46 (parent audit task).
<!-- SECTION:DESCRIPTION:END -->
