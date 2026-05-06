---
id: TASK-221
title: 'closure-evidence: exemption for obsolete closures'
status: To Do
assignee: []
created_date: '2026-05-06 20:14'
labels:
  - harness
  - tech-debt
dependencies: []
references:
  - .claude/hooks/gates/test-run.js
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
<!-- SECTION:DESCRIPTION:BEGIN -->
The closure-evidence path of `gates/test-run.js` requires a qualifying integration test for any task flipping to `status: Done`, with no exemption for genuinely-obsolete closures. Hit during TASK-189 closure (commit `d749b157`): the task was a non-reproducible observation never validated and never re-triggered, but landing the closure required running `Main.exe -total_frames 1` purely to satisfy the gate.

Fix shape: recognise an explicit closure-reason marker (e.g. `closure_reason: obsolete` in frontmatter, or a `Closure-Reason: obsolete` commit footer) and bypass the test-run requirement when present. Keep the gate strict for normal "work landed" closures.
<!-- SECTION:DESCRIPTION:END -->
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
