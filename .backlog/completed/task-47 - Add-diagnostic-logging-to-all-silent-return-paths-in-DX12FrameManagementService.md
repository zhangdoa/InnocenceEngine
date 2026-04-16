---
id: TASK-47
title: >-
  Add diagnostic logging to all silent return paths in
  DX12FrameManagementService
status: Done
assignee: []
created_date: '2026-04-16 19:05'
updated_date: '2026-04-16 19:25'
labels:
  - reliability
  - DX12
  - observability
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** DX12FrameManagementService.cpp has 12+ `return false` paths with no logging — BeginFrame command allocator reset failures, null commandList/renderPass, TLAS not ready, non-resident meshes, zero draw commands, swap chain Present nulls, WaitAllOnCPU null semaphore, PushRootConstants null params, BindGPUResource unhandled shader stage fallthrough, and UnbindGPUResource (complete no-op).

When these fail, entire render passes silently vanish with zero diagnostic output. This is the #1 source of "silent failures hiding bugs for weeks."

**Fix:** Add `Log(Warning, ...)` to every silent return path. Each log must identify the function, what failed, and why.

**Scope:** Only DX12FrameManagementService.cpp — the rendering client pass ObjectStatus guards (H4) are a separate task.

**Lines to fix (non-exhaustive):** 72, 252, 247, 333, 404, 461, 475, 528, 644, 979, 985, 991, 1025-1030, 1165
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added diagnostic logging to all 12+ silent return paths in DX12FrameManagementService.cpp. Every early-out now identifies the function, what failed, and why. Also surfaced that UnbindGPUResource is called during normal operation despite being a no-op — this is now visible as a warning.\n\nCommit: d1f2df93
<!-- SECTION:FINAL_SUMMARY:END -->
