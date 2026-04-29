---
id: TASK-188
title: >-
  Editor: surface LightComponent K-mode toggle and colorTemperature (TASK-184
  follow-up)
status: Done
assignee:
  - editor-tooling-expert
created_date: '2026-04-28 18:25'
updated_date: '2026-04-28 20:13'
labels:
  - editor
  - lighting
  - feature
dependencies:
  - TASK-184
references:
  - Source/Engine/Services/EditorService.cpp
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Source/Engine/Component/LightComponent.h
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-184 review ADVISORY (editor-tooling-expert peer, 2026-04-28).**

TASK-184's IPC color-handler fix (`EditorService.cpp:609-619`) silently sets `m_UseColorTemperature = false` whenever the user edits the RGB color, so the every-frame `LightSimulationService` K→RGB overwrite stops winning. **But**: there's no editor surface to flip K-mode back on. Once the user touches color, the light is RGB-mode permanently from the editor's perspective; only a source-asset reload (`.InnoScene` re-load) restores K-mode.

GISponza ships every light with `UseColorTemperature: true` and a meaningful K (warm 2500-6500K range). The artistic workflow presumes K-temperature lighting is reachable; today it is one-way-out from the editor.

### What this delivers

1. **GET payload extension** — `EditorService.cpp:308-318` (LightComponent serializer) emits `useColorTemperature : bool` and `colorTemperature : float`.
2. **UPDATE_ENTITY_PROPERTY writers** — handlers for both new fields. `useColorTemperature` flips the flag; `colorTemperature` writes the K value (which `LightSimulationService::Update()` then re-derives RGB from on the next frame).
3. **`LightEditor.vue` widgets** — `n-checkbox` for K-mode + `n-input-number` (gated on the checkbox) for K value. Layout consistent with the existing intensity / cast-shadow rows.
4. **`entity-property-symmetry.spec.js`** writable-light-fields pin extended for the two new fields.
5. **`light-editor-roundtrip.spec.js`** — additional case for the K-mode toggle round-trip.

### Why medium priority

Real artistic-workflow gap (K-mode is the authored default for half the lights), but workaround exists (edit the source `.InnoScene` JSON). Pairs naturally with TASK-184 — same surface, same discipline.

### Owner

`editor-tooling-expert`. Coordinates with `software-architect` if `LightComponent` schema needs adjustment (probably not — fields already exist, only the editor surface is missing).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 GET_ENTITY_DETAILS LightComponent payload includes `useColorTemperature` + `colorTemperature`
- [x] #2 UPDATE_ENTITY_PROPERTY handlers for both new fields land at `EditorService.cpp` (mirror existing `intensity` / `color` shape)
- [x] #3 LightEditor.vue exposes K-mode checkbox + K-temp input (gated on checkbox)
- [x] #4 entity-property-symmetry.spec.js writable-light-fields pin extended
- [x] #5 light-editor-roundtrip.spec.js K-mode round-trip case added; live-engine spec green
- [x] #6 Peer review per discipline
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
### What landed (editor-tooling-expert, 2026-04-28)

**Engine side — `Source/Engine/Services/EditorService.cpp`**:
- GET_ENTITY_DETAILS LightComponent serializer (line ~310): emits `useColorTemperature: bool` and `colorTemperature: float` alongside the existing `color`/`shape`/`lightType`/`intensity`/`castShadow` fields.
- UPDATE_ENTITY_PROPERTY LightComponent branch: two new setters
  - `useColorTemperature` → writes the bool, no side effects. Setting back to `true` is the previously-impossible reverse path; `LightSimulationService::Update()` re-derives `m_RGBColor` from `m_ColorTemperature` on the next frame as the existing K-mode contract.
  - `colorTemperature` → writes the K float, no side effects. Same simulation-service path picks it up next frame.
- Comment-discipline cleanup on the `color` branch: dropped the `(TASK-184)` task-id trailer (deferred ADVISORY from TASK-184 review) and re-anchored the comment to the present invariant — the reverse path is now the explicit `useColorTemperature` setter rather than an implicit "no editor surface".

**Editor side — `Source/Editor-Next/src/components/inspector/LightEditor.vue`**:
- Two new `<n-form-item>` rows: "Use Temp." (`n-checkbox`) and "Temperature" (`n-input-number`, `:disabled="!draft.useColorTemperature"`, K range 1000–12000, step 100).
- `draft` reactive mirrors `useColorTemperature` and `colorTemperature` from the GET payload; `watch` resyncs after engine commits.
- `onColor` mirrors the engine-side side-effect locally — `draft.useColorTemperature = false` — so the K input visually disables in the same frame as the color edit, before the next GET arrives.
- `onUseColorTemperature` and `onColorTemperature` commit through the same `commit()` helper as the existing fields. `label-width` widened 75 → 105 to fit the new "Temperature" label.

