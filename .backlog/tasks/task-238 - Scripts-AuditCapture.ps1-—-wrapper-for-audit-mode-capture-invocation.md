---
id: TASK-238
title: Scripts/AuditCapture.ps1 — wrapper for audit-mode capture invocation
status: To Do
assignee: []
created_date: '2026-05-17 16:57'
labels:
  - tooling
  - scripts
  - audit
  - followup
dependencies: []
references:
  - 8355775a
  - Scripts/BuildWin.ps1
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced by TASK-233 implementer (commit `8355775a`) per `surface-dont-chase`.

Audit-mode capture is invoked today via: `Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 60 -offscreen audit`. The flag combination is undocumented outside commit messages and skill examples. A `Scripts/AuditCapture.ps1` wrapper would:

- Encode the flag baseline as defaults.
- Accept overrides (renderer, total_frames, scene).
- Write captures to a deterministic path (`Build/captures/<tag>/`) for diff workflows.
- Print the invocation and capture path on completion.

Matches the project pattern under `Scripts/` (BuildWin.ps1, etc.). Low priority — current CLI works; this is convenience tooling.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Scripts/AuditCapture.ps1 wraps the audit-mode invocation with sensible defaults and parameter overrides
- [ ] #2 Captures land in deterministic path (default Build/captures/<tag>/)
- [ ] #3 Script documented in Scripts/CLAUDE.md per project pattern
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
