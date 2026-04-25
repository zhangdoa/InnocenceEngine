---
id: TASK-132
title: >-
  tests/editor.spec.js fails at HEAD — Main Camera entity assertion broken in
  hierarchy smoke
status: To Do
assignee: []
created_date: '2026-04-25 13:25'
labels:
  - editor
  - tests
  - regression
dependencies: []
references:
  - 6a70375c
  - bf07d562
  - tests/editor.spec.js
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

`tests/editor.spec.js` (live-engine hierarchy smoke) is broken at HEAD on the `ecs-overhaul` branch. The spec performs a live scene load and asserts that a `Main Camera/` entity appears in the editor hierarchy; the assertion fails.

## Pre-existing — not caused by current WIP

`editor-tooling-expert` validated the failure is pre-existing by `git stash`'ing the TASK-92 changes and re-running the spec; the failure persists at the underlying HEAD commit. Confirmed at commit `6a70375c` (TaskDebuggerPanel — collapse-by-default per-thread workload rows).

## Investigation candidates (hypotheses, not conclusions)

The root cause has not been determined. Plausible directions to investigate:

- Engine no longer creates a `Main Camera` entity by default.
- Editor IPC handler returns a different entity name format (`/` suffix dropped or added).
- Live engine connection times out before the scene populates the hierarchy.
- Recent rendering / scene changes (TASK-125 CL1/CL2/CL3, GI passes) altered the default scene serialization path.

## Useful working reference

TASK-101 commit `bf07d562` (`test(editor): assert GET/UPDATE entity-property symmetry`) **passes** today and exercises the editor IPC contract end-to-end. It is the recommended working reference for what the IPC currently returns — diff its expected entity-shape against what the broken hierarchy assertion expects to localize whether the contract drifted or only the hierarchy/scene-load path is affected.

## Constraints

The DoD requires fixing the underlying breakage, **not** modifying the test to make it pass. The test encodes a real product invariant (default scene yields a `Main Camera/` entity visible in the hierarchy via live IPC). If the invariant itself has legitimately changed, that is itself a finding worth surfacing before any spec edit.

## Scope

Cross-cutting test debt; not a feature. No parent task.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause of `tests/editor.spec.js` hierarchy-smoke failure identified and documented in Implementation Notes
- [ ] #2 Underlying breakage fixed in source (engine, editor IPC, or scene-load path) — the test itself is not modified to pass
- [ ] #3 `tests/editor.spec.js` re-runs green at the resulting HEAD on `ecs-overhaul`
- [ ] #4 If the `Main Camera/` invariant has legitimately changed (rather than regressed), that finding is escalated before any spec edit
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
### Root cause (AC #1)

The `Main Camera/` invariant **legitimately changed** in commit `c22b3b64` ("chore(rendering): strip trailing '/' from component names"), which removed the legacy trailing-slash convention from component names project-wide (55 files; rendering passes, services, third-party wrappers, test client). After that commit the engine emits the entity as `Main Camera`, not `Main Camera/`. The hierarchy-smoke spec was never updated, so its literal-text matcher `text=Main Camera/` could no longer find the row. This is **not** a regression in the engine, editor IPC, or scene-load path — it is stale test state.

### Resolution: AC #4 escalation → user-approved Option C

Prior triage established the invariant change was intentional. Per AC #4, the finding was escalated; the user authorized **Option C**: replace the literal-text match with a durable structural locator that asserts hierarchy semantics (an `.entity-item` row contains "Main Camera"), decoupled from naming conventions like the trailing-slash format.

### Edit (AC #2)

`Source/Editor-Next/tests/editor.spec.js` lines 46-49:

- Before: `window.locator('text=Main Camera/')` — brittle full-string match.
- After: `window.locator('.entity-item', { hasText: 'Main Camera' })` — Playwright structural locator (CSS class + substring filter via the documented `hasText` option). Covers the same product invariant: the row exists, can be clicked, and populates `.properties-content`. No coverage was weakened — selection-click and post-stop hierarchy-clear assertions are unchanged.

### Validation (AC #3)

- `npx playwright test editor.spec.js` → `1 passed` (3.8s standalone).
- `npx playwright test entity-property-symmetry.spec.js` → `1 passed` (TASK-101 IPC symmetry spec, regression check requested by user).
- Full-suite `npx playwright test` flakes due to a separate, pre-existing test-isolation issue: Playwright defaults to parallel workers but each spec launches its own Electron + engine binary, contending on engine port / GPU resources / Dockview layout persisted in `%APPDATA%/editor-next/Local Storage`. Clearing Local Storage between runs restored green. This pre-existing harness fragility is **not in TASK-132's scope** — flagging here for a future infra task (configure `playwright.config.js` workers=1, isolated `userData` dir per spec).

### Not verified

- Did not re-run the other 14 specs in the suite. Out of scope for this single-line spec fix; the symmetry spec was the only one the user explicitly named, and it passes.
- Did not add any new tests; the existing `editor.spec.js` covers the smoke path and the structural locator strengthens (not weakens) it.
<!-- SECTION:NOTES:END -->
