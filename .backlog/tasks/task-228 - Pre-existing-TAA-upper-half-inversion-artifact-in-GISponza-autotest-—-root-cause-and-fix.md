---
id: TASK-228
title: >-
  Pre-existing TAA upper-half-inversion artifact in GISponza autotest —
  root-cause and fix
status: To Do
assignee: []
created_date: '2026-05-16 21:08'
labels:
  - rendering
  - bug
  - TAA
  - regression
dependencies: []
references:
  - .alignments/TASK-226.4-port-audit.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-226.4 peer review (commit e0e68900). The GISponza autotest frame capture shows the upper half of the framebuffer rendering an inverted (upside-down) view, plus a central black void in the scene. Pre-existing — present in both the pre-change and post-change captures of the TASK-226.4 review. Not caused by TASK-226.4.

Captured A/B frames at the time of discovery: `Bin/RelWithDebInfo/gpu_output_0055_baseline.png` and `gpu_output_0055_postchange.png` (may be cleaned between sessions — re-capture if needed).

Likely suspects (need bisect):
- TAA pass — motion-vector convention mismatch between RT0/RT3 GBuffer and TAA-pass reprojection.
- Final-blend / composition — UV flip in a y-axis transform.
- Renderer split between rasterized + GI passes — incorrect render-target slice composition.

This artifact has been present long enough that it shipped through several TASK-226.x docs commits without being flagged; either it was a recently-introduced regression masked by side-cache fill or a longer-standing issue. Bisect against the TASK-226.2 baseline frames if available.

Blocks: clean baseline for TASK-226.8 visual + perf gate (which compares against TASK-226.2 baseline). Without addressing this, fine-grained AC #4/#5 disocclusion-fill comparison is impractical.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause identified via bisect / RenderDoc capture (TAA / composition / GBuffer-flip / other)
- [ ] #2 GISponza autotest frame capture renders correctly — no upper-half inversion, no central black void
- [ ] #3 Bisect range + breaking commit recorded in alignment / closure note
- [ ] #4 Fix landed and visual A/B vs known-good baseline confirms regression closed
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
