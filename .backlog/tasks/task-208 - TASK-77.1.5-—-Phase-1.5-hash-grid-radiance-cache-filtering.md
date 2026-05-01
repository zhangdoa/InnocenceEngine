---
id: TASK-208
title: TASK-77.1.5 — Phase-1.5 hash-grid radiance cache filtering
status: To Do
assignee:
  - rendering-researcher
created_date: '2026-05-01 06:39'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies:
  - TASK-77.1.3
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

Phase-1 of the world-space hash-grid radiance cache (TASK-77.1) shipped wiring + atomic-correctness (InterlockedAdd via `quantizedRadianceSum`). AC #2's noise-floor target (≥5× drop) was deliberately deferred — phase-1's primary-hit-read framing + missing filtering layer cap the achievable variance reduction. This task lands the filtering work that makes the cache actually denoise.

## Scope (priority order, per implementer's gap analysis at TASK-77.1.3 closure)

1. **EMA filter pass** mirroring Capsaicin's `gi1.comp:2160-2217`. Per-frame scratch buffer + separate kernel that EMA-blends scratch into a persistent `ValueBuffer` with a `max_sample_count` clamp. This is what bounds bright-outlier bias from sun-NEE rays — currently the unbounded mean lets one bright sample dominate the cached cell.
2. **Power-of-2 cell-size quantisation** mirroring `hash_grid_cache.hlsl:96-102`. Current continuous cell-size approaches a single screen-space pixel at Sponza-interior depths; Halton-jittered camera samples cross cells between frames, defeating accumulation. Power-of-2 quantisation locks neighbouring frames to the same cell.
3. **NEE-variance source examination.** Phase-1's primary-hit-read framing exposes sun-direct visibility variance the cache architecture isn't designed to filter. The paper's variance-reduction guarantee assumes secondary-vertex reads (D6/D7 in alignment artifact, scope-locked from phase-1). Decide in this task: is secondary-vertex read in phase-1.5's scope, or is it phase-2? If phase-2, document why phase-1.5's expected noise reduction has a ceiling and what the realistic AC bar should be.

4. **Atomic sampleCap race + uint32-wrap risk** (folded from TASK-77.1.3 peer-review ADVISORY-1). Current `HashGridCache.hlsl` cap-check at lines 266-276 lets a full 8×8 wavefront (~64 threads hashing to one cell on a flat near-camera surface) race past the cap on a single frame. Worst-case per-frame sum growth = 64 × 1e5 × 1e3 = 6.4e9 — wraps uint32 in one bad frame. The existing comment underestimates this. Two options when redesigning under this task: (a) atomic `InterlockedCompareExchange` cap-check on `sampleCount` before issuing the radiance adds; (b) the per-frame scratch + EMA pass shape from item 1, which sidesteps the race entirely by separating accumulation from the persistent ValueBuffer. Option (b) is the priority-1 work above — verify it covers this concern, otherwise add (a) as a sub-fix.

## Closes

- TASK-77.1.3's deferred AC #2 (≥5× noise-floor drop on at least one of {p50, p95, p99} cache-on vs cache-off, GISponza fixed-camera 30-frame stddev).

## Cross-refs

- Parent: TASK-77.1 (phase-1 closes alongside this; TASK-77 umbrella stays open until rasterizer demotion lands).
- Hard dependency: TASK-77.1.3 (must close first — this is the follow-on filtering work).
- Alignment artifact: `.alignments/TASK-77.1.3-hash-grid-cache-paper-port.md` divergences D3, D6, D7, D9, D10 — D9 partially resolved, the others are this task's surface.
- Capsaicin reference: `gi1.comp:2160-2217` (EMA filter), `hash_grid_cache.hlsl:96-102` (power-of-2 quantisation).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 EMA filter pass authored mirroring Capsaicin gi1.comp:2160-2217: per-frame scratch buffer + separate kernel + persistent ValueBuffer with max_sample_count clamp
- [ ] #2 Power-of-2 cell-size quantisation in HashGridCache_CellSize matching hash_grid_cache.hlsl:96-102
- [ ] #3 NEE-variance source examined; secondary-vertex read either in scope (with implementation) or out-of-scope (with documented rationale + revised AC bar)
- [ ] #4 Re-measurement: 30-frame fixed-camera stddev cache-on vs cache-off shows ≥5x drop on ≥ one of {p50, p95, p99}, OR documented justification of revised bar with measurement
- [ ] #5 Onscreen windowed Main.exe run (not offscreen) demonstrates visible noise reduction per feedback_onscreen_testing
- [ ] #6 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
- [ ] #7 Paper-auditor sub-agent re-dispatched against the updated alignment artifact
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
