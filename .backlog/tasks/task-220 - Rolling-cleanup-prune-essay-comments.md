---
id: TASK-220
title: 'Rolling cleanup: prune essay comments'
status: To Do
assignee: []
created_date: '2026-05-05 20:26'
labels:
  - tech-debt
  - code-quality
  - rolling-cleanup
dependencies: []
references:
  - .claude/disciplines/always/comment-discipline.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rolling sweep against `disciplines/always/comment-discipline.md`. Per CL: pick one file (or tight cluster), drop comments not justified by the discipline, no behavior change, build + qualifying test green. Owner: stage that owns the file.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Each CL prunes one file or cluster; build + test green; no behavior change.
- [ ] #2 Comments kept only when the discipline says keep.
- [ ] #3 Stays open as a rolling tracker; new violations caught at write-time, not refiled here.
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
