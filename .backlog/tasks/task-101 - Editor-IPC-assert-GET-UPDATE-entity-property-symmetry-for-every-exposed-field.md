---
id: TASK-101
title: 'Editor IPC: assert GET/UPDATE entity property symmetry for every exposed field'
status: To Do
assignee: []
created_date: '2026-04-19 18:28'
labels:
  - editor
  - ipc
  - structural
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Context

TASK-93 surfaced that `TransformComponent.rot` was readable via `GET_ENTITY_DETAILS` but silently rejected by `UPDATE_ENTITY_PROPERTY` (it had a `// TODO: Rotation (Quat)` branch and fell through to `BAD_PROPERTY`). The inspector rendered it as a read-only text dump — the asymmetry was invisible to users.

## Structural weakness

`GET_ENTITY_DETAILS` and `UPDATE_ENTITY_PROPERTY` are two independent handlers. A field can appear in the read path and be missing from the write path indefinitely. The inspector has no signal that a field is intentionally read-only vs accidentally stuck.

## Options

1. **Test-level** — a Playwright regression that enumerates every `component.<prop>` surfaced by `GET_ENTITY_DETAILS` for the reference scene and verifies each one round-trips through `UPDATE_ENTITY_PROPERTY` without `BAD_PROPERTY`. Cheap, catches drift, but doesn't prevent the UX dead-end.
2. **Handler-level** — factor the property table into a shared declaration consumed by both handlers: `{ name, read, write }` tuples per component. The read handler iterates the table for serialization; the write handler dispatches on `property` through the same table. A missing `write` entry becomes explicit (intentional read-only) vs accidental.
3. **UI-level** — when the inspector sees a field with no editor, render a clearly disabled control with a "read-only" badge. Complements (1)/(2) but doesn't address the asymmetry itself.

Probably (1) + (2) in sequence — test first to prove current gap, then refactor to the shared table.

## Acceptance

- Every field returned by `GET_ENTITY_DETAILS` for the reference scene has an `UPDATE_ENTITY_PROPERTY` path that commits and reads back (or is explicitly flagged read-only in a single declaration).
- Drift is caught by a Playwright regression, not by a user opening the inspector months later.
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
