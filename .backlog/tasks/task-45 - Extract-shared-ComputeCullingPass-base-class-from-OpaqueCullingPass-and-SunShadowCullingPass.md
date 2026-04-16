---
id: TASK-45
title: >-
  Extract shared ComputeCullingPass base class from OpaqueCullingPass and
  SunShadowCullingPass
status: To Do
assignee: []
created_date: '2026-04-16 18:30'
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
