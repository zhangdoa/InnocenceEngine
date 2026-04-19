---
id: TASK-93
title: 'Editor inspector: use Euler angles for rotation editing'
status: To Do
assignee: []
created_date: '2026-04-19 17:09'
labels:
  - editor
  - inspector
  - transform
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Rotation in the inspector should be edited as Euler angles (XYZ in degrees), not raw quaternion components. Quaternions stay internal to the engine; only the wire/UI layer speaks Euler.

## Why

- User-facing: humans think in degrees-around-an-axis.
- Engine-facing: quaternions remain the storage and interpolation format; gimbal lock etc. stays an editor concern only.

## Scope

- **Wire** — `UPDATE_ENTITY_PROPERTY` adds a `rot` branch for `TransformComponent` that accepts a 3-element array `[degX, degY, degZ]`. Engine converts to quaternion before storing into `m_LocalRot`.
- **Reply** — `GET_ENTITY_DETAILS` returns `rot` as `[degX, degY, degZ]` extracted from the stored quaternion via a stable Euler convention (pick one explicitly — XYZ intrinsic is the usual default; document it in the handler).
- **Inspector (`TransformEditor.vue`)** — three numeric inputs per axis, coloured same as position (X=red, Y=green, Z=blue). Values displayed to 2 decimal places. Commits flow through `sceneStore.updateProperty({ component: 'TransformComponent', property: 'rot', value: [x, y, z] })`.
- **Round-trip stability** — editing `rot.x` must produce a committed value whose re-decomposed `rot.x` matches within float tolerance (not the adjacent 360° equivalent) when the other axes are zero. Near gimbal-lock singularities exact round-trip isn't guaranteed — accept that, but the convention must be deterministic so the UI doesn't flip values.

## Acceptance Criteria

- [ ] #1 Engine's `UPDATE_ENTITY_PROPERTY` for `TransformComponent.rot` accepts a 3-element Euler-degree array and stores the equivalent quaternion
- [ ] #2 `GET_ENTITY_DETAILS` returns `rot` as Euler degrees extracted from the stored quaternion using a documented, consistent convention
- [ ] #3 Inspector renders rotation as three X/Y/Z numeric inputs (not a readonly text dump of quat components as it is today)
- [ ] #4 Round-trip: edit each axis independently → reply value decomposes back to the same axis value within 1e-4 degrees when the other axes are zero
- [ ] #5 Playwright regression covers the edit → commit → re-read flow for rotation
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
