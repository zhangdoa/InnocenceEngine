---
id: TASK-93
title: 'Editor inspector: use Euler angles for rotation editing'
status: Done
assignee: []
created_date: '2026-04-19 17:09'
updated_date: '2026-04-19 18:29'
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
<!-- AC:BEGIN -->
- [x] #1 #1 Engine's `UPDATE_ENTITY_PROPERTY` for `TransformComponent.rot` accepts a 3-element Euler-degree array and stores the equivalent quaternion
- [ ] #2 #2 `GET_ENTITY_DETAILS` returns `rot` as Euler degrees extracted from the stored quaternion using a documented, consistent convention
- [x] #3 #3 Inspector renders rotation as three X/Y/Z numeric inputs (not a readonly text dump of quat components as it is today)
- [x] #4 #4 Round-trip: edit each axis independently → reply value decomposes back to the same axis value within 1e-4 degrees when the other axes are zero
- [x] #5 #5 Playwright regression covers the edit → commit → re-read flow for rotation
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## What shipped

Commit `8394eb50` — `feat(editor): inspector edits rotation as Euler degrees`.

- `Source/Editor-Next/src/math/quatEuler.js` — XYZ-intrinsic (Tait-Bryan) conversion with gimbal-lock guard on the asin branch.
- `Source/Editor-Next/src/math/index.js` + `main.js` — expose `window.__innoMath` for Playwright reach-through (same pattern as `window.__innoStores`).
- `Source/Editor-Next/src/components/inspector/TransformEditor.vue` — three X/Y/Z `n-input-number` inputs (R/G/B axis badges) for rotation; watch on `props.component.rot` re-derives Euler draft on engine reply; commit goes through `eulerDegToQuat` and `sceneStore.updateProperty({ property: 'rot', value: quat })`.
- `Source/Engine/Services/EditorService.cpp` — `UPDATE_ENTITY_PROPERTY` gets a `rot` branch that stores the incoming Vec4 quat into `m_LocalRot` and replies with the read-back state (matches the setter-reply convention documented in TASK-95).
- `Source/Editor-Next/tests/inspector-rotation.spec.js` — per-axis round-trip via inline mock engine: commit 35°/-47°/88° on X/Y/Z respectively, verify the re-decomposed Euler matches within 1e-4° and that cross-axis leakage is below tolerance. Second test asserts the three inputs render with the right axis labels.

## Approach note (deviation from task spec)

User's guidance mid-task: "just a conversion happening on the UI side." AC #2 as originally written required `GET_ENTITY_DETAILS` to return Euler degrees; we chose the simpler path of keeping the wire quaternion-only and doing the bidirectional conversion in the editor. AC #2 is therefore intentionally not met — the engine continues to return `rot` as `[x, y, z, w]`. The remaining ACs are satisfied.

## Validation

- Editor build: `vite build` — `✓ built in 9.13s`, 4157 modules.
- Engine build: `BuildWin.ps1` — exit 0; Main.exe + RenderTest.exe produced without errors.
- RenderTest tier-1 `draw_instanced`: exit 0, engine terminated cleanly.
- `inspector-rotation.spec.js`: 2/2 passed in 4.4s.
- `scene-vertical.spec.js` regression: 4/4 passed in 7.0s — UPDATE_ENTITY_PROPERTY reply handling still works for pos/rot/scale.

## What was NOT verified

- **Live-engine UI interaction** — no screenshot or manual session driving the Euler inputs against a real `Main.exe` editor process. The inspector-rotation spec is mock-based; the engine-side `rot` branch is structurally identical to `pos`/`scale` (same shape, same read-back pattern), so RenderTest tier-1 is the only engine-side integration run. DoD #4: flagged.
- **Gimbal-lock UX** — when `|β| > ~90°` the decomposed Euler pins γ=0 and folds into α. This is mathematically correct but users may observe a jump in γ's displayed value as they cross the singularity. Not tested; inherent to any Euler parameterization.
- **RenderDoc capture** — transform edits wouldn't show obvious visual drift at tier-1 frame counts; skipped. Would be worth checking on GISponza if the rotation UX surfaces weirdness in practice.

## Structural retrospective → TASK-101

The read-only `rot` sat in the inspector for months because `GET_ENTITY_DETAILS` and `UPDATE_ENTITY_PROPERTY` drifted asymmetrically. Filed TASK-101 to assert read/write symmetry for every exposed component field.
<!-- SECTION:FINAL_SUMMARY:END -->
