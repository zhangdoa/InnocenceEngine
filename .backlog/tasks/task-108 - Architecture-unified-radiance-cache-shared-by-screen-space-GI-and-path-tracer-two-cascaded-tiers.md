---
id: TASK-108
title: >-
  Architecture: unified radiance cache shared by screen-space GI and path tracer
  (two cascaded tiers)
status: To Do
assignee: []
created_date: '2026-04-19 19:27'
labels:
  - rendering
  - radiance-cache
  - path-tracer
  - research
  - architecture
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Idea

Rather than maintaining separate lighting caches for the rasterized screen-space GI path and the GPU path tracer, share a single radiance-cache data structure between them. The difference between "screen-space" and "world-space" reduces to *where the probes live*, not what they store or how they are queried.

- **Screen-space tier**: probes placed at G-buffer pixels (what AMD GI 1.0 does — hence TASK-6). Fast, camera-aware, stable over small frame deltas. Cheap to update.
- **World-space tier**: probes placed in the scene volume (voxel-grid / BVH-leaf / surfel-based). Survives camera movement, available to rays that leave the screen, natural fit for the path tracer's indirect bounces.

Both tiers write into and read from the same radiance representation (e.g. SH-coeffs, filtered samples, or a spatial hash keyed by position + normal). The two tiers form a *cascade*: near-camera queries preferentially hit the screen-space tier; off-screen or behind-geometry queries fall through to the world-space tier.

## Why this matters

- **The rasterized GI and the path tracer converge to the same physical answer**. Letting them share the cache means rays / probes either produce data the other consumes or benefit from data the other produced. No double compute.
- **AMD GI 1.0 is G-buffer-based** — it only knows about visible surfaces. Adding a world-space tier covers everything the G-buffer misses (behind camera, inside geometry pockets, reflected off-screen content). Without changing the G-buffer algorithm.
- **The path tracer is currently a hardcore geometric integrator** — it evaluates every bounce independently. Borrowing cached radiance from near-identical previous rays is pure speedup. Particularly valuable when combined with TASK-18 (mesh-shader / meshlet LOD) — LODs mean more effective cache hits because the same radiance is valid across LOD levels of the same cluster.

## Dependencies / adjacent work

- **TASK-6** (radiance cache alignment with AMD GI 1.0 reference) — implement the screen-space tier first using the AMD GI 1.0 reference as the baseline.
- **TASK-18** (mesh-shader / meshlet pipeline) — the world-space tier's probe granularity should align with meshlet boundaries where possible so LODs share cache entries.
- **TASK-77** (path tracer denoise + promote to primary) — the cache is a cheap denoiser: cache hits are low-variance, cache misses fall back to full integration.
- **TASK-98 / 99 / 104** (cloth / volumetric / clouds) — all of these want sampled radiance and would benefit from cache reuse.

## Scope (research task, not ship-now)

- **R&D doc** — compare cache representations (SH, spatial hash, octree, surfel) on cost vs. quality vs. cache-hit rate for the two tiers.
- **Prototype** — stand up the shared data structure behind both the screen-space GI update compute pass and the path tracer's indirect-lookup phase.
- **Test scenes** — Sponza (atrium + architecture = plenty of indirect bounces), an open outdoor scene, and a high-frequency-geometry scene (meshlet-friendly).

## Deliverables

- Architecture writeup with the cache API surface nailed down.
- Prototype sharing a cache between at least one screen-space GI pass and the path tracer.
- Performance numbers: with cache vs. without cache for both pipelines on the three test scenes.
- A decision point: does the shared cache win on both axes (cost + quality), only one, or neither — documented with data.
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
