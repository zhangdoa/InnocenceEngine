---
id: TASK-184
title: 'Editor light + render-toggles panel breakage — investigation and fix'
status: To Do
assignee: []
created_date: '2026-04-28 17:30'
labels:
  - editor
  - bug
  - diagnostic
dependencies: []
priority: high
references:
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
  - Source/Engine/Services/EditorService.cpp
  - Source/Engine/Services/DevToggleRegistry.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"i can't tweak the light properties, the panel doesn't work somehow."*

Editor's light-property panel (and possibly the render-toggles panel) does not work. Symptom unclear without runtime evidence — could be: IPC handler regression, Vue component rendering error, sceneStore/devToggleStore desync, or naive-ui binding issue.

### Why high priority

Without working editor panels, every property tweak (light intensity / color / position, render-pass bypass, debug visualization mode) requires source edit + rebuild. This is the same diagnostic-cost-of-30s-rebuild gap that TASK-183 addresses for visualization, but for *property authoring*.

### Investigation directions

1. **Confirm symptom**: launch editor, open Sponza, click a light entity. Does the LightEditor panel render? Does the field for `intensity` / `color` / `castShadow` show? Does typing in a field do anything? Does the engine view update?
2. **IPC trace**: editor's `EntityProperty` panel issues `GET_ENTITY_DETAILS` and `UPDATE_ENTITY_PROPERTY` per TASK-101 contract. If either fails or returns wrong shape, the panel can silently no-op.
3. **devToggleStore vs sceneStore**: are they both functional, or is one broken? The render-toggles panel uses devToggleStore; the light editor uses sceneStore. If both broken: framework-side. If one: per-feature.
4. **Recent commits to editor**: `a544eb40` (workspace + outliner) was last editor commit; before it, `2eefa0f8` (TASK-149 castShadow + IPC). Both touched IPC paths. Recent regression possible.

### What this delivers

1. Reproduction — exact symptom captured (Playwright spec or screen recording).
2. Root cause identified with cited file:line.
3. Fix landed; both panels operate correctly: light properties round-trip; render-toggles flip GPU state.
4. Regression test added to `Source/Editor-Next/tests/` covering the failure shape.

### Owner

`editor-tooling-expert` (Vue + IPC-Node side). Coordinate with `software-architect` if engine-side IPC handler is at fault.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Symptom reproduced; root cause file:line identified
- [ ] #2 Fix lands; light editor round-trips position/color/intensity/castShadow
- [ ] #3 Render-toggles panel flips GPUPathTracer + RasterizedGI live (no engine restart)
- [ ] #4 Regression Playwright spec added covering the failure shape
- [ ] #5 Live-engine spec passes
- [ ] #6 Peer review per discipline
<!-- AC:END -->

## Implementation Notes

### Review (editor-tooling-expert peer, 2026-04-28)

**Verdict: ADVISORY**

Diff is correct, narrowly scoped, and tests verify the user-visible behaviour live. Two non-blocking observations and one follow-up seed are recorded — none rise to BLOCKED.

#### Diagnosis confirmation

- `Source/Engine/Services/LightSimulationService.cpp:32-33` — every-frame overwrite `if (m_UseColorTemperature) m_RGBColor = ColorTemperatureToRGB(m_ColorTemperature)` confirmed verbatim. Implementer's claim accurate.
- `Source/Engine/Component/LightComponent.h:35` — `m_UseColorTemperature = true` is the field default; GISponza lights inherit this. The K-mode contract is "K is the source of truth, RGB is the cache." Editing RGB without flipping the flag IS a no-op against that contract — implementer's framing of the fix as "explicit color edit IS the user signal to leave K-mode" is consistent with the contract semantics.

AC#1 (root cause file:line) → green. AC#2 (round-trip works for color) → green at the diff layer.

#### Fix-layer evaluation (`coding-principles.md` § fix at the right layer)

Two candidate layers, both defensible:

- **(a) IPC-side implicit side effect** (chosen) — color edit silently flips `UseColorTemperature` off in the handler at `EditorService.cpp:609-619`. Pro: zero new Vue surface, fixes the symptom in the smallest diff, semantically honest (user just said "the color is X", which is meaningful only outside K-mode). Con: irreversible from the editor — see §"Side-effect ripple" below.
- **(b) Editor-side explicit toggle** — surface `useColorTemperature` + `colorTemperature` in `LightEditor.vue` and the GET payload; user picks RGB-vs-K mode; IPC handler does no implicit flip. Pro: contract is explicit at every layer, both directions are reachable. Con: bigger surface; needs GET-side serialization, write handlers for two more properties, Vue widgets, symmetry-spec entries.

(a) is acceptable as the *fix*; (b) is the right shape for the *full feature*. The implementer chose to ship (a) and seed (b) as follow-up — that ordering is correct because the symptom is regression-blocking and (b) is feature scope, not bug scope. **No BLOCKED.** This decision IS a design choice and should ideally have been narrated as a/b/c per `tech-choice-vs-default.md` before commit; flagging as ADVISORY rather than BLOCKED because the chosen layer matches the smallest-diff regression-fix bias `regression-fix-flow.md` codifies.

#### Side-effect ripple — ADVISORY follow-up

