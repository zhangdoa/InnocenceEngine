---
id: TASK-26
title: Fix D3D12 command allocator reset crash during GISponza first render frame
status: Done
assignee: []
created_date: '2026-04-13 17:59'
updated_date: '2026-04-18 09:08'
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

## Resolution (2026-04-18)

The two parts of the symptom had separate causes.

1. **"Command allocator was reset after command list was recorded"** —
   this specific D3D12 error no longer reproduces. It was downstream of
   the NamedObjectPool collision (5a4050c2): aliased render targets and
   compute buffers meant different passes' recorded command lists
   referenced the same resource in incompatible states, which the DX12
   runtime reported as a post-hoc allocator-reset error. Fixing the
   aliasing made the symptom vanish.

2. **STATUS_STACK_BUFFER_OVERRUN (-1073740791) on exit** — fixed in
   11f3af15 by switching `LogService`'s fatal-in-test handler from
   `std::exit(1)` to `std::_Exit(1)`. The old `exit` ran CRT static
   destructors while the D3D12 debug-callback thread was still inside
   `D3D12Core.dll`, producing a /GS stack cookie failure that masqueraded
   as the real exit code. `_Exit` skips CRT teardown entirely, so the
   process exits with code 1 as intended.

The deeper structural concern — that GPU resource lifetime is managed
by convention rather than explicit fence-based tracking — remains open
and is tracked by TASK-32.
