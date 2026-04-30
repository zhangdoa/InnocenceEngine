---
id: TASK-108
title: >-
  Architecture: unified radiance cache shared by screen-space GI and path tracer
  (two cascaded tiers)
status: Done
assignee: []
created_date: '2026-04-19 19:27'
updated_date: '2026-04-30 21:30'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Status note (2026-04-30) — paused pending TASK-77

Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

This task's premise is a **shared cache between two consumers** (screen-space GI + path tracer). Under PT-primary, the screen-space-GI consumer is on the demotion path — the second consumer that justified the "shared" framing isn't load-bearing for the project's direction anymore. Only the PT-side world-space cache survives as a candidate consumer.

**Paused, not closed.** Waiting on TASK-77's denoiser-axis outcome (see TASK-77.1 — temporal-reprojection denoiser as the cheapest first axis). Once TASK-77's denoiser direction settles and the user decides whether radiance caching wins over pure spatial+temporal denoising for the PT, this task either:

- **Rescopes** to "PT-only world-space radiance cache as denoiser" — which would substantially overlap with TASK-77's ReSTIR / light-BVH / faster-convergence sub-tasks. The architecture writeup, prototype, and perf-numbers deliverables would shift to PT-only validation.
- **Archives** — if the user picks pure denoising over caching, or if TASK-77.1's temporal-reprojection result is good enough to render the cache-as-denoiser axis unnecessary.

Status stays `To Do` per project precedent (no `Paused` / `Deferred` enum value); the deferred-pending state lives in this note. Re-evaluate after TASK-77.1's design call closes.

**Cross-ref**: TASK-77 (PT-primary direction approval), TASK-77.1 (temporal-reprojection denoiser — phase 1, the trigger for this re-evaluation), TASK-6 family (originally cited as the screen-space-tier trigger; now in maintenance mode).
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Superseded by TASK-77.1 (2026-04-30)

Closed Done as **superseded by TASK-77.1**. The PT-side world-space radiance cache scope — the only surviving subset of this task under PT-primary direction — folded into TASK-77.1's rewritten phase-1 brief on the same day this task was paused.

### What triggered the closure earlier than expected

The 2026-04-30 status note (above) framed re-evaluation as "after TASK-77.1's design call closes." The trigger arrived earlier — at TASK-77.1's filing, not its design call:

User direction (2026-04-30, same session): *"motion vectors come from a g-pass which is still rasterized; world-space cache makes more sense here."* Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

The PT denoiser must not depend on rasterizer-derived inputs (G-buffer motion vectors, depth, normals) — that would couple the surviving primary pipeline to a demoted subsystem. World-space radiance cache is the rasterizer-independent alternative, and it was already this task's surviving subset (the PT-side world-space tier). TASK-77.1 pivoted to that approach in the same backlog batch.

### What folded where

- **Architecture writeup** deliverable → TASK-77.1's design-call output (cache structure picked + rationale + reference-impl alignment).
- **Prototype** deliverable → TASK-77.1's implementation lanes.
- **Perf numbers** deliverable → TASK-77.1's AC #4 (memory footprint) + AC #6 (visual A/B + noise-floor delta).
- **Decision point** ("does the shared cache win on both axes") → no longer load-bearing; the "shared" framing is gone (SSGI-side consumer demoted), so only the PT-side cost/quality question matters and that question moves to TASK-77.1.

### ACs left unticked

ACs #1-#6 (DoD section) all remain unticked — they were not done; they are now irrelevant under the supersede:

- DoD #1-#4 (build, integration tests, mock-test gap, validation) — no implementation landed under this task ID.
- DoD #5-#6 (user-observable outcome, what-was-not-verified) — moves to TASK-77.1's Final Summary when that phase closes.

This matches the closure shape of TASK-153 in commit `3dcead84` (obsolete-framing closure under PT-primary, ACs left unticked, scope folded into successor task / no follow-up filed).

### No follow-up task filed

The "shared cache between two consumers" framing is itself obsolete. The right successor is TASK-77.1's PT-only world-space cache, not a re-incarnation of this task. SSGI-side cache work is on the demotion path — if it ever comes back, it does so as a debug-comparison harness, not as a peer of the PT cache.

**Cross-ref**: TASK-77 (PT-primary direction approval), TASK-77.1 (PT-only world-space radiance cache as denoiser — phase 1; supersedes this task's PT-side scope), TASK-153 (precedent for obsolete-framing closure shape under PT-primary), TASK-6 family (originally cited as the screen-space-tier trigger; now in maintenance mode under PT-primary).
<!-- SECTION:FINAL_SUMMARY:END -->
