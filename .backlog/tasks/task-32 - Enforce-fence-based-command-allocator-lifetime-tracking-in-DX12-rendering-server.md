---
id: TASK-32
title: >-
  Enforce fence-based command allocator lifetime tracking in DX12 rendering
  server
status: To Do
assignee: []
created_date: '2026-04-13 18:13'
labels:
  - architecture
  - gpu-sync
  - d3d12
  - explicit-contracts
  - reliability
dependencies:
  - TASK-26
references:
  - Source/Engine/RenderingServer/DX12/DX12RenderingServer_CommandListAPI.cpp
  - Source/Engine/RenderingServer/Common/IRenderingServer.cpp
  - Source/Engine/Services/Common/FrameManagementServiceImpl.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The D3D12 crash (TASK-26) — "command allocator was reset after the command list was recorded" — is caused by the engine resetting a command allocator while the GPU is still executing a command list recorded from it. The engine relies on implicit frame-pacing timing rather than explicit GPU fence tracking to determine when it is safe to reset an allocator.

**Structural weakness:** GPU resource lifetime is managed by convention (frame N-2 is assumed complete by frame N) rather than by explicit fence-based verification. This assumption breaks under load spikes (the 800-900ms rendering task warning that precedes the crash), during scene transitions when InitializeComponents adds extra work, and in any scenario where frame timing is irregular.

**Target improvement:**
- Each command allocator should track the fence value of its last submission (the fence value that must be reached before Reset() is safe)
- DX12RenderingServer::Reset() (or its equivalent) should assert that the GPU has passed that fence value before resetting the allocator
- If the fence has not been reached, either wait (with a log warning) or defer the reset to the next frame
- This is the correct fix for the class of bugs where CPU-side resource reuse races with in-flight GPU work — it eliminates the entire category, not just the specific manifestation

See also: stash@{0} ("WIP: DX12 sync debugging — fence event fix + swap chain CL separation + lifecycle tracking") contains prior investigation work on this.
<!-- SECTION:DESCRIPTION:END -->
