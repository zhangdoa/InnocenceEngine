---
id: TASK-174
title: 'Extract ExampleRenderingClient PrepareCommands/ExecuteCommands to sibling .inl files'
status: Done
assignee: []
created_date: '2026-04-28 07:30'
updated_date: '2026-05-15'
labels:
  - rendering
  - refactor
  - file-size
  - superseded
dependencies: []
priority: low
references:
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
  - .claude/disciplines/split-before-grow.md
---

## Closure (2026-05-15) — superseded

Superseded by TASK-219 commit `af981c25` (2026-05-05), which split `ExampleRenderingClient.cpp` 1518 → 318 lines via partial-class `.cpp` peer files (`_PrepareCommands.cpp`, `_ExecuteCommands.cpp`, `_ExecuteCommands_GI.cpp`, `_ExecuteCommands_Rasterizer.cpp`, `_Bootstrap.cpp`, `_Setup.cpp`, `_Capture.cpp`, `_AuditDump.cpp`) plus `_Internal.h`. Largest resulting TU is 305 lines (under the 400-line ratchet).

Shape divergence from this ticket's brief: TASK-219 chose `.cpp` (partial-class) over `.inl` (function-body include). Both are listed in `file-splitting` SKILL.md as valid C++ partial-split patterns; `.cpp` is the canonical pattern in that skill table. The only remaining `.inl` (`ExampleRenderingClient_Bypass.inl`) holds anonymous-namespace helper templates that legitimately need TU-scope replication.

AC disposition:
- AC #1 PrepareCommands / ExecuteCommands extracted to sibling files — Done as `.cpp` not `.inl`
- AC #2 cpp shrunk below 1000 lines — Done (318 lines)
- AC #3 Build clean — inherited from `af981c25` + subsequent CLs
- AC #4 Smoke clean — inherited
- AC #5 New files under 400 lines — Done (largest 305)

No code action required by this closure; the work landed two weeks earlier under TASK-219.

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Filed by TASK-171 closure, 2026-04-28.**

`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` is 1212 lines (TASK-171 commit `d8f9dcb9`, +49 over the pre-TASK-171 baseline of 1163). The file-size soft ratchet flags any growth on this grandfathered oversized file. TASK-171 used `[skip-size-gate]` legitimately because the per-site bypass instrumentation is intrinsic to the feature, but the underlying problem (the file is too big) wasn't fixed.

### Required refactor

Extract `PrepareCommands` and `ExecuteCommands` (the two large per-frame pass-orchestration functions) into sibling `.inl` files:

- `ExampleRenderingClient_PrepareCommands.inl`
- `ExampleRenderingClient_ExecuteCommands.inl`

Each `.inl` contains the function body; the cpp `#include`s them. Mirror the precedent of `LightDataService_PointShadow.inl` (TASK-147) and `ExampleRenderingClient_Bypass.inl` (TASK-171 iteration 3).

After the extraction, the cpp should be substantially below 1163 lines (probably ~600-700, since PrepareCommands and ExecuteCommands together are roughly ~600 lines). The two new .inl files will be 200-400 lines each — well under the 400-line ratchet for new files.

### Why low priority

Pure mechanical refactor. No behavioural change. No user-visible impact. Improves future per-pass instrumentation (TASK-168, etc.) by making each pass-orchestration function smaller and easier to extend without hitting the size gate.

### Owner

`rendering-researcher` (file owner). Mechanical-refactor exemption in `peer-review-required.md` applies — `Review-Skipped: mechanical-rename` is the appropriate artifact.

### Why not bundled with TASK-171

Two reasons:
1. Scope discipline — TASK-171 was scoped to bypass functionality. Bundling the file-extraction would have made the CL harder to review.
2. The pre-TASK-171 file size already triggered the gate's grandfather rule. Fixing the underlying size is a separate concern from instrumenting the file with bypass.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 PrepareCommands and ExecuteCommands extracted to sibling .inl files
- [ ] #2 cpp file shrunk to under 1000 lines (target: ~600-700)
- [ ] #3 Build clean (RelWithDebInfo, DX12)
- [ ] #4 Smoke clean (Main.exe -total_frames 30 → exit 0, no behavioural change)
- [ ] #5 New .inl files each under 400 lines (don't trigger size gate themselves)
<!-- AC:END -->
