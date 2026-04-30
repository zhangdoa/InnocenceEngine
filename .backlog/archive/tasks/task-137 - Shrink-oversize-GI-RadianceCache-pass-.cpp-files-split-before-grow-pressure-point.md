---
id: TASK-137
title: >-
  Shrink oversize GI/RadianceCache pass .cpp files (split-before-grow pressure
  point)
status: To Do
assignee: []
created_date: '2026-04-25 22:39'
updated_date: '2026-04-30 19:11'
labels:
  - rendering
  - refactor
  - split-before-grow
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/GIDenoisePass.cpp
  - Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from the TASK-127 commit (2026-04-25). The fix-CL hit the commit-gate's 400-line soft-ratchet on two files:

- `GIDenoisePass.cpp` 451 → 460 lines (+9, mostly a load-bearing TASK-127 hazard comment + ceiling-divide migration)
- `RadianceCacheReprojectionPass.cpp` 449 → 450 (+1, the unavoidable `#include "RadianceCacheConstants.h"`)

Both files were already over the 400-line threshold before TASK-127. The CL committed via `[skip-size-gate]` because the additions were necessary for the bug fix (single-source-of-truth migration), but the size pressure is real and the files are growing.

### What to look at

Per `.claude/disciplines/split-before-grow.md`, an oversize-and-growing C++ file usually has a separable responsibility. Likely candidates inside these passes:

- **`PrepareCommandList`** is typically the bulk of a render-pass `.cpp`. The binding-list section can often factor out into a helper (one binding-table-builder per pass family, or per-stage helpers).
- **`RenderTargetsCreationFunc`** — texture allocation / descriptor setup is often duplicated in shape across passes; consolidate per-subsystem.
- **`GetResult` / `GetSideResult` / accessor jumble** — straightforward to push into the header as inline.

### Constraint

Behavior-preserving. This is a structural cleanup, not a refactor. Don't fold in TASK-6.4 / TASK-136 work; those are separate.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 GIDenoisePass.cpp under 400 lines or with a clear pre-and-post line count justifying any remaining excess
- [ ] #2 RadianceCacheReprojectionPass.cpp under 400 lines or with a clear pre-and-post line count justifying any remaining excess
- [ ] #3 No behavior change — same offscreen capture before vs after
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Archived under PT-primary direction (2026-04-30)

Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

This task was pure split-before-grow hygiene on `GIDenoisePass.cpp` (460 lines) and `RadianceCacheReprojectionPass.cpp` (450 lines) — both screen-space-GI-as-rasterizer-feature passes. The soft-ratchet pressure that motivated the task was the assumption these files would continue to grow as the screen-space GI pipeline matured. Under PT-primary that assumption no longer holds: the SSGI / RadianceCache passes are now in maintenance / debug-comparison mode. With no expected new lines, the files are not the growing-and-oversize pair the discipline is targeting.

If a future SSGI feature does add lines (unlikely under the new direction — the quality work is being routed into PT denoising and convergence instead, see TASK-77.1 and other TASK-77 phase tasks), file fresh at that moment with current-state line counts. The discipline's correct trigger is "oversize AND growing"; without the growth signal, this is dead-letter cleanup.

**Cross-ref**: TASK-127 (the CL that originally tripped the soft-ratchet), TASK-77 (PT-primary direction approval), TASK-128 (ping-pong helper extraction — also demoted under PT-primary in the same batch).
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
