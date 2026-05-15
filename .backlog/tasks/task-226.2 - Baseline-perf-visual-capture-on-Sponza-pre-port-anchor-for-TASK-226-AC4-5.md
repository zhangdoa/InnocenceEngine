---
id: TASK-226.2
title: >-
  Baseline perf + visual capture on Sponza (pre-port anchor for TASK-226
  AC#4/#5)
status: To Do
assignee: []
created_date: '2026-05-15 20:24'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
  - baseline
dependencies: []
references:
  - .alignments/TASK-226-gap-matrix.md
parent_task_id: TASK-226
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Capture pre-port reference state on the Sponza autotest scene so TASK-226's AC #4 (visual parity vs Capsaicin) and AC #5 (60-FPS bar) have a concrete anchor before any port stages land.

Outputs (all under `.alignments/TASK-226-baseline/`):
- Frame-time numbers from the autotest camera path: min/avg/max ms per frame, plus the rasterized-GI-on vs RasterizedGI-off ratio (use the existing dev toggle).
- 4-angle screenshot set: same camera positions used in TASK-6.6 / TASK-189 captures, RasterizedGI=ON.
- Same 4-angle set with RasterizedGI=OFF (PT-only) for the comparison delta.
- A `baseline.md` recording GPU model, driver, build commit SHA, scene name, and per-frame settings.

Anchors AC #5 (60 FPS) — once baseline is known, the AC can be re-evaluated as "no regression vs baseline" if the 4× ray-count + bent-cone changes (TASK-226.5 / .6 / .7) push past the 60 FPS bar.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Frame-time numbers (min/avg/max) captured on Sponza autotest camera, RasterizedGI=ON and =OFF
- [ ] #2 #2 4-angle screenshot set captured at both toggle states, saved under .alignments/TASK-226-baseline/
- [ ] #3 #3 baseline.md records build SHA, GPU/driver, scene config
- [ ] #4 #4 No code changes — capture artifact only
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
