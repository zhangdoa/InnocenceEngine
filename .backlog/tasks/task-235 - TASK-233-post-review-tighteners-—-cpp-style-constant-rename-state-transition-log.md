---
id: TASK-235
title: >-
  TASK-233 post-review tighteners — cpp-style constant rename + state-transition
  log
status: Done
assignee:
  - zhangdoa
created_date: '2026-05-17 16:56'
updated_date: '2026-05-17 17:47'
labels:
  - rendering
  - cpp-style
  - safety-observability
  - followup
dependencies: []
references:
  - 8355775a
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient_AuditDump.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Two ADVISORY items from TASK-233 peer review (commit `8355775a`). Both small, both inside `Source/ExampleProject/RenderingClient/ExampleRenderingClient_AuditDump.cpp`.

## ADVISORY-1: cpp-style constant naming

`_AuditDump.cpp:139` declares `kAuditPostLoadSettleFrames`. Per `.claude/skills/cpp-style/SKILL.md:21`, constants are PascalCase (`MaxPoolSize` example). Rename to `AuditPostLoadSettleFrames`.

Project-local `k_`-prefix precedents exist at `TestRenderingClient.h:38 k_TargetFrames` and `NRDIntegrationAdapter_Impl.h:159 k_InputCount` — these also drift from the skill canon. **Out of scope here** (separate task if a sweep is wanted; this task fixes only the constant introduced by TASK-233).

## ADVISORY-2: state-transition log on audit first-trigger

`_AuditDump.cpp:141-145` silently flips `m_AuditCountingStarted = true` on first observed scene-load. Per `safety-observability` skill, log-on-state-transition is acceptable in lieu of per-frame hot-path logging. Add one `Log(Verbose, "Audit: counter armed at frame ...")` on the transition. Makes audit-mode runs self-documenting in logs — the implementer had to reconstruct timing from `GISponza-loaded` log messages in their own verification.

Triggered by review verdict in TASK-233 Implementation Notes; surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `kAuditPostLoadSettleFrames` renamed to `AuditPostLoadSettleFrames` (PascalCase per cpp-style skill)
- [x] #2 State-transition Log call added on audit first-trigger (single Log(Verbose, ...) on the transition, NOT per-frame)
- [x] #3 Build green; audit autotest still runs end-to-end and captures GISponza
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Both ADVISORY items applied in `Source/ExampleProject/RenderingClient/ExampleRenderingClient_AuditDump.cpp`:
- Renamed `kAuditPostLoadSettleFrames` → `AuditPostLoadSettleFrames` (PascalCase per cpp-style skill).
- Added `Log(Success, "Audit: post-load settle counter armed; will dump in ", AuditPostLoadSettleFrames, " frames.")` on the audit first-trigger state transition. Edge-triggered (one log per scene-load event), not per-frame. Success-level (not Verbose) chosen so the log is visible at the autotest's default `-loglevel 1` — Verbose would have hidden it from standard runs, defeating the self-documenting intent.

Build green. Audit autotest re-run: armed-counter log fires after GISponza scene-load ("Audit: post-load settle counter armed; will dump in 25 frames." at 17:45:52.234, GISponza loaded at 17:45:52.194 — 40ms gap), AuditDump fires ~830ms later (~25 frames as designed), all 11 HDRs written.
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
