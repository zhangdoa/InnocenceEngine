---
id: TASK-26
title: Fix D3D12 command allocator reset crash during GISponza first render frame
status: To Do
assignee: []
created_date: '2026-04-13 17:59'
labels:
  - bug
  - d3d12
  - gpu-sync
  - crash
  - scene-reload
dependencies: []
references:
  - Source/Engine/Services/Common/FrameManagementServiceImpl.cpp
  - Source/Engine/RenderingServer/DX12/DX12RenderingServer_CommandListAPI.cpp
  - Source/Engine/RenderingServer/Common/IRenderingServer.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
After GISponza finishes loading (frame 5), the engine crashes with exit code -1073740791 (STATUS_STACK_BUFFER_OVERRUN) during the first render frame. The D3D12 debug layer fires:

```
D3D12 ERROR: ID3D12CommandQueue::ExecuteCommandList: The command allocator was reset after the command list was recorded.
```

followed by the fatal error handler calling std::exit(1), but the process exits with -1073740791 instead of 1 — indicating a /GS stack cookie failure during the exit path.

**Observed pattern (from log):**
- MaterialResourceService::InitializeComponents runs (deferred materials for NewSponza, ShaderBall, dragon, bunny)
- "Rendering Execution Task" takes ~800-900ms
- 300ms later: D3D12 error fires from ExecuteCommandList
- Fatal error handler: "exiting with code 1"
- Process exit: -1073740791

This happens in BOTH the basic 10-frame integration test (-total_frames 10) AND the 20-frame scene reload test (-total_frames 20 -reload_at_frame 10), on the very first GISponza render frame. Confirmed pre-existing (present before the texture overrun fix in aef0f866).

**Root cause hypothesis:** MaterialResourceService::InitializeComponents and the rendering execution task run concurrently. InitializeComponents may Reset() a command allocator while the GPU is still executing a command list recorded from that allocator — a GPU sync race condition.

**Investigation starting points:**
- FrameManagementService::Update() calls InitializeComponents then ExecuteGlobalCommands — check whether InitializeComponents on the scene-load frame races with a rendering thread that submitted commands before the scene was flagged as loaded
- DX12RenderingServer command allocator reset/reuse logic
- The stash `WIP: DX12 sync debugging - fence event fix + swap chain CL separation + lifecycle tracking` (stash@{0}) contains prior work on this; review before starting fresh
<!-- SECTION:DESCRIPTION:END -->
