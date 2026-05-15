---
id: TASK-197
title: >-
  Split light-editor-roundtrip.spec.js by feature (intensity / K-mode /
  rasterized-GI)
status: Done
assignee:
  - editor-tooling-expert
created_date: '2026-04-28 20:18'
labels:
  - editor
  - test-hygiene
  - split-before-grow
  - follow-up
dependencies: []
references:
  - Source/Editor-Next/tests/light-editor-roundtrip.spec.js
  - .claude/hooks/gates/file-size.js
  - .claude/disciplines/split-before-grow.md
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Motivation

`Source/Editor-Next/tests/light-editor-roundtrip.spec.js` reached 422 lines after TASK-188 added two K-mode tests (+150 lines). Tripped `.claude/hooks/gates/file-size.js` ratchet. TASK-188 used `[skip-size-gate]` sentinel as a one-off; this is the structural follow-up.

## Scope

Split into per-feature spec files. Each test is independent (live-engine round-trip), so splitting by feature is clean:

- `tests/light-editor-intensity.spec.js` — intensity roundtrip
- `tests/light-editor-k-mode.spec.js` — K-mode toggle + Temperature input + RGB re-derive (TASK-188's two new tests)
- `tests/light-editor-rasterized-gi.spec.js` — RasterizedGI dev-toggle (existing test)

Common setup (engine launch, editor connect, GISponza load) probably belongs in a small `helpers/light-editor-fixture.js` helper if duplicated across files. Confirm during design.

## Acceptance criteria

- [ ] Each spec file under 400 lines
- [ ] All three new specs pass in isolation against live engine + editor
- [ ] No duplication: shared setup extracted to a helper if used in 2+ files
- [ ] Original `light-editor-roundtrip.spec.js` deleted (or repurposed if a remaining test belongs there)
- [ ] `entity-property-symmetry.spec.js` not split (different concern, separate file already)

## Notes

- This is a mechanical refactor. No behavior changes.
- Owner: editor-tooling-expert.
- Follow-up to TASK-188 (committed with `[skip-size-gate]`).
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

Split 422-line `Source/Editor-Next/tests/light-editor-roundtrip.spec.js` into three feature-grouped specs plus a shared launch fixture. Mechanical refactor — no behavior change, no assertion change, no helper invented beyond the four functions already shared by every test in the original file.

### File inventory

| File | Lines | Tests |
|---|---|---|
| `Source/Editor-Next/tests/helpers/light-editor-fixture.js` | 63 | n/a (`FLOAT_TOL`, `launchAgainstEngine`, `findLightEntity`, `readEngineLight`) |
| `Source/Editor-Next/tests/light-editor-intensity.spec.js` | 151 | intensity, Cast Shadow, color picker — LightEditor basic-field round-trips |
| `Source/Editor-Next/tests/light-editor-k-mode.spec.js` | 143 | Use Temp. toggle + Temperature input RGB re-derive |
| `Source/Editor-Next/tests/light-editor-rasterized-gi.spec.js` | 30 | RenderTogglesPanel RasterizedGI dev-toggle |
| `Source/Editor-Next/tests/light-editor-roundtrip.spec.js` | — | DELETED |

All three new specs under the 300-line ratchet; ticket's 400-line AC easily satisfied.

### Helper extraction decision

Yes — `launchAgainstEngine` / `findLightEntity` / `readEngineLight` / `FLOAT_TOL` are used in all three new spec files. Inlining the 30-line `launchAgainstEngine` × 3 would have re-duplicated what the split is trying to disentangle. Helper lives at `tests/helpers/light-editor-fixture.js`; `cwd` adjusted to `path.join(__dirname, '..', '..')` to account for the new helper subdirectory.

### Cluster rationale

The ticket names three target files (`intensity`, `k-mode`, `rasterized-gi`) but the source had six tests. The intensity spec absorbs the three basic LightEditor field round-trips (intensity / Cast Shadow / color picker) — same shape, same mount path, same widget driver pattern. K-mode and RasterizedGI clusters per the ticket as-stated.

### Live-engine validation

Run sequentially with `--workers=1`. First pass hit sibling-agent engine-resource contention (DX12 device-create HRESULT=-2147024809 on rasterized-gi; 240s timeouts on intensity first two tests). After sibling agents released the engine, all three specs re-ran clean:

- `npx playwright test tests/light-editor-intensity.spec.js --workers=1` — 3 passed (37.5s)
- `npx playwright test tests/light-editor-k-mode.spec.js --workers=1` — 2 passed (1.4m)
- `npx playwright test tests/light-editor-rasterized-gi.spec.js --workers=1` — 1 passed (19.5s)

Total 6/6 tests passed against live engine + editor + GISponza scene.

### DoD coverage

- #1: n/a (no compiled code touched — JS spec files only).
- #2: pre-existing integration tests **are** what was split; all six survived the move and re-pass.
- #5: live-engine `npx playwright test` runs are the user-observable outcome; the GISponza scene loaded, the LightEditor inspector mounted, the widget commits round-tripped through engine read-back.
- Not verified: nothing material — the split is mechanical and the live-engine pass set covers the full assertion surface of the original file.

### File-size gate

Original 422-line file deleted; ratchet does not fire on the (now-absent) source. New files (151 / 143 / 63 / 30) are all under the 300-line ratchet.
