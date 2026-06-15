---
id: TASK-128
title: Extract reusable ping-pong texture helper to shrink GI pass classes
status: To Do
assignee: []
created_date: '2026-04-25 00:00'
updated_date: '2026-04-30 21:30'
labels:
  - rendering
  - refactor
  - code-quality
dependencies:
  - TASK-6.2
references:
  - Source/Engine/RenderingClient/GIDenoisePass.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Discovered during TASK-125 CL1 implementation. Adding the three new ping-pong texture pairs (`m_PrevWorldPos_{Even,Odd}`, `m_ColorDelta_{Even,Odd}`, plus the pre-existing `m_GIHistory_{Even,Odd}` and `m_Moments_{Even,Odd}`) grew `Source/Engine/RenderingClient/GIDenoisePass.cpp` from 297 → 453 lines, mostly in repetitive resource creation, transition, and binding boilerplate. CL1 used `[skip-size-gate]` to land; the underlying responsibility split is real and the boilerplate is the symptom.

### Pattern to extract

A `PingPongTexture` (or similarly named) RAII type that:
- Owns `Even` / `Odd` `TextureComponent*` pairs.
- Exposes `GetPrev()` / `GetCurr()` accessors based on a frame-parity input (or an internal toggle advanced once per frame by the owner pass).
- Centralises transition + binding for both halves, so callers state intent ("bind prev as SRV, curr as UAV for this dispatch") in one line instead of four.
- Likely lives in foundation or rendering-client common — exact placement to be decided at design time once the call sites are surveyed.

### Beneficiaries

The implementer should grep the codebase for existing hand-rolled ping-pong patterns and list every site in this task's Implementation Notes before refactoring. Known starting points:

- `Source/Engine/RenderingClient/GIDenoisePass.cpp` — surviving pairs after the GI denoiser dust settles. The pair count is in flux while TASK-6.2 lands (TASK-6.2 deletes the `m_Moments_{Even,Odd}` pair); survey at refactor time against the actual current state, not against any historical CL.
- The radiance-cache passes (probe history, screen cache reprojection, world-cache MIPs — at least one of these ping-pongs by hand).
- Any TAA / motion-blur passes that exist in the engine.

The list above is a starting scope, not authoritative. Survey before extracting.

### Schedule constraint: depends on TASK-6.2

TASK-6.2 deletes the `m_Moments_{Even,Odd}` pair and trims `GIDenoisePass`'s descriptor layout from 17 → 15 entries. Doing this extraction before TASK-6.2 lands means templating the wrong set of pairs and immediately re-doing the survey + descriptor accounting once TASK-6.2 trims the layout. Sequence: TASK-6.2 first (focused descriptor cleanup, no behaviour change), then TASK-128 (cross-cutting helper extraction).

If TASK-6.2 stalls and TASK-128 becomes urgent for an unrelated reason (e.g. another pass adds a fifth pair and trips the file-size gate again), the two can be merged into one CL — but the default is sequential, since the helper extraction is naturally larger and benefits from a stable target.

### Why this is low priority

Pure quality-of-life cleanup. Not a defect, not a blocker on any feature, no user-visible symptom. Drives the file-size gate compliance for `GIDenoisePass.cpp` and (more importantly) reduces the failure modes of "forgot to transition the second half of the pair" type bugs that hand-rolled ping-pongs are prone to.

### Cross-cutting: not a child of TASK-125 or any single feature

This is foundation / shared-utility work. No parent task; the dependency on TASK-6.2 is only a scheduling constraint, not a parent-of relationship.
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
## Status note (2026-04-30) — demoted; trigger open

Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

This task was originally scoped against ping-pong sites in the screen-space-GI passes (`GIDenoisePass` + radiance-cache passes) plus speculative TAA / motion-blur pass sites. Under PT-primary, the GI passes are in maintenance mode — they will not gain new ping-pong pairs that motivate extracting the shared helper. Two stable callers (the surviving GI pair + TAA's motion-vector ping-pong) is below the three-caller bar that justifies extracting a shared utility per `tech-choice-vs-default.md` / `coding-principles.md` ("fix at the right layer" — don't extract until you have three concrete uses).

### Earlier TASK-77.1 trigger dropped (2026-04-30 pivot)

Earlier in this batch this task was re-linked to TASK-77.1 (then scoped as a screen-space temporal-reprojection denoiser reusing TAA motion vectors) as the natural third ping-pong site — temporal accumulation of (irradiance estimate, sample count) buffers fits the even/odd ping-pong texture pattern.

User direction (2026-04-30, same session) pivoted TASK-77.1 to a **world-space radiance cache** approach. World-space caches are 3D-world constructs — probe grids, hash grids, voxel cascades, surfel pools — keyed on world position + normal. They do **not** follow the even/odd ping-pong texture pattern this helper would extract (the cache is updated in place, or via append-buffers / hash-table writes, not via two-buffer alternation). The TASK-77.1 trigger no longer holds.

### Trigger is now open

The third concrete ping-pong site that would justify extraction is back to "unknown." Plausible future sources (file when one of these actually lands, not speculatively):

- A TASK-77 sibling axis (spatial à-trous / SVGF, ReSTIR DI temporal reservoirs) lands a screen-space pass with a ping-pong pattern. ReSTIR temporal reservoirs in particular are a natural fit — but only file the link when that task is actually scoped, not pre-emptively (per `feedback_dont_pile_on_backlog_tasks`).
- A future TAA / motion-blur pass extends the existing ping-pong site in a way that motivates the extraction.
- Any unrelated rendering feature adds a fourth ping-pong pair to the existing `GIDenoisePass` boilerplate, tripping the file-size gate again.

**Demoted to `low` priority** (was `low` already in this project's priority enum; the demotion is conceptual — "file when triggered, do not actively pull"). Status stays `To Do` per project precedent. **Don't archive** — the helper is still potentially useful if a third site appears; the previous trigger link is the only thing being dropped here. The TASK-6.2 dependency that originally constrained scheduling is no longer load-bearing under PT-primary.

**Cross-ref**: TASK-77 (PT-primary direction approval), TASK-77.1 (pivoted to world-space radiance cache — no longer a ping-pong-texture-shaped trigger), TASK-6.2 (original sequencing constraint, now stale).
<!-- SECTION:NOTES:END -->
