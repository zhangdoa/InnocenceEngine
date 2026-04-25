---
id: TASK-128
title: Extract reusable ping-pong texture helper to shrink GI pass classes
status: To Do
assignee: []
created_date: '2026-04-25 00:00'
updated_date: '2026-04-25 19:46'
labels:
  - rendering
  - refactor
  - code-quality
dependencies:
  - TASK-6.2
references:
  - Source/Engine/RenderingClient/GIDenoisePass.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Discovered during TASK-125 CL1 implementation. Adding the three new ping-pong texture pairs (`m_PrevWorldPos_{Even,Odd}`, `m_ColorDelta_{Even,Odd}`, plus the pre-existing `m_GIHistory_{Even,Odd}` and `m_Moments_{Even,Odd}`) grew `Source/Engine/RenderingClient/GIDenoisePass.cpp` from 297 → 453 lines, mostly in repetitive resource creation, transition, and binding boilerplate. CL1 used `[skip-size-gate]` to land; the underlying responsibility split is real and the boilerplate is the symptom.

### Pattern to extract

A `PingPongTexture` (or similarly named) RAII type that:
- Owns `Even` / `Odd` `TextureComponent*` pairs.
- Exposes `GetPrev()` / `GetCurr()` accessors based on a frame-parity input (or an internal toggle advanced once per frame by the owner pass).
- Centralises transition + binding for both halves, so callers state intent ("bind prev as SRV, curr as UAV for this dispatch") in one line instead of four.
- Likely lives in foundation or rendering-client common — exact placement to be decided at design time once the call sites are surveyed.

### Beneficiaries

The implementer should grep the codebase for existing hand-rolled ping-pong patterns and list every site in this task's Implementation Notes before refactoring. Known starting points:

- `Source/Engine/RenderingClient/GIDenoisePass.cpp` — surviving pairs after the GI denoiser dust settles. The pair count is in flux while TASK-6.2 lands (TASK-6.2 deletes the `m_Moments_{Even,Odd}` pair); survey at refactor time against the actual current state, not against any historical CL.
- The radiance-cache passes (probe history, screen cache reprojection, world-cache MIPs — at least one of these ping-pongs by hand).
- Any TAA / motion-blur passes that exist in the engine.

The list above is a starting scope, not authoritative. Survey before extracting.

### Schedule constraint: depends on TASK-6.2

TASK-6.2 deletes the `m_Moments_{Even,Odd}` pair and trims `GIDenoisePass`'s descriptor layout from 17 → 15 entries. Doing this extraction before TASK-6.2 lands means templating the wrong set of pairs and immediately re-doing the survey + descriptor accounting once TASK-6.2 trims the layout. Sequence: TASK-6.2 first (focused descriptor cleanup, no behaviour change), then TASK-128 (cross-cutting helper extraction).

If TASK-6.2 stalls and TASK-128 becomes urgent for an unrelated reason (e.g. another pass adds a fifth pair and trips the file-size gate again), the two can be merged into one CL — but the default is sequential, since the helper extraction is naturally larger and benefits from a stable target.

### Why this is low priority

Pure quality-of-life cleanup. Not a defect, not a blocker on any feature, no user-visible symptom. Drives the file-size gate compliance for `GIDenoisePass.cpp` and (more importantly) reduces the failure modes of "forgot to transition the second half of the pair" type bugs that hand-rolled ping-pongs are prone to.

### Cross-cutting: not a child of TASK-125 or any single feature

This is foundation / shared-utility work. No parent task; the dependency on TASK-6.2 is only a scheduling constraint, not a parent-of relationship.
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
