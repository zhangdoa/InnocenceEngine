---
id: TASK-152
title: 'Fixture rot: CameraComponent + MaterialComponent JSON drift — one-shot sweep'
status: To Do
assignee: []
created_date: '2026-04-27 00:30'
labels:
  - data
  - fixtures
  - chore
dependencies: []
references:
  - Data/ExampleProject/Components/
  - Data/ExampleProject/Scenes/
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by software-architect during TASK-149 (point/sphere shadow component flag) live serialize-test, 2026-04-27.**

The JSON fixture set for `GITestBox` and `GISponza` has drifted away from the current component schemas. Specific gaps observed:

- `Data/ExampleProject/Components/GITestBox.Camera.CameraComponent.json` lacks the fields TASK-144 added to `CameraComponent`: `ExposureMode`, `AutoExposureKey`, `AutoExposureCompensation`. JSON loader's `j.value("Field", default)` masks this at runtime — the fixture loads with default values that may not match what the scene was authored against.
- Various `GISponza` / `GITestBox` `MaterialComponent` files exhibit similar drift (specific fields not enumerated; needs an audit pass).

### Why this matters (and why low priority)

The runtime impact is bounded: `j.value` defaults absorb missing fields, so engine boots. But:

- Default-absorbed values silently drift away from authored intent. A scene authored at one auto-exposure key now silently runs at a different default.
- `feedback_no_data_integrity_assumptions.md` says we validate at every boundary — fixture-vs-schema drift is exactly the kind of soft-failure the discipline is meant to catch.
- Future schema evolution (e.g. TASK-66 point shadows adding `m_CastShadow`) repeats the same pattern. A one-shot sweep aligns the existing fixtures and creates a known-good baseline.

### Out of scope for this task

- Adding migration tooling (e.g. an "audit fixtures vs schema" CLI). That is its own infrastructure effort; this is a manual sweep first.
- Adding a build-time check that fails if fixture is missing required fields. Could be a follow-up if recurrence justifies it.

### Owner

Likely `software-architect` (owns code-data coupling) or `test-expert` (owns fixtures / example project). Coordinate via producer.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Audit pass over `Data/ExampleProject/Components/` and `Data/ExampleProject/Scenes/` — every JSON serialized component file enumerates every field its current schema serializes
- [ ] #2 Drift inventoried: list of files × missing fields recorded in this task's Implementation Notes
- [ ] #3 Sweep applied: missing fields added with their schema defaults (or schema-matched authored values where intent is recoverable from git history)
- [ ] #4 Live serialize-test green on UnitTest / GISponza / GITestBox after the sweep
<!-- AC:END -->
