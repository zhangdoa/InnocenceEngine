---
id: TASK-45
title: >-
  Extract shared ComputeCullingPass base class from OpaqueCullingPass and
  SunShadowCullingPass
status: Done
assignee: []
created_date: '2026-04-16 18:30'
updated_date: '2026-04-18 11:49'
labels:
  - refactor
  - rendering
  - DX12
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/OpaqueCullingPass.cpp
  - Source/ExampleProject/RenderingClient/SunShadowCullingPass.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?**
Both culling passes (OpaqueCullingPass, SunShadowCullingPass) follow an identical pattern: Setup allocates shader+renderpass+indirect buffer+command list with the same resource binding layout, Initialize calls the same 4 service initializers, PrepareCommandList does the same bind-dispatch-track sequence. When the SetCurrentState fix was needed (TASK-44), it had to be applied independently to both files. Any future culling pass (transparency, decals) would copy-paste the same 160 lines again.

**Structural weakness:** No shared abstraction for "compute culling pass that writes an indirect draw buffer." Each pass re-implements the entire lifecycle.

**Improvement:** Extract a `ComputeCullingPass` base class (or template) that owns the common Setup/Initialize/Terminate/PrepareCommandList skeleton. Subclasses only specify the shader path, resource name prefix, and optionally override the resource binding layout if needed. The DeviceMemoryBarrier + SetCurrentState pattern becomes a single implementation point.
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Extracted `ComputeCullingPass` base class in d3dc6286. OpaqueCullingPass and SunShadowCullingPass each collapse to a 14-line header-only class declaring two virtual hooks (`GetPassName()`, `GetComputeShaderPath()`). The shared Setup / Initialize / Terminate / PrepareCommandList skeleton — including the resource binding layout, dispatch math, and UAV state-tracking update — now lives in one place.

Net -262 / +192 lines. Any future culling sibling (transparency, decals, particles) becomes a 14-line header. TASK-44's `SetCurrentState` and any future compute-culling fix applies once, not per subclass.

Regression: RenderTest, Main 10-frame integration, and scene reload all exit 0.
<!-- SECTION:FINAL_SUMMARY:END -->
