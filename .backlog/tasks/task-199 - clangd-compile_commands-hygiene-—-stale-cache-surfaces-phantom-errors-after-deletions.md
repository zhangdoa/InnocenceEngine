---
id: TASK-199
title: >-
  clangd compile_commands hygiene — stale cache surfaces phantom errors after
  deletions
status: To Do
assignee: []
created_date: '2026-04-29 07:19'
labels:
  - clangd
  - ci-build
  - tooling-hygiene
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

clangd's index/cache repeatedly surfaces diagnostics for files that don't exist on disk:

- `Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp` — file-not-found for `.h`; `SunShadowGeometryProcessPass` undeclared (10+ errors). File was deleted in commit `a47efda3` (TASK-138 phase 2 — sun CSM swapped for RT shadows).
- `Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp` — same shape. File deleted in commit `f41a4ffb` (TASK-177 cube-shadow stack).
- `Source/Engine/Engine.cpp:677-704` — `Pasting formed '<HIDService'` invalid preprocessing token errors. Verified actual code at those lines is fine (`SystemSetup(HIDService);` etc.). clangd reading stale cached translation unit.

Pattern: every session opens with diagnostic noise for files/state that doesn't match the on-disk truth. We've been ignoring it. That's a feedback_no_dismissing_tool_noise.md violation building up over time.

## Goal

Make clangd's view of the project always reflect on-disk truth — automatically, without manual intervention per session.

## Likely approaches (pick during design)

- **A — Post-commit / post-rebuild hook.** Trigger `compile_commands.json` regeneration when the file set changes. May be a CMake reconfigure or a custom CDB regenerator.
- **B — `.clangd` config tweak.** Index-on-save, index-purge-on-mismatch, or background re-index more aggressively.
- **C — Document a manual reset SOP.** Less ideal — discipline rots — but acceptable as a stopgap if (A) and (B) are heavy.

## Acceptance criteria

- [ ] Diagnostic noise for deleted files does not survive past one session boundary
- [ ] Engine.cpp-style stale-token-paste errors don't recur
- [ ] Solution is documented in `Scripts/` or `.claude/disciplines/` (SOP) or wired into a build hook
- [ ] No regression in clangd functionality (autocomplete, find-refs)

## Owner

`ci-build-expert` (CMake / clangd integration — Scripts subtree).

## References

- Sample stale-cache errors above (recurring throughout this session's transcripts)
- `.clangd` config (if exists at repo root)
- `compile_commands.json` location (likely `Build/` per CMake export)
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
