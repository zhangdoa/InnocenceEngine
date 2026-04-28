---
id: TASK-189
title: 'Non-reproducible yellow-green Sponza tint — observation + likely GI convergence'
status: To Do
assignee: []
created_date: '2026-04-28 18:40'
labels:
  - rendering
  - gi
  - bug
  - observation
  - non-reproducible
dependencies:
  - TASK-183
  - TASK-185
priority: low
references:
  - Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp
  - Source/ExampleProject/RenderingClient/GIDenoisePass.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-observed 2026-04-28, then self-resolved without intervention.**

### Observation

- Sponza walls/curtains/pillars rendered with a uniform yellow-green tint.
- Red curtains → yellow-green; green curtains → yellow-green; blue curtains → black-ish.
- User-quoted: *"the same yellow green color like the dragon, red and green curtains all become yellow green, and the blue curtains are black ish, and lots of pillars are the same yellow green."*
- After ~20 minutes of dispatch-loop work (no engine code touching the rendered output), user retested: *"it's normal again. i changed nothing."*

### Diagnosis path attempted

1. Hypothesis: material binding broken → ruled out (rendering-researcher diagnose-only investigation, no source path produces "wall RT indices = Dragon RT indices").
2. Hypothesis: inline RT shadow path silently broken (TASK-176 AC#7 was unverified) → ruled out (rendering-researcher source audit confirmed gate / RayQuery / cbuffer offset / TLAS bind all line-grounded correct).
3. Hypothesis: spectral signature didn't match — point lights are warm-orange `(1.0, 0.54, 0.05)` and `(1.0, 0.76, 0.55)`, not yellow-green. Unshadowed point-light flood would produce warm-orange tint, not yellow-green.

### Most-likely root cause: GI convergence on cold start

`RadianceCacheRaytracingPass` + temporal denoise / SVGF accumulation needs N frames of temporal coherence to converge. On cold engine start (no warm radiance cache, no temporal history), early frames produce wildly off-tinted indirect contribution. The off-tint can persist for seconds-to-minutes depending on camera motion and probe traversal.

Consistent with:
- Spectral signature (yellow-green is plausible un-converged sun-bounce off Sponza's stone).
- Timing (~30 min between "good" and "broken" includes a rebuild + cold restart).
- Self-resolution without intervention (after enough frames, GI converges to correct tint).

### Why this is not actionable yet

Without TASK-183 (runtime visualization modes — direct only / indirect only / per-frame GI buffer view) or TASK-185 (output-texture viewer — see RadianceCache RT directly), there is no way to confirm or refute the hypothesis when the symptom recurs. Source-audit and offline reasoning have already been exhausted.

### What this task captures

The observation, the timing, the ruled-out hypotheses, and the dependency on the diagnostic tooling tasks. **If the symptom recurs**: use TASK-183 / TASK-185 to bisect in real-time. This task remains "open observation" until either (a) symptom recurs and is isolated via diagnostic tooling, OR (b) hypothesis is confirmed by other means (e.g. a sample-reuse / faster-convergence task lands and the symptom no longer happens).

### Related

- **PT sample-reuse / faster-convergence backlog tasks** (user-mentioned but I haven't gathered the IDs) — same root cause family. If GI convergence is the symptom, faster convergence is one fix path.
- **TASK-183** (visualization modes) — diagnostic dependency.
- **TASK-185** (output-texture viewer) — diagnostic dependency.
- **TASK-184** (panel fix, just landed `bfe66510`) — color edit now works, so user can A/B-test point lights as a quick check next time.

### Owner

`rendering-researcher` (when symptom recurs and tools are available).

### Why low priority

Self-resolved; not blocking user workflow today. Recurrence may surface it again — tooling tasks unblock isolation.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 If symptom recurs: confirmed root cause via TASK-183 / TASK-185 visualization
- [ ] #2 OR: sample-reuse / faster-convergence task lands, this task is closed as superseded
- [ ] #3 OR: other path identifies the cause (e.g. a Generated/ cache audit, RenderDoc capture comparison)
<!-- AC:END -->