`Source/Engine/Services/EditorService.cpp:308-318` (GET_ENTITY_DETAILS LightComponent serializer) does **not** expose `useColorTemperature` or `colorTemperature`. `LightEditor.vue` does **not** render either field. With the new write at `EditorService.cpp:617`, once the user touches color the light is flipped to RGB-mode permanently from the editor's perspective — there is no editor surface to flip K-mode back on. Source-asset reload (`.InnoScene` → `LOAD_SCENE`) is the only path back.

For zhangdoa specifically (per project-CLAUDE.md framing) this is a self-imposed limitation, not a downstream-team blocker. Still: artistic workflow on Sponza presumes K-temperature lighting is reachable. **Recommend: file follow-up backlog task to expose `useColorTemperature` (n-checkbox) + `colorTemperature` (n-input-number, gated on the checkbox) in `LightEditor.vue` and add `useColorTemperature` / `colorTemperature` to the GET payload + writer.** Title suggestion: "Editor: surface LightComponent K-mode toggle and colorTemperature." This unblocks (b) above.

#### Comment discipline (`comment-discipline.md`)

`Source/Engine/Services/EditorService.cpp:611-615` — five-line WHY comment ending with "(TASK-184)". Two observations:

1. The body of the comment ("explicit color edit is a user signal to leave K-mode; otherwise LightSimulationService::Update() overwrites the commit ... and the inspector readback drifts back to the K-derived value") IS load-bearing WHY: it documents a present invariant (the every-frame K-overwrite race) that the next reader of this branch cannot infer from the two-line body alone. Acceptable.
2. The trailing `(TASK-184)` is the banned shape per `comment-discipline.md` ("fixed in TASK-NN" / "added in `<sha>`"). The task-id rots — once TASK-184 archives, this reference is dangling. **Recommend: drop `(TASK-184)`**; the rest of the comment carries its own weight. ADVISORY.

#### Test quality

Four tests, all live-engine, all driving the full UI path:

- `light-editor-roundtrip.spec.js:80-142` (intensity) — DOM input → blur → poll engine readback. Asserts the n-input-number commit path, not the IPC handler in isolation. Correct boundary.
- `light-editor-roundtrip.spec.js:144-179` (castShadow) — n-checkbox click → poll readback. Asserts `!initialCast`, defends against either stuck-true or stuck-false. Correct.
- `light-editor-roundtrip.spec.js:181-244` (color) — bypasses n-color-picker popover, calls `sceneStore.updateProperty` with the same payload `LightEditor.commit('color', ...)` produces. Documented rationale at lines 193-200 (popover is teleported + canvas-rendered, hard to drive from outside; naive-ui has its own suite for picker UX). **Boundary call is correct** for THIS spec — the regression target is the wiring `after` `@update:value`, not the picker chrome. The trade-off: if `LightEditor.vue:82-86` (the `onColor` handler that calls `hexToRgbArray` then `commit`) regresses to a no-op, this spec won't catch it. That's ADVISORY-grade — a separate "color picker DOM-driven" spec would close the gap.
- `light-editor-roundtrip.spec.js:246-272` (RasterizedGI) — switch click → `aria-checked` flip → engine readback via `devToggleStore.refresh()`. Tight.

The intensity test polls 30×100ms = 3 s for the engine readback and asserts `< FLOAT_TOL` (1e-4). For color, the same 3-s budget polls until `valuesMatch` then falls through to a `fallback` GET if budget is exhausted — that fallback path also asserts the value, so a slow engine doesn't false-pass. Correct shape.

AC#4 (regression spec covering the failure shape) → green. AC#5 (live-engine spec passes) → green per implementer ("4/4 tests pass post-fix").

#### Other discipline checks

- `backlog-workflow.md` § no-rogue-implementations — implementer correctly declined to expand scope to a K-mode editor toggle and seeded it as follow-up. Green.
- `target-qualities.md` § fail loudly — `EditorService.cpp` color branch propagates exceptions through `EditorReqError` for `BAD_PROPERTY` / `READ_ONLY`; the new `m_UseColorTemperature = false` write has no failure mode. Green.
- `target-qualities.md` § orthogonality — the implicit flag flip couples two writes in one branch. Documented. Acceptable for the regression fix; the orthogonal shape is option (b) above, deferred.
- Defects-the-implementer-is-least-primed-to-see scan: no log spam (no new logs), no dead branches, no copy-paste, no magic numbers, no silent guards. Clean.

#### Summary

- AC#1 / #2 (color) / #4 / #5 verified at the diff layer with line-grounded evidence.
- AC#2 partial — color is the only field that was actually broken; intensity/castShadow were fine pre-fix and remain fine. Spec pins this so future regressions surface.
- AC#3 (render-toggles flip live) verified by the RasterizedGI spec; no diff was needed, which the implementer's diagnosis predicted.
- AC#6 — this review.

**Two ADVISORY items + one follow-up seed:**

1. Drop `(TASK-184)` from `EditorService.cpp:615` before commit (one-line edit, no re-review needed).
2. (Optional) add a popover-driven color-picker spec when naive-ui ergonomics permit; current bypass is correct but narrow.
3. File follow-up backlog task: surface `useColorTemperature` + `colorTemperature` in `LightEditor.vue` and the GET/UPDATE handlers, so K-mode is reachable from the editor after this fix lands.

Verdict: **ADVISORY** — implementer commits after dropping the `(TASK-184)` tail per item 1; items 2 and 3 ship as backlog seeds.