**Tests — `Source/Editor-Next/tests/`**:
- `entity-property-symmetry.spec.js`: `writableLightFields` pin extended with `useColorTemperature`, `colorTemperature` (per-field presence assertion).
- `light-editor-roundtrip.spec.js`: two new live-engine cases
  - "K-mode toggle round-trips through the Use Temp. checkbox" — flips the checkbox off then on, asserts engine read-back, asserts the Temperature input is `disabled` while K-mode is off (the editor-side affordance under test).
  - "Temperature input round-trips and re-derives RGB when K-mode is on" — types K=6500 into the input, polls until BOTH the engine `colorTemperature` matches AND the engine `color` has shifted from the GISponza-authored 5780K-derived RGB. The poll for both conditions is load-bearing: the K write is synchronous in the IPC handler, but `LightSimulationService::Update()` re-derives RGB on its own frame, so a poll that exits on K alone races and reads stale RGB.

### Test evidence (RelWithDebInfo Main.exe rebuilt 21:47)

In-isolation (each spec on its own; full-suite run had two flake failures attributable to slow first-cold-cache startup and zombie processes between tests, both passed cleanly when re-run alone):

- `npx playwright test tests/light-editor-roundtrip.spec.js -g "intensity through the n-input-number"` → 1 passed (8.7s).
- `npx playwright test tests/light-editor-roundtrip.spec.js -g "K-mode|Temperature input"` → 2 passed (20.8s).
- `npx playwright test tests/entity-property-symmetry.spec.js` → 1 passed (9.0s). Summary line confirmed both new fields:
  - `OK   LightComponent.useColorTemperature`
  - `OK   LightComponent.colorTemperature`

The K-temperature test in particular validates the full chain: editor commit → IPC handler → engine field write → simulation service K→RGB re-derivation → next-frame GET reflects the new RGB.

### a/b/c (tech-choice-vs-default.md)

For the editor-side surface shape:
- (a) Two separate widgets (checkbox + gated number input) — chosen. Mirrors the engine's two-field `(m_UseColorTemperature, m_ColorTemperature)` storage 1:1; the gating makes the contract ("K is the source of truth only when the bool is true") visible. Naive-ui has the primitives.
- (b) Single composite K-or-RGB switcher widget — rejected. Would hide the bool flag from the editor surface and force editor-side translation logic; "fix at the right layer" says the engine owns the contract, the editor exposes it.
- (c) Project precedent: existing LightEditor rows are flat `<n-form-item>` with one widget each (Color/Luminous/Cast Shadow). (a) matches.

Picked (a) per project precedent + smallest editor-side translation surface.

### Outstanding

AC #6 — peer review per `.claude/disciplines/peer-review-required.md`, dispatched by the parent (editor-tooling-expert peer or `software-architect` cross-domain). The CL is editor-tooling scope — IPC payload extension + Vue widgets + Playwright spec — so a peer in the same role family is the default reviewer.

After review, commit the four files (EditorService.cpp + LightEditor.vue + the two specs) plus this task with `status: Done`. Working tree note for the dispatcher: there is unrelated TASK-183 WIP still in the working tree (`GPUDataStructure.h`, `PerFrameDataService.{cpp,h}`, lightPass HLSL) that I stashed for the build and the stash conflicted on pop; my four files are clean and committable on their own with a path-scoped `git add`.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## K-mode editor surface landed

**Implementation** (5 files):

- `Source/Engine/Services/EditorService.cpp` — GET_ENTITY_DETAILS extended with `useColorTemperature` + `colorTemperature`; UPDATE_ENTITY_PROPERTY gained two new branches (1:1 mirror of existing `intensity`/`castShadow` shape, read-back-not-echo). `color` setter comment cleaned up — anchored to present invariant (no `(TASK-184)` history trailer).
- `Source/Editor-Next/src/components/inspector/LightEditor.vue` — "Use Temp." n-checkbox + "Temperature" n-input-number rows (gated `:disabled="!draft.useColorTemperature"`, 1000–12000K, step 100). `draft` mirrors both new fields with watch resync; `onColor` mirrors engine K-mode side-effect locally so K input visually disables in same frame; label-width widened 75→105.
- `Source/Editor-Next/tests/entity-property-symmetry.spec.js` — `writableLightFields` pin extended.
- `Source/Editor-Next/tests/light-editor-roundtrip.spec.js` — two new live-engine tests (K-mode toggle + Temperature input with poll-on-both-K-and-RGB-shift to validate simulation-service round-trip).

**Validation**:
- Engine: `cmake --build Build --config RelWithDebInfo --target Main` clean (Main.exe 16,668,672 bytes).
- Editor: `npm run build` clean (4157 modules, 10.76s).
- `light-editor-roundtrip.spec.js` (in isolation): intensity ✓, K-mode toggle ✓, Temperature input ✓, RasterizedGI ✓.
- `entity-property-symmetry.spec.js` (in isolation): ✓; new fields green.

**Peer review**: editor-tooling-expert — **PASS+ADVISORY**. All concerns (IPC payload shape symmetry, Vue gating semantics matches engine contract, K-temperature test poll semantics) verified line-grounded. Single non-blocking ADVISORY: temperature range constants (1000/12000/100K) inline in template — matches project convention (intensity row uses same pattern); hoist to named constants only if a third K-aware UI consumer materializes.

**Stash incident note**: during this work, a `git stash` swept up concurrent rendering-researcher TASK-183 edits (worktree-wide stash; cross-agent files included). Recovery handled at parent dispatcher level. Systemic fix filed as TASK-196 (cross-agent stash protection gate).
<!-- SECTION:FINAL_SUMMARY:END -->
