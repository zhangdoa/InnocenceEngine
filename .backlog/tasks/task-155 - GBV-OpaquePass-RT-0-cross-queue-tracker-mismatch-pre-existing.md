---
id: TASK-155
title: 'GBV: OpaquePass_RT_0 cross-queue tracker mismatch (pre-existing)'
status: To Do
assignee: []
created_date: '2026-04-27 02:00'
labels:
  - graphics
  - dx12
  - bug
  - validation
dependencies: []
references:
  - Source/Engine/Services/DX12/
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-138 phase 1, 2026-04-27.**

GPU-Based Validation reports a cross-queue tracker mismatch on `OpaquePass_RT_0` resource. Verified pre-existing via stash-bisect: failure reproduces against the pre-TASK-138 baseline as well, so it is not a regression introduced by the RT-shadow scaffold.

### Symptom

`-gpu_validation -total_frames N` reports a cross-queue resource state tracking violation involving `OpaquePass_RT_0`. The resource is touched on multiple queues; D3D12 GBV's tracker is reporting that the state at queue B's first use does not match the state queue A left it in (or that an explicit transition is missing between cross-queue uses).

### Why pre-existing yet not previously caught

Likely the warning exists across the whole pre-TASK-138 history but was overlooked in prior sessions because:
- GBV warnings don't fail the run — engine continues to render.
- Most validation runs were short-frame smokes that may not exercise the cross-queue path enough to flush the warning into log scrape.

### Required fix

Audit `OpaquePass_RT_0` resource lifetime across the queue boundary. Either:
- Insert the missing cross-queue state transition / fence wait at the boundary site.
- Move the consumer to the same queue as the producer if the cross-queue use is incidental.
- Fix the engine-side cross-queue tracker reconciliation (the engine may track resource state independently of GBV; if those drift, only one is wrong).

### Why medium priority

GBV warnings are not engine-fatal, but each one is a real symbol the validator surfaced — `feedback_no_dismissing_tool_noise.md` says don't learn to ignore. As more RT/compute work lands (TASK-138 phase 2 swap, future point-shadow RT extension), cross-queue state-tracking bugs will become harder to diagnose, not easier. Better to fix while the surface is small.

### Owner

`graphics-api-expert` — owns DX12 services subtree and resource state machinery.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause identified — cross-queue producer/consumer pair for `OpaquePass_RT_0` enumerated, missing transition or tracker drift located
- [ ] #2 Fix lands; `-gpu_validation -total_frames 30` clean (zero `D3D12 ERROR` / `D3D12 WARNING` / cross-queue tracker matches)
- [ ] #3 Audit pass on other RT pass resources for the same bug class (anything created via RT pass + consumed in compute / graphics queue)
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
**2026-04-27**: First dispatch attempt (graphics-api-expert) hit quota wall after ~50 min / 181 tool uses with no commits landed. Working tree clean post-attempt — investigation context lost. Re-dispatch recommended after quota refresh; consider tighter scoping (e.g. start with read-only audit pass, then propose the fix in a separate dispatch) to avoid the same wall.
<!-- SECTION:NOTES:END -->
