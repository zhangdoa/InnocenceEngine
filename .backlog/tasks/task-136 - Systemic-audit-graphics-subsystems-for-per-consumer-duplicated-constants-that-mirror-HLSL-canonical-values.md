---
id: TASK-136
title: >-
  Systemic: audit graphics subsystems for per-consumer-duplicated constants that
  mirror HLSL canonical values
status: To Do
assignee: []
created_date: '2026-04-25 22:25'
labels:
  - rendering
  - refactor
  - systemic-hygiene
  - constants
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/RadianceCacheConstants.h
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from the TASK-127 retrospective (2026-04-25). The 2/3 GI cutoff bug was caused by a stale `SH_TILE_SIZE = 2` constant duplicated per-pass in `Source/ExampleProject/RenderingClient/RadianceCache*Pass.h` headers, drifting from the HLSL canonical `SH_TILE_SIZE = 3` in `Source/Shaders/HLSL/RayTracingTypes.hlsl`. The C++ side allocated the SH atlas at `probeGrid * 2`; the HLSL wrote/read at `probeIndex * 3`; result was OOB writes (UAV returned zero) for probes past the 2/3 boundary.

The fix consolidated the constants into a single `RadianceCacheConstants.h` namespace shared across all GI passes. Per `feedback_systemic_not_local.md`, this drift class is almost certainly not unique to the GI subsystem.

### What to audit

Search `Source/ExampleProject/RenderingClient/*.h` (and any other graphics-pass headers) for `const uint32_t` / `static constexpr` member constants that mirror values defined canonically in `Source/Shaders/HLSL/` (especially `RayTracingTypes.hlsl`, `BRDF.hlsl`, `common/*.hlsl`).

### Likely candidates to investigate (not exhaustive)

- **Shadow CSM split count** — split count likely defined in the shadow pass class AND in the shadow shaders
- **Light culling tile size** — per-pass tile constant likely duplicated between `lightCulling.comp` and `LightCullingPass.h`
- **BRDF LUT extent** — LUT dimensions likely duplicated between the LUT generator pass class and the consumer shader
- **TAA history extent / jitter sample count** — TAA pass class constants vs `TAAPass.cpp` shader-side
- **Probe / volume dimensions for any other voxel/volume pass** (irradiance volumes, fog, etc.)

### Deliverable

For each subsystem found with the drift pattern: consolidate the constants into a single `<Subsystem>Constants.h` header analogous to `RadianceCacheConstants.h`, with comments anchoring the contract to the HLSL canonical file. Validate that the C++ value matches the HLSL value pre-consolidation (i.e. no live drift) — if any drift IS found, file a task per finding with a captured-symptom screenshot before fixing.

### Why medium priority, not low

Drift bugs from this class are silent until they manifest as visible artifacts (TASK-127 was visible only because the result was a large rectangular cutoff; smaller drifts can hide for years). Consolidating now prevents the next instance.

### Constraint

Behavior-preserving consolidation only. Any value-change uncovered during the audit is a separate task, not folded into this one.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Source/ExampleProject/RenderingClient/*.h grepped for `const uint32_t` / `static constexpr` member constants — inventory in final summary
- [ ] #2 Each found constant cross-referenced against HLSL canonical (RayTracingTypes.hlsl / BRDF.hlsl / common/*.hlsl) — match or drift status quoted per constant
- [ ] #3 For each subsystem with the drift pattern: a `<Subsystem>Constants.h` header consolidates the constants with HLSL-canonical anchor comments
- [ ] #4 Any live drift discovered (i.e. C++ value != current HLSL value) filed as its own task with symptom screenshot, NOT silently fixed in this CL
- [ ] #5 Final summary lists subsystems audited AND subsystems explicitly skipped (with reason)
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
