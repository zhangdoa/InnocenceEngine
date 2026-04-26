---
id: TASK-149
title: 'Point shadow C: LightComponent castShadow flag + atlas-slot field + serialization'
status: To Do
assignee: []
created_date: '2026-04-26 22:30'
updated_date: '2026-04-26 22:30'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - components
  - serialization
dependencies:
  - TASK-66-design
parent_task_id: TASK-66
priority: high
references:
  - Source/Engine/Component/LightComponent.h
  - Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Data/ExampleProject/Scenes/GISponza.InnoScene
  - Data/ExampleProject/Scenes/GITestBox.InnoScene
  - Data/ExampleProject/Scenes/UnitTest.InnoScene
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask C of TASK-66 (point/sphere light shadows in rasterized pipeline).** Owner: `software-architect`.

Component-side surface of the shadow feature. Independent of atlas internals — depends only on the contract agreed in TASK-147's design phase ("atlas slot index is `uint32_t`" or similar).

### What this subtask delivers

1. **`m_CastShadow : bool` field on `LightComponent`** (`Source/Engine/Component/LightComponent.h`). Default `false` for now (existing scene authors haven't opted in, so opt-in preserves current behavior). Or default `true` for point/sphere light types specifically — `software-architect` decides which default-policy aligns with how scenes are authored today.

2. **`m_AtlasSlotIndex : uint32_t` field on `LightComponent`** (or equivalent — could live in a sidecar GPU-data struct if `software-architect` prefers that layout). This field is **populated by `LightDataService` per frame**, not authored. It exists on the component because `LightDataService` walks LightComponent storage to build the cbuffer; whichever struct holds the slot, the contract is "by the time `LightDataService::Update()` returns, every shadow-casting LightComponent has a valid slot index for this frame, and unshadowed lights have a sentinel".

3. **JSON serialization** (`Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp:27-45,391-406`): add `m_CastShadow` to both `to_json` and `Load` for `LightComponent`. **Do NOT** serialize `m_AtlasSlotIndex` — it is per-frame ephemeral, not authored data. Pattern precedent: `m_ColorTemperature`, `m_LuminousFlux` are authored; transient fields are not serialized in the existing pattern.

4. **Editor inspector** (`Source/Editor-Next/src/components/inspector/LightEditor.vue`): expose `m_CastShadow` as a checkbox. The editor IPC GET/UPDATE symmetry contract (TASK-101) must be satisfied — both directions must round-trip the flag.

5. **Scene-data migration**: existing `.InnoScene` files (`GISponza`, `GITestBox`, `UnitTest`) reference LightComponent JSON. Default-value migration is automatic (missing field → JSON loader applies default), but if `software-architect` chooses default `true` for point/sphere lights, the existing scenes will start casting shadows immediately when TASK-148 lands — this is desirable behavior but worth flagging.

### Project invariants (anchor — read before implementing)

- **Component shape stability**: `LightComponent` is plain old data per the ECS overhaul. Adding a `bool` is fine; adding internal cache state is a smell. If the slot-index field violates the POD invariant, it lives in a separate per-frame GPU-side cache struct in `LightDataService`, NOT on the component.
- **Serialization round-trip**: per `feedback_no_data_integrity_assumptions.md`, the round-trip must be validated. `Source/Editor-Next/tests/entity-property-symmetry.spec.js` is the existing harness for editor-side symmetry — extend it to cover the new field.
- **No magic numbers**: `INVALID_ATLAS_SLOT` sentinel constant lives next to `INVALID_TEXTURE_INDEX` (`GPUDataStructure.h:8`).
- **Editor-tooling-expert** owns `Source/Editor-Next/`; `software-architect` co-dispatches the editor surface or hands off after the C++ landing.

### What this subtask does NOT do

- No render-side consumption of the flag — that is TASK-148's job.
- No atlas allocation logic — that is TASK-147's allocator.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `LightComponent::m_CastShadow` field present with documented default policy
- [ ] #2 JSON `to_json` + `Load` round-trip the new field (verified by serialize-test or new spec)
- [ ] #3 Editor inspector exposes the flag with GET/UPDATE symmetry per TASK-101 contract
- [ ] #4 `LightDataService` populates the per-frame atlas-slot index (sentinel for non-shadow-casting lights)
- [ ] #5 `entity-property-symmetry.spec.js` (or equivalent) extended to cover the new field; pass count quoted
- [ ] #6 Existing scenes (`GISponza`, `GITestBox`, `UnitTest`) load + save without data loss
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Sequencing**: Can run in parallel with TASK-147 once the contract is fixed in TASK-66's design phase ("atlas slot is u32, sentinel = `INVALID_ATLAS_SLOT`"). Land before TASK-148 begins so the caster pass can read the flag.

**Editor co-dispatch**: the C++ field + serialization is `software-architect`. The Vue inspector + IPC roundtrip is `editor-tooling-expert`. Coordinate via the producer.

**Round-trip validation requirement**: per the universal `regression-fix-flow.md` discipline and `feedback_no_data_integrity_assumptions.md`, the serialization round-trip MUST be tested with a real load/save (not a mock). Run the live-engine serialize round-trip per the project's test policy.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (engine + editor)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — editor inspector toggle round-trips through save/load on GISponza; serialize-test green; new editor spec green
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
