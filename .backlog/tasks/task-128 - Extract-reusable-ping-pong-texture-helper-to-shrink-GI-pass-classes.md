---
id: TASK-128
title: 'Extract reusable ping-pong texture helper to shrink GI pass classes'
status: To Do
assignee: []
created_date: '2026-04-25 00:00'
labels:
  - rendering
  - refactor
  - code-quality
dependencies:
  - TASK-125
references:
  - Source/Engine/RenderingClient/GIDenoisePass.cpp
parent_task_id: ''
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

- `Source/Engine/RenderingClient/GIDenoisePass.cpp` — at least four pairs after CL3 lands (CL3 deletes the moments pair, so post-CL3 the count will differ; survey at refactor time).
- The radiance-cache passes (probe history, screen cache reprojection, world-cache MIPs — at least one of these ping-pongs by hand).
- Any TAA / motion-blur passes that exist in the engine.

The list above is a starting scope, not authoritative. Survey before extracting.

### Schedule constraint: defer until AFTER TASK-125 CL2 + CL3 land

CL2 and CL3 of TASK-125 actively modify the set of ping-pong textures in `GIDenoisePass`:
- CL3 deletes the moments ping-pong entirely (replaced by the dilated-blur-mask spatial filter from the paper §2.4.3).
- CL2 / CL3 may add or remove additional pairs depending on how the paper-aligned denoiser is structured.

Doing this extraction before CL3 is wasted churn — the pairs we'd template would be the wrong set. Do it after CL3 lands so the survey reflects the final state.

### Why this is low priority

Pure quality-of-life cleanup. Not a defect, not a blocker on any feature, no user-visible symptom. Drives the file-size gate compliance for `GIDenoisePass.cpp` and (more importantly) reduces the failure modes of "forgot to transition the second half of the pair" type bugs that hand-rolled ping-pongs are prone to.

### Cross-cutting: not a child of TASK-125 or any single feature

This is foundation / shared-utility work. No parent task; the dependency on TASK-125 is only a scheduling constraint, not a parent-of relationship.
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
