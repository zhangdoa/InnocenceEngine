---
id: TASK-116
title: 'Radiance cache [S2] Screen-cache robustness — LRU side cache + mask-MIP filter'
status: To Do
assignee: []
created_date: '2026-04-20 17:36'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Persistent LRU side cache for evicted probes (paper §2.1.8): separate 2D texture for radiance; per-entry 128-bit `(float3 pos, packed_normal)`; list built per 8×8 tile by scatter (projected screen coords) so reconstruction can query a 3×3 tile neighborhood of cached probes after the reprojected-grid search. MRU reorder pass per frame; decay-based eviction of LRU entries.

Rewrite the separable 7×7 bilateral filter (paper Algo 5) to: walk the mask MIP via `find_closest_probe` instead of reading position textures, apply parallax correction per tap, enforce the 50-unit-relative direction threshold, respect the unified adaptive cell size.

Depends on [F] (mask MIP) and [S1] (eviction signal).

Parent: TASK-6 (see Implementation Notes §[S2]).
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
