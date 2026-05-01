---
id: TASK-77.1.1
title: >-
  Hash-grid radiance cache resource + PT primary-hit write path (TASK-77.1 phase
  1 sub-1)
status: To Do
assignee: []
created_date: '2026-04-30 19:43'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies: []
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Author the world-space hash-grid radiance cache resource (HLSL helpers + C++ buffer ownership) and modify the PT raygen to write at primary hit. The convergence-acceleration win (writes/reads at secondary vertices) is deferred to phase 2 per user direction 2026-04-30 — phase 1 is intentionally primary-hit-only.

## Owner

- **HLSL helpers, resource lifetimes, descriptor layout**: `graphics-api-expert`.
- **Algorithmic correctness on the radiance update, prior-art alignment** (Capsaicin GI-1.0 `hash_grid_cache` shape, online running-mean update, sample-count cap): `rendering-researcher`.

Two lanes can run in parallel; coordination point is the `HashGridCache.hlsl` helper signatures.

## Files (anticipated, not prescriptive)

- **New**: `Source/Shaders/HLSL/common/HashGridCache.hlsl` — `Insert`, `Read`, `OnlineMeanUpdate` helpers; open-addressing hash grid keyed by `(quantize(posWS), packOcta(N))`; 20 B per cell + 4 B key.
- **Modified**: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — drop in-shader running-average lerp at line 450-453; emit per-frame noisy radiance; write hash-grid at primary hit.
- **Modified**: `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` — own the hash-grid `RWStructuredBuffer<HashCell>` + key buffer; OR file a separate pass per `split-before-grow.md` if author judges scope warrants.
- **New (constants header)**: per the `RadianceCacheConstants.h` mirror pattern.

## Tech picks (resolved by design call 2026-04-30)

- Capsaicin GI-1.0 `hash_grid_cache` (option c — recent precedent).
- Open-addressing hash grid keyed by `(quantize(posWS), packOcta(N))`, 20 B per cell + 4 B key.
- Adaptive cell size via existing `RadianceCacheCommon.hlsl::AdaptiveCellSize`.
- Online running-mean update with sample-count cap 256.
- Read-at-primary-hit (divergence from Capsaicin, which reads at secondary — phase-1-specific).
- Zero rasterizer-derived inputs.

## Out of scope

- Denoise composition pass (TASK-77.1.2 owns it).
- Secondary-vertex writes / reads (deferred to phase 2 per user direction 2026-04-30 — see TASK-77.1 Implementation Notes "Deferred work").
- Visual A/B (TASK-77.1.3 owns it).
- Colour-delta invalidation (deferred — see TASK-77.1 Implementation Notes "Deferred work").

## Cross-refs

- Parent: TASK-77.1.
- Sibling: TASK-77.1.2 (consumes per-frame noisy buffer + hash-grid resource produced here).
- Sibling: TASK-77.1.3 (validation closure).
- Discipline anchors: `peer-review-required.md`, `tech-choice-vs-default.md`, `feedback_reference_impl_over_paper`, `split-before-grow.md`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 HashGridCache.hlsl exists with Insert, Read, OnlineMeanUpdate helpers; constants mirror via the RadianceCacheConstants.h header pattern
- [ ] #2 Hash-grid buffer authored: 2^20 cells, ~25 MB total (cells + keys); capacity exposed as a tunable in code (configurable, not yet runtime-toggled)
- [ ] #3 PT raygen writes the hash-grid at primary hit; sample-count cap = 256
- [ ] #4 PT raygen produces a per-frame noisy buffer (the in-shader running-average lerp at GPUPathTracerRayGen.hlsl:450-453 is removed)
- [ ] #5 Engine builds RelWithDebInfo clean
- [ ] #6 GBV clean on smoke test (engine launches and runs a few frames without debug-layer ERROR/WARNING)
- [ ] #7 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family (rendering-researcher implementing → graphics-api-expert reviews, or vice versa)
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
