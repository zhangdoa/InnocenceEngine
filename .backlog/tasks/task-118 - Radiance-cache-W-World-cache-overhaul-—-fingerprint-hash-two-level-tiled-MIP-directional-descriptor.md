---
id: TASK-118
title: >-
  Radiance cache [W] World-cache overhaul — fingerprint hash + two-level tiled
  MIP + directional descriptor
status: To Do
assignee: []
created_date: '2026-04-20 17:36'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rewrite the flat `WorldProbeGrid` into the paper's two-level spatial hash (§2.2):

- Hash structure: bucket table + per-bucket fingerprint-addressed entries (open-addressing linear probing). Two independent hash functions from Jarzynski–Olano 2020 — one for bucket, one for fingerprint — chosen for low cross-collision.
- Descriptor: `(quant(pos, level), quant(dir), short_ray_bit)` where `short_ray_bit = (ray_length < cell_size)` to break light leaks (paper §2.2.2). Quantization level adapts with distance → radiance LODs.
- Two-level layout: cells grouped into 8×8 tiles stored in linear memory with in-place MIP chain; 2D-projected along the largest component of outgoing direction (paper §2.2.3). Tile size = 8×8 empirically best.
- Decay-based eviction: each cell has decay counter reset on access, ticks toward zero otherwise; zero → freed.
- Per-vertex cached index: one atomic lookup per vertex, not per access.

Independent of [F]/[S1]/[S2]/[I] — can be done in parallel once [F] lands (closest-hit shader is the only overlap point).

Parent: TASK-6 (see Implementation Notes §[W]).
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
