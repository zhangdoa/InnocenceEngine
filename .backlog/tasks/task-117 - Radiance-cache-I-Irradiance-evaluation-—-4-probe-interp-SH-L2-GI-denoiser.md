---
id: TASK-117
title: >-
  Radiance cache [I] Irradiance evaluation — 4-probe interp + SH L2 + GI
  denoiser
status: To Do
assignee: []
created_date: '2026-04-20 17:36'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
LightPass GI consumption rewrite (paper §2.4):

- Per-pixel: jitter pixel position (cancel if crossing plane), find 4 neighbor probes via `find_closest_probe`, edge-aware weights from depth + normal, relaxed-interpolation fallback (equal weights) when all neighbors are rejected. Flag relaxed pixels in alpha for denoiser hint.
- SH upgrade: bands 0–2 (9 coefficients × 3 channels). Storage layout: pick between atlas expansion or packed buffer (decide at impl time based on shader access patterns; document choice).
- Irradiance = SH radiance · analytic projected cosine lobe (Ramamoorthi–Hanrahan).
- Spatiotemporal GI denoiser: temporal accumulation + adaptive-radius spatial filter sized by accumulated sample count; disocclusion mask → dilated blur mask (paper Fig. 19).

Depends on [S1]+[S2] — probes need to be converging and filtered before consumption changes.

Parent: TASK-6 (see Implementation Notes §[I]).
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
