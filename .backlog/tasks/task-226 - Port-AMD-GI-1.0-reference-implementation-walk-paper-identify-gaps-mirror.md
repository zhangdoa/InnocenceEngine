---
id: TASK-226
title: 'Port AMD GI 1.0 reference implementation: walk paper, identify gaps, mirror'
status: In Progress
assignee: []
created_date: '2026-05-15'
updated_date: '2026-05-15 20:25'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
  - umbrella
dependencies: []
references:
  - Build/GI1_0.pdf
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Source/Shaders/HLSL/RadianceCacheReprojection.comp
  - Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp
  - Source/Shaders/HLSL/RadianceCacheIntegration.comp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why this exists

The current radiance cache implementation is structurally broken. TASK-6's gap matrix (closed 2026-04-23) catalogued 19 paper-vs-current divergences; subsequent piecemeal subtasks (TASK-114 F-Foundation, TASK-115 S1-Convergence, TASK-116 S2-Robustness, TASK-117 I-Irradiance, TASK-118 W-World — all closed) attempted to land paper-faithful slices. Quality has not converged to the paper.

Direction (2026-05-15): **stop iterating on the in-house port. Mirror the AMD GI 1.0 open-sourced reference implementation directly.**

Rationale:
- The reference impl is open-sourced and authoritative on the paper's algorithms.
- Eight rounds of partial paper porting (TASK-114 through TASK-118 plus the 6.x children) have not produced parity. The accumulated divergence is the symptom; the iterative approach is the cause.
- Mirroring the reference removes the interpretation surface that has produced silent divergences (TASK-127 GI cutoff, TASK-225 SSAO drift, the world-cache hash collision class flagged in TASK-118's notes).

## Deliverables

### Phase 0 — paper + reference walk-through (this dispatch)

1. Read the AMD GI 1.0 paper (`Build/GI1_0.pdf`) end-to-end.
2. Identify and clone the open-sourced reference repository.
3. Walk the reference implementation, mapping each algorithmic stage to its file:line in the reference.
4. Build a **gap matrix** between the reference impl and our current code, granular enough that each row maps to a concrete diff target. This replaces TASK-6's gap matrix.
5. Decide port strategy:
   - Option A: file-by-file mirror (copy reference HLSL + C++ structure verbatim, adapt to our service/component shape).
   - Option B: stage-by-stage rewrite from scratch using the reference as the spec.
   - Option C: keep our current Pass-class wrapping; replace per-pass HLSL bodies with reference shader code.
   Pick during the walk-through based on what the reference's structure actually looks like.

### Phase 1+ — execution

Decomposed during the Phase 0 walk-through. Each phase = one CL, peer-reviewable.

## What this supersedes

Closing as superseded by this umbrella:

- TASK-6.1 — W.3b-api helper header (the world-tile API would mirror the reference's, not ours)
- TASK-6.4 — dispatch probe-grid clamp floor → ceil (a local bandaid on the broken impl)
- TASK-6.8 — noise-floor reduction roadmap (recommendations #2-#6 from a self-audit of the broken impl)

The five letter-tagged decomposition subtasks (TASK-114 F, TASK-115 S1, TASK-116 S2, TASK-117 I, TASK-118 W) are already closed; their landed work stays in `master` but the framing they established is replaced by this umbrella.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Paper + reference walked end-to-end; gap matrix produced
- [x] #2 #2 Port strategy decided and documented
- [x] #3 #3 Phase 1 sub-tasks filed with concrete file-level scope
- [ ] #4 #4 Rendered output of the ported impl visually matches the reference impl on a comparable scene (likely Sponza)
- [ ] #5 #5 60-FPS bar preserved on the autotest camera

## Out of scope

- Engine-side service/component refactoring beyond what the port requires.
- NRD ReBLUR integration (TASK-77.4) is orthogonal — the port consumes the denoiser output, doesn't redesign it.
- Path-tracer primary mode (TASK-77 family) is independent.

## Why high priority

Visual quality is the top user-facing complaint about the GI subsystem. Eight iterations of partial porting have not closed the gap; the next iteration of the same approach has no reason to do better. The reference-mirror approach is the structural lever.
<!-- SECTION:DESCRIPTION:END -->
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
