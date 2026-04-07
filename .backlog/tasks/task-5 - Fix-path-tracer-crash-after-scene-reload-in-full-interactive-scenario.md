---
id: TASK-5
title: Fix path tracer crash after scene reload in full interactive scenario
status: To Do
assignee: []
created_date: '2026-04-06 22:56'
labels:
  - bug
  - path-tracer
  - scene-reload
  - gpu-sync
dependencies: []
references:
  - Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp
  - Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
When running the full interactive test scenario (camera movement → path tracer toggle on/off/on/off → scene reload → path tracer toggle on), the engine crashes with D3D12 error: `GPUPathTracerMegaVB` resource is released while GPU operations are still in-flight on the ComputeCommandQueue. This is a pre-existing race condition in the scene reload path — the geometry buffers are destroyed while the compute queue still references them. Reproduced on both pre- and post-PostTAA-removal code.
<!-- SECTION:DESCRIPTION:END -->
