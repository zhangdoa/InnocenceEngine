---
id: TASK-77.1.3
title: Visual A/B + paper-port audit + closure (TASK-77.1 phase 1 sub-3)
status: To Do
assignee: []
created_date: '2026-04-30 19:44'
updated_date: '2026-05-21 20:44'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
  - validation
dependencies: []
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

End-to-end validation of the phase-1 world-space-radiance-cache denoiser. On-screen visual A/B (cache off vs. on, GISponza, fixed camera path), per-pixel stddev numeric noise-floor measurement, rasterizer-independence smoke test (rasterizer pipeline `IsBypassed` end-to-end → PT + cache + denoise still produces a correct frame). Paper-port audit per `paper-port.md` discipline acknowledging the primary-vs-secondary-read divergence from Capsaicin GI-1.0.

## Owner

`rendering-researcher` — validation lead, paper-port audit. Captures, numeric measurement, and the final closure record live with the algorithmic owner.

## Validation methodology

- **On-screen visual A/B**: stills + short video, GISponza scene, multiple camera angles, cache off vs. on side-by-side. On-screen / windowed test per `feedback_onscreen_testing` (offscreen-only does not satisfy this AC).
- **Numeric noise-floor**: per-pixel stddev across 30 frames at fixed camera, target ≥5× drop with cache on.
- **Rasterizer-independence smoke test**: with rasterizer bypassed end-to-end, PT + cache + denoise produce a correct frame; capture proof (RenderDoc or screenshot transcript).
- **Paper-port audit**: paper-auditor sub-agent dispatch on the closing CL, primary-read divergence from Capsaicin acknowledged in implementation notes.

## Out of scope

- Hash-grid authoring (TASK-77.1.1).
- Denoise pass authoring (TASK-77.1.2).
- Phase 2 secondary writes/reads, capacity tuning, colour-delta invalidation (deferred — see TASK-77.1 Implementation Notes).

## Cross-refs

- Parent: TASK-77.1.
- Hard dependencies: TASK-77.1.1 + TASK-77.1.2 (both must land first).
- Discipline anchors: `peer-review-required.md`, `paper-port.md`, `regression-fix-flow.md`, `feedback_onscreen_testing`, `feedback_pt_comparison_must_account_for_rast_omissions`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 On-screen visual A/B captures: stills + short video, GISponza scene, multiple camera angles, cache off vs. on side-by-side
- [ ] #2 Numeric noise-floor check: per-pixel stddev across 30 frames at fixed camera, drops by ≥5x with cache on
- [ ] #3 Rasterizer-independence smoke test: with rasterizer bypassed end-to-end, PT + cache + denoise produce a correct frame; capture proof
- [ ] #4 Paper-auditor sub-agent dispatch on the closing CL, primary-read divergence from Capsaicin acknowledged in implementation notes
- [ ] #5 Closure record with implementer notes; status Done
- [ ] #6 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
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
