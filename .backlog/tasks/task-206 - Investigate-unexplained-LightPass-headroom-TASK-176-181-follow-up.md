---
id: TASK-206
title: 'Investigate unexplained ~0.4 ms LightPass headroom (TASK-176/181 follow-up)'
status: To Do
assignee: []
created_date: '2026-04-30'
labels:
  - rendering
  - performance
  - follow-up
dependencies:
  - TASK-181
  - TASK-176
priority: low
references:
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Shaders/HLSL/lightPass.comp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
TASK-181 landed the attenuation-gate fix that the TASK-176 graphics-api-expert ADVISORY predicted would close the 2.42 ms (measured) vs 1-2 ms (predicted) LightPass cost gap on the GISponza autotest camera. The measured A/B refuted that hypothesis:

| Build | N | Mean (ms) | Stddev (ms) |
|-------|---|-----------|-------------|
| A (baseline, pre-gate) | 33 | 1.8024 | 0.0728 |
| B (with gate) | 33 | 1.8054 | 0.0718 |

Delta: **+0.003 ms — sub-noise.** The gate is correct (no perf regression, no correctness issue) but the ADVISORY's "likely root cause" attribution for the 1-2 ms vs 2.42 ms gap is wrong on this scene. The headroom is real but unexplained.

TASK-181's reviewer (rendering-researcher) explicitly flagged this as a follow-up rather than a "fold into closure notes" disposition — the refuted hypothesis deserves its own audit trail.

### Two paths to resolve

1. **Stress-scene re-test.** Build or select a denser-light scene that actually exercises the edge-tile attenuated-out case the gate targets. Re-run A/B against TASK-181's baseline. If the gate produces a measurable delta in the new scene, the ADVISORY's prediction was scene-dependent (under-exercised in GISponza, not wrong) and the GISponza headroom lives elsewhere.

2. **GPU-side attribution.** If even a stress scene shows a sub-noise delta, attribute the residual ~0.4 ms elsewhere — BSDF cost, light-list iteration overhead, warp/wave divergence, IES sampling cost, the shadow trace itself in the worst case, or the cbuffer-load cost of `g_Frame`. Use PIX or RenderDoc serialised pipeline timings to localise.

### Why this is low priority

The 60-FPS bar is met on this branch; the gap is engineering hygiene and a refuted-hypothesis cleanup, not a perf blocker. Filed so the audit trail (predicted → measured → refuted → reattributed) closes cleanly rather than dangling in TASK-181's closure notes.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria

<!-- AC:BEGIN -->
- [ ] #1 LightPass cost on GISponza autotest is either reduced into the 1-2 ms predicted band, OR the residual ~0.4 ms is attributed to a specific GPU-side cost with data backing the attribution
- [ ] #2 If a stress scene is built or selected, `perf-measurement-frame-budget.md` discipline is followed for the A/B (N≥30, RelWithDebInfo, GPU timestamps)
- [ ] #3 The TASK-176 ADVISORY's "likely root cause" attribution is either confirmed (in the stress scene) or formally retracted in this task's closure notes
<!-- AC:END -->

## Owner

`rendering-researcher` (with optional `graphics-api-expert` pairing for the GPU profile path if needed).

## References

- TASK-181 — committed `72f3d92` (closed; A/B refuted ADVISORY prediction)
- TASK-176 — closed; this task does not block it
- `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` — site of the gate
- `.claude/disciplines/perf-measurement-frame-budget.md` — required for any A/B in this task

## Implementation Notes

(Empty — to be filled by the implementer.)

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
(Empty — to be filled at closure.)
<!-- SECTION:FINAL_SUMMARY:END -->
