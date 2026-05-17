---
id: TASK-236
title: >-
  Stale audit-mode log message at Engine_ParseInitConfig.cpp:145 — "frame 5" no
  longer meaningful under event-driven trigger
status: Done
assignee:
  - zhangdoa
created_date: '2026-05-17 16:56'
updated_date: '2026-05-17 17:47'
labels:
  - rendering
  - logging
  - stale
  - followup
dependencies: []
references:
  - 8355775a
  - Source/Engine/Engine_ParseInitConfig.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced by TASK-233 implementer (commit `8355775a`) per `surface-dont-chase`; not folded in.

`Source/Engine/Engine_ParseInitConfig.cpp:145` logs "Audit mode: will dump all pass outputs on frame 5." That message described the legacy magic-frame fence. Under the event-driven trigger landed in TASK-233, the dump fires K=25 render frames after the GISponza scene-load callback — no longer at any specific frame number.

Fix shape: update the log text to match the new behaviour, e.g. "Audit mode: will dump all pass outputs after scene-load + N settle frames." Trivial single-file change.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Log message at Engine_ParseInitConfig.cpp:145 updated to reflect event-driven trigger behaviour (no "frame 5" magic number)
- [x] #2 Build green
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
`Source/Engine/Engine_ParseInitConfig.cpp:145` log message updated from "Audit mode: will dump all pass outputs on frame 5." (stale since TASK-233 event-driven trigger landed) to "Audit mode: will dump all pass outputs after scene-load completes + settle frames." (matches actual behaviour without hard-coding the K=25 constant in two places).

Build green. Audit autotest re-run confirmed the new message appears at engine config parse: `[Success] [Inno::Engine::ParseInitConfig] Audit mode: will dump all pass outputs after scene-load completes + settle frames.`
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
