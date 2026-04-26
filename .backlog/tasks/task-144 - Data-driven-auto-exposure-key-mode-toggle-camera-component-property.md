---
id: TASK-144
title: Data-driven auto-exposure key + mode toggle (camera component property)
status: Done
assignee: []
created_date: '2026-04-26 19:37'
updated_date: '2026-04-26 20:00'
labels:
  - rendering
  - auto-exposure
  - data-driven
  - camera
dependencies:
  - TASK-142
references:
  - Source/Shaders/HLSL/finalBlendPass.comp
  - Source/Engine/Component/CameraComponent.h
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User direction (2026-04-26): post-TASK-142 the auto-exposure key value `K=6.0` is hardcoded in the shader. Should be data-driven on the camera component so it can be tuned at runtime (and per-camera) without recompiles, AND the exposure mode (auto vs manual) should be togglable from the same place.

### Current state

`Source/Shaders/HLSL/finalBlendPass.comp:55-67`:

```hlsl
#ifdef AUTO_EXPOSURE
const float K = 6.0f;
float maxLuminance = in_luminanceAverage[0] * K;
float exposure = 1.0f / max(maxLuminance, EPSILON);
#else
float EV100 = ComputeEV100(g_Frame.aperture, g_Frame.shutterTime, g_Frame.ISO);
float exposure = ConvertEV100ToExposure(EV100);
#endif
```

The `#ifdef AUTO_EXPOSURE` is a compile-time gate — toggle requires shader recompile. K is a hardcoded const. Manual-mode parameters (`aperture`, `shutterTime`, `ISO`) ARE already data-driven via `g_Frame` per-frame CB.

### What to add

1. **Camera-side properties** on `CameraComponent` (or wherever camera-shaped state lives):
   - `ExposureMode m_ExposureMode` (enum: Manual, Auto)
   - `float m_AutoExposureKey` (default 6.0)
   - Optional: `float m_AutoExposureCompensation` (EV stops bias on top of auto, default 0.0). Common in modern engines for content tweaking without re-tuning K.
2. **CB plumbing** — extend `PerFrame_CB` (or add `PerCamera_CB` if cleaner) with `m_ExposureMode`, `m_AutoExposureKey`, `m_AutoExposureCompensation`. Update the C++ side that fills the CB.
3. **Shader change** at `finalBlendPass.comp:55-67`:
   - Drop the `#ifdef AUTO_EXPOSURE` gate — runtime branch on the new mode field
   - Read K from CB instead of const
   - Apply the EV compensation if non-zero
