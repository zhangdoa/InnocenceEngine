---
id: TASK-146
title: >-
  Build: Bin/Shaders/DXIL/ deploy must be mirror (delete-orphans), not additive
  — phantom-regression root cause
status: To Do
assignee: []
created_date: '2026-04-26 21:28'
labels:
  - build
  - infrastructure
  - bug
  - shaders
dependencies: []
references:
  - Build/HLSL2DXIL_NoPause.ps1
  - Scripts/HLSL2DXIL.ps1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Critical infrastructure bug discovered 2026-04-26 via bisect.**

The shader compile + deploy chain accumulates stale DXIL artifacts in `Bin/Shaders/DXIL/`. When source-tree HLSL files are deleted or renamed (e.g. revert of WIP work, branch switch, bisect), their previously-compiled `.dxil` files are NOT removed from the build cache. The deploy step then copies the entire (contaminated) cache to `Bin/RelWithDebInfo/Shaders/DXIL/`.

Worse: when the SAME shader name has different content across commits (e.g. `lightPass.comp.dxil` was the monolithic shader pre-TASK-135 and the post-split refactor post-TASK-135), incremental builds at an older commit may NOT recompile the shader (timestamp comparison says cached DXIL is newer), leaving the engine loading DXIL with bindings that don't match the engine binary's expectations → GPU device-hang or silent rendering corruption.

### How this caused a phantom regression chain

A multi-hour debug session (TASK-141 → TASK-142 → TASK-144 → TASK-145) chased a "sun shadow regression" on GISponza/UnitTest/GITestBox that turned out to be entirely build-cache contamination, NOT a source-code bug:

- TASK-138 WIP (later reverted) created new shader files (`SunShadowRT*.hlsl`); their `.dxil` linged in `Bin/Shaders/DXIL/` after revert
- Subsequent incremental builds at HEAD AND at older bisect commits (60c645a0, 723c94b4, d58b4fbf) inherited stale DXIL
- The d58b4fbf engine, attempting to load `lightPass.comp.dxil`, got the post-TASK-135 split version → binding mismatch → device-removed (`HRESULT=-2005270522`) → black screen
- Every "no shadow" / "dark image" / "fragmented shadow" user report during the session was contamination, not real

Once disciplined (nuke `Bin/Shaders/DXIL/` + `Bin/RelWithDebInfo/Shaders/DXIL/` + repopulate via `HLSL2DXIL_NoPause.ps1` + cmake build), every commit in the bisect chain showed shadows working correctly.

### Required fix

The shader compile/deploy chain must be **mirror semantics**: the deploy target (`Bin/RelWithDebInfo/Shaders/DXIL/`) should EXACTLY match the set of shaders compiled from current source, with orphans removed. Same for the upstream cache (`Bin/Shaders/DXIL/`).

Two implementation options:

1. **HLSL2DXIL_NoPause.ps1 nuke-on-start**: at the top of the script, delete `Bin/Shaders/DXIL/` before recompiling. Forces full rebuild every invocation; slow but bulletproof.
2. **Tracked-file delete pass**: script enumerates source HLSL files, computes expected DXIL filenames, deletes any DXIL in `Bin/Shaders/DXIL/` that doesn't have a corresponding source. Faster but more fragile.

Option 1 is the safest first cut. Option 2 is the optimization if rebuild time becomes a problem.

The CMake deploy step (currently `cmake -E copy_directory Bin/Shaders/DXIL Bin/RelWithDebInfo/Shaders/DXIL`) should similarly be replaced with a mirror operation: `cmake -E remove_directory <target>` before copy, OR use a sync-style command that removes orphans.

Same concern likely applies to `Bin/Data/` deploy chain — check whether deleted source-tree component / scene files persist in `Bin/Data/`.

### Why high priority

This bug has been latent since the build system was set up. Every shader-touching CL has been at risk of this contamination silently for unknown duration. The phantom regression chain that surfaced it cost hours of dispatch time and ~5 commits of speculative fixes before being caught. Without this fix, ANY future bisect on shader work has the same contamination risk.

### Owner

`ci-build-expert` — owns CMake + automation scripts.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 HLSL2DXIL_NoPause.ps1 (and Scripts/HLSL2DXIL.ps1) clean Bin/Shaders/DXIL/ at start, OR enumerate orphans and delete
- [ ] #2 CMake deploy step uses mirror semantics (remove + copy, OR sync-with-delete) for Bin/RelWithDebInfo/Shaders/DXIL/
- [ ] #3 Same audit done for Bin/Data/ deploy chain — orphans removed if any
- [ ] #4 Validation: revert a known-modifying CL (e.g. delete a test .hlsl file from source), build, confirm Bin/Shaders/DXIL/ no longer contains the orphan .dxil
- [ ] #5 Document the mirror semantics in the build script comments so a future regression of this rule is self-explanatory
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
