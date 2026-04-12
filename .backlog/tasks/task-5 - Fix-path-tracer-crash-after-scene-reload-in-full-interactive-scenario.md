---
id: TASK-5
title: Fix path tracer crash after scene reload in full interactive scenario
status: Done
assignee: []
created_date: '2026-04-06 22:56'
updated_date: '2026-04-12 02:03'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fixed in commit b326e6fd. Two root causes in MeshResourceServiceImpl.cpp:
1. Double-init heap corruption: added Residency guard — skips InitializeImpl if asset is already Resident.
2. Dangling pointer after GPU wait: save asset handle before InitializeImpl, re-fetch both resource and component pointers after it returns (std::vector reallocation during GPU stall made old pointers invalid).
Also added null-check on template pointer in JSONSerializer_Components.cpp for non-Customized mesh shapes.
All four test tiers pass: RenderTest, 10-frame integration, scene reload, interactive full scenario.
<!-- SECTION:FINAL_SUMMARY:END -->
