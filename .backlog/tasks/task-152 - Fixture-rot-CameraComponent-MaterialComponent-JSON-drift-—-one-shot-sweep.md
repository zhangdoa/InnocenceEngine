---
id: TASK-152
title: 'Fixture rot: CameraComponent + MaterialComponent JSON drift — one-shot sweep'
status: Done
assignee: []
created_date: '2026-04-27 00:30'
updated_date: '2026-04-27 15:55'
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
- [x] #1 Audit pass over `Data/ExampleProject/Components/` and `Data/ExampleProject/Scenes/` — every JSON serialized component file enumerates every field its current schema serializes
- [x] #2 Drift inventoried: list of files × missing fields recorded in this task's Implementation Notes
- [x] #3 Sweep applied: missing fields added with their schema defaults (or schema-matched authored values where intent is recoverable from git history)
- [x] #4 Live serialize-test green on UnitTest / GISponza / GITestBox after the sweep
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Owner**: software-architect (code-data coupling). Single-CL sweep, 2026-04-27.

## Audit (AC #1)

Schema source-of-truth: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`. Per-type `to_json` field lists (insertion order matters — nlohmann::json preserves it):

| Type | Fields written by `to_json` |
|------|------|
| TransformComponent (1) | ComponentType, Position, Rotation, Scale |
| LightComponent (3) | ComponentType, RGBColor, Shape, LightType, ColorTemperature, LuminousFlux, UseColorTemperature, **CastShadow** (TASK-149) |
| CameraComponent (4) | ComponentType, FOVX, WidthScale, HeightScale, zNear, zFar, Aperture, ShutterTime, ISO, **ExposureMode**, **AutoExposureKey**, **AutoExposureCompensation** (TASK-144) |
| MeshComponent (6) | MeshShape; binary fields (File, VerticesNumber, IndicesNumber) appended by `AssetService::Save` for non-Customized shapes — out of scope here per `SceneService::Save` treating mesh as reference-only |
| MaterialComponent (7) | ComponentType, ShaderModel, Albedo, Metallic, Roughness, AO, Thickness, TextureComponents |

Engine output format (`Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp::Save`): `nlohmann::json` `setw(4)` pretty-print + trailing `endl`, with float leaves trimmed by `TrimFloatPrecision` (`%.6g` round-trip, leaving 0/NaN/Inf alone).

In-struct defaults (used when fixture lacks the field):
- `LightComponent::m_CastShadow = true` (`Source/Engine/Component/LightComponent.h:45`).
- `CameraComponent::m_ExposureMode = ExposureMode::Auto = 1` (`CameraComponent.h:35`).
- `CameraComponent::m_AutoExposureKey = 6.0f` (`CameraComponent.h:36`).
- `CameraComponent::m_AutoExposureCompensation = 0.0f` (`CameraComponent.h:37`).

Loader contract (`Load(...)` in same `JSONSerializer_Components.cpp`): the post-TASK-144/149 fields use `j.value("Field", component.m_Field)` so older fixtures still load. Other fields use `j["Field"]` and *must* be present.

## Drift inventory (AC #2)

47 component fixtures swept. Drift falls into three categories:

### A. Schema field gaps (the load-bearing rot)

| File set | Missing field(s) | Source-of-truth |
|---|---|---|
| All 5 LightComponent fixtures (`{GISponza,GITestBox,UnitTest}.{Sun,PointLight,PointLight2,SphereLight}.LightComponent.json`) | `CastShadow` | TASK-149 |
| Both CameraComponent fixtures (`GISponza.Camera`, `GITestBox.Camera`) | `ExposureMode`, `AutoExposureKey`, `AutoExposureCompensation` | TASK-144 |

For all of these, no historical author value exists in git (these fields were added after the fixtures were authored). Schema defaults are the only defensible values per anchored invariant #1.

### B. Dead embedded `Transform` blocks

`GISponza.Camera.CameraComponent.json`, `GISponza.Sun.LightComponent.json`, `GITestBox.PointLight.LightComponent.json`, `GITestBox.SphereLight.LightComponent.json` carried a legacy embedded `Transform` block. The current schema's `to_json` does not write it and the loader does not read it. The corresponding sibling `*.TransformComponent.json` files exist with identical Position/Rotation/Scale — verified before drop. Safe removal.

### C. Format drift (whitespace + float precision)

Every changed file had at least one of:
- One-line `{...}` collapses (e.g. `"Albedo": { "R": 1.0, ... }`) that the engine's `setw(4)` re-expands to multi-line.
- Float values with full IEEE-754 round-trip precision (e.g. `0.0010000000474974513`) that `TrimFloatPrecision`'s `%.6g` trims (→ `0.001`).
- Field-ordering drift: fixtures that listed (e.g.) `LuminousFlux` before `ColorTemperature`, while `to_json` emits `ColorTemperature` first.
- One file (`GISponza.SphereLight.LightComponent.json` and `GITestBox.SphereLight.LightComponent.json`) lacked a trailing newline — `<< std::endl` in `Save` adds one.

Format drift is not "missing fields" but it still breaks round-trip: a fresh-build deploy + `Save` writes the canonical form, which then fails byte-equality with the on-disk source-form. The serialize-test correctly flags it.

## Sweep (AC #3)

**Tool**: `Build/fixture_sweep.py` (gitignored — `Build/` is in `.gitignore`). Reads each source JSON, parses, builds a canonical dict per the `to_json` field list and ordering, applies the engine's `%.6g` float trim, emits with `json.dumps(indent=4) + "\n"` (byte-identical to nlohmann `setw(4)` + `endl` for our content). MeshComponents pass through unchanged (not on the round-trip surface — `SceneService::Save` treats mesh as reference-only, and AssetService binary-side metadata would be clobbered).

**Files changed**: 47 components under `Data/ExampleProject/Components/`. No scene `.InnoScene` files touched (audit confirmed they contain only references, no inline component blobs — out of scope per task).

**Anchored-invariants check**:
1. *Don't change authored values unless schema-default is the only reasonable substitute.* Held — the only authored-value mutations are `%.6g` precision trims (which the engine itself applies on every `Save`, so they round-trip identically) and dead-Transform-block drops (verified against sibling `*.TransformComponent.json` first).
2. *`j.value(field, default)` calls in the loader define the schema; `to_json`'s field-list is authoritative.* Held — script generates strictly from `to_json`'s field set in its insertion order.
3. *Editor save round-trip must remain symmetric.* Held — verified by serialize-test.
4. *No new tooling.* The sweep script is a one-shot in gitignored `Build/`; not part of the build, not committed, not run automatically.

## Live serialize-test (AC #4)

Ran each scene from a fresh source-state mirror (`rm -rf Bin/Data/ExampleProject/{Components,Scenes/*.InnoScene}` then `cp -r Data/ExampleProject/{Components,Scenes/*.InnoScene}` to simulate post-deploy state):

```
=== UnitTest ===  serialize-test PASSED  (round-trip idempotent)
=== GITestBox === serialize-test PASSED  (round-trip idempotent)
=== GISponza ===  serialize-test PASSED  (round-trip idempotent)
```

Each test re-mirrored from source between runs to ensure the test reflects on-disk source state, not residual canonical state from a prior run.

## Out-of-scope follow-ups (not done in this CL)

- **Test reports only the first DIFF.** `LogService::Print` `_Exit(1)`s on `LogLevel::Error` in test mode, so `CompareAndRestore`'s for-loop terminates on the first divergence and never logs the rest, never restores. Caused most of the agonising over Bin/Data state during this sweep. Not load-bearing for TASK-152 (we made the sweep work anyway), but a real ergonomics gap worth a separate task.
- **Fixture-vs-schema CI gate.** Task explicitly defers it; mentioned only because the audit pass exposes how easy it is for new schema fields to outpace fixture maintenance.

## Files

- `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` (read-only — schema source-of-truth)
- `Source/Engine/Component/CameraComponent.h` (read-only — defaults)
- `Source/Engine/Component/LightComponent.h` (read-only — defaults)
- `Data/ExampleProject/Components/*.json` (47 files modified)
- `Build/fixture_sweep.py` (gitignored, one-shot)
<!-- SECTION:NOTES:END -->
