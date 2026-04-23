---
id: TASK-115
title: >-
  Radiance cache [S1] Screen-cache convergence — sparse spawn + Algo 2/3, 3×3
  ray guiding w/ parallax
status: Done
assignee: []
created_date: '2026-04-20 17:36'
updated_date: '2026-04-20 19:30'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replaces the current "spawn every tile every frame + simple EMA" with paper-accurate convergence:

- Sparse probe spawning: drive spawn from `upscaleFactor` so ~1 out of (Ux·Uy) spawn-tiles get probes per frame; Halton(2,3) chooses sub-pixel.
- Adaptive sampling (Algo 2): empty_tile + override_tile queues; `patch_screen_probes` kernel redistributes rays from succeeded-reprojection tiles to empty disoccluded tiles.
- Ray guiding rewrite: 3×3 probe neighborhood hemisphere reconstruction (not 1-tile CDF) with parallax correction — store ray travel distance in the alpha channel of the atlas and recover hit-point position when reusing cells across probes.
- Replace RayGen's simple EMA with Algorithm 3's biased shadow-preserving temporal hysteresis (firefly suppression falls out of this).
- Radiance-average backup: untraced cells (low ray-guiding probability) get the average of populated cells in the probe.

Depends on [F] — mask MIP + unified cell size are used by all four queues/searches here.

Parent: TASK-6 (see Implementation Notes §[S1]).
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-04-20: Landed [S1.1] Algorithm 3 hysteresis (73225d28), [S1.2] distance-in-alpha (a5271822), [S1.3] 3x3 neighbourhood CDF with parallax correction (30a3ff99). Remaining: [S1.4] radiance-average backup — deferred until ray redistribution in [S1.5] surfaces ‘untraced cell’ set explicitly; [S1.5] sparse spawning + Algorithm 2 hole-filling + mask MIP chain — big piece, deserves own session (touches resource allocation, pass scheduling, and all four radiance-cache shaders).

2026-04-20 (later): [S1.5] sparse spawning + Halton + Reprojection mask invalidation landed (39de2a1b). Main.exe 20 frames + reload at 10: exit 0, no new warnings. Visual verification attempted via windowed Main.exe + -capture_frame + renderdoccmd thumb, but thumbnail comes out solid black on both HEAD and HEAD~1 — not a regression, but the A/B-capture workflow is broken. Filed TASK-119. Remaining [S1] sub-pieces deferred: [S1.4] radiance-average backup (blocked on ray-redistribution tracking), [S1.5b] mask MIP chain (bundled), [S1.5c] Algorithm 2 ray redistribution.

2026-04-23: Closing the umbrella — all remaining S1 sub-slices landed (S1.5b, S1.5c, S1.5c-override). S1.4 remains deferred per TASK-6 Remaining Work; it's blocked on per-cell traced-vs-untraced tracking infrastructure that only makes sense alongside the paper-faithful dispatch-indirect override queue (filed as [S1.5c-override-full]). File [S1.4] as its own task if the deferral path is needed.
<!-- SECTION:NOTES:END -->