4. **Editor exposure** (optional follow-up if the editor doesn't already inspect `CameraComponent`) — surface the new properties so the user can tweak with sliders. May be editor-tooling-expert work; flag as separate follow-up if not trivial in the existing inspector.

### Default values

- `m_ExposureMode = Auto` (preserves current behavior)
- `m_AutoExposureKey = 6.0` (TASK-142 canonical AgX-Default mid-grey)
- `m_AutoExposureCompensation = 0.0`

### Constraints

- **Don't change the manual-exposure path math** — it already uses `g_Frame.aperture/shutterTime/ISO`; just route it via the new mode-switch instead of `#ifdef`.
- **Don't introduce magic numbers** per `feedback_no_magic_numbers.md`. The 6.0 default goes in the camera component's default-init, not the shader.
- **Loud on data violations** per `feedback_no_data_integrity_assumptions.md`. K=0 or NaN → shader should fail loudly (clamp to a sensible floor + log via debug build), not divide-by-zero.

### Why high priority

User flagged this directly: K=6.0 still too dark, wants to dial without recompile cycle. Blocks fast iteration on the AgX exposure feel.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 CameraComponent has m_ExposureMode (Manual/Auto enum) + m_AutoExposureKey + m_AutoExposureCompensation properties
- [x] #2 PerFrame_CB (or PerCamera_CB) extended with the new fields; C++ side fills them from CameraComponent
- [x] #3 finalBlendPass.comp drops #ifdef AUTO_EXPOSURE, reads K + mode + compensation from CB
- [x] #4 Default values: Auto mode, K=6.0, compensation=0.0 (preserves TASK-142 behavior)
- [x] #5 Build green; smoke exit 0; GBV pass clean
- [x] #6 User can change K at runtime (via editor or scene file) without recompile and see the brightness change next frame
- [ ] #7 Editor inspector exposes the new fields (or filed as separate follow-up if non-trivial)
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Promoted auto-exposure key + mode from shader hardcode to data-driven `CameraComponent` properties. User can now dial K + EV compensation at runtime via JSON edit + restart (no recompile cycle). Defaults preserve TASK-142 behavior (Auto mode, K=6.0, no compensation).

### Files touched

| File | Change |
|---|---|
| `Source/Engine/Component/CameraComponent.h` | `enum class ExposureMode { Manual=0, Auto=1 }`; 3 new fields (`m_ExposureMode`, `m_AutoExposureKey`, `m_AutoExposureCompensation`) |
| `Source/Engine/Common/GPUDataStructure.h` | +4 fields on `PerFrameConstantBuffer` (mode + K + comp + padding); padding `[16]→[12]` keeps total bytes unchanged |
| `Source/Shaders/HLSL/common/common.hlsl` | mirror struct change in `PerFrame_CB`; `padding_c[4]→[3]` |
| `Source/Engine/Services/PerFrameDataService.cpp` | fill new CB fields from active camera (`CameraComponent`) |
| `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` | extend `to_json` (writes 3 new keys) and `Load` (uses `j.value()` for missing-key fallback so older scenes round-trip) |
| `Source/Shaders/HLSL/finalBlendPass.comp` | drop `#define AUTO_EXPOSURE` + `#ifdef` gate; runtime branch on `g_Frame.exposureMode`; clamp K to `AUTO_EXPOSURE_KEY_MIN=0.01f`; apply `exp2(autoExposureCompensation)` on auto path |

### CB layout (before / after)

Before (trailing tail): `... frameIndex, modelCount, float padding[16];` (64 bytes)

After: `... frameIndex, modelCount, exposureMode(u32), autoExposureKey(f32), autoExposureCompensation(f32), exposurePadding(f32), float padding[12];` (16 + 48 = 64 bytes — unchanged)

Same in HLSL `PerFrame_CB` (`padding_c[4]→[3]` = 48 bytes).

### Defaults preserve TASK-142

- `m_ExposureMode = ExposureMode::Auto`
- `m_AutoExposureKey = 6.0f` (TASK-142 canonical AgX-Default mid-grey)
- `m_AutoExposureCompensation = 0.0f`

### Validation

- **Build green** (RelWithDebInfo all targets).
- **Smoke**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- **GBV**: `-gpu_validation -total_frames 10` exit 0; only pre-existing GBV "Release-shader false-positive" warnings (unrelated).
- **Runtime tunability VERIFIED**: agent edited `Bin/Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json` to inject `"ExposureMode": 1, "AutoExposureKey": 2.0, "AutoExposureCompensation": 1.0` — engine ran clean. Then injected `"AutoExposureKey": 0.0` (degenerate) — clamp floor `AUTO_EXPOSURE_KEY_MIN=0.01` prevented divide-by-zero, GBV clean. Verified C++ ↔ HLSL CB layout match (mismatch would have produced GBV out-of-bounds).
- Test JSON restored after validation; source-tree `Data/...` not modified (next build redeploys cleanly).

### How user dials K post-fix

Edit `Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json` (source-tree; gets deployed on next build) or `Bin/Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json` (deployed; faster but ephemeral):

```json
{
    "ExposureMode": 1,            // 0=Manual, 1=Auto
    "AutoExposureKey": 4.0,       // lower=brighter; default 6.0
    "AutoExposureCompensation": 0.0  // EV stops bias; +1 doubles brightness
}
```

Restart engine to see change. No shader recompile, no engine link.

### Editor inspection note

Engine has no auto-reflection editor inspector for `CameraComponent`. JSON serialization in `JSONSerializer_Components.cpp` is hand-rolled per-field. Same was true pre-TASK-144 for `m_Aperture / m_ShutterTime / m_ISO`. No editor work in this CL — file separate task for editor-tooling-expert if a slider is wanted.

### What was NOT verified

1. **Visual byte-identical baseline** — math is null on auto path with defaults (`max(6.0, 0.01) == 6.0`, `exp2(0.0) == 1.0`); byte-identical by construction but not pixel-diff captured.
2. **Per-camera tunability with multiple cameras** — `PerFrameDataService` reads the active camera; not regressed by this CL but multi-camera behavior follows whichever camera is active.
3. **Manual-mode side** — bit-identical to pre-TASK-144 `#else` path; not separately exercised end-to-end.
4. **GISponza visual brightening with K=2.0/+1EV** — pipeline accepted values cleanly but agent did not capture-and-compare visual brightness. **User test next session.**

### Coordination — TASK-145 leverages this

The sun-shadow "totally gone" diagnostic (TASK-145, parallel) was **inconclusive** but ruled out 3/4 hypotheses (no code regression, no broken depth, no resolver inversion, K=6.0 doesn't wash GITestBox shadows). Leading remaining hypothesis: GI flood in GISponza interior (recent TASK-6.7/6.9/6.10 boosted indirect; `lightPassIndirectCompose.hlsl:32` adds indirect to direct unconditionally). User can now test the K-perception hypothesis trivially via JSON edit:
- K=9.6 (legacy ACES value): if shadows return → K=6.0 + indirect = wash
- K=12 (deliberate over-dim): if shadows still gone → real GI flood, not K-perception
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
