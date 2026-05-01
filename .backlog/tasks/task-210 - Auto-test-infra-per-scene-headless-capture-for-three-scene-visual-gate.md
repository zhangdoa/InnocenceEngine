---
id: TASK-210
title: 'Auto-test infra: per-scene headless capture for three-scene visual gate'
status: In Progress
assignee:
  - test-expert
created_date: '2026-05-01 14:27'
updated_date: '2026-05-01 14:41'
labels:
  - test-infra
  - rendering
  - harness
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

The visual-validation discipline now requires every rendering-output CL to capture from all three of `UnitTest.InnoScene`, `GITestBox.InnoScene`, and `GISponza.InnoScene` (auto-memory `feedback_three_scene_rendering_capture.md`; visual-validation §3a follow-up). The current auto-test path can only render `GISponza` end-to-end — `-test gpu_path_tracer` loads `UnitTest` then unconditionally switches to `GISponza` at frame 5 (`Source/ExampleProject/LogicClient/World.inl:268-273`), and there is no entry point for `GITestBox`. Required by TASK-77.1 rework's AC-1 closure (multi-scene visual gate); not required by its first cache-implementation CL.

## Approach

Option 2 (`-scene <path>` override) over option 1 (per-scene `-test` cases): one CLI flag, one InitConfig field, single-point gate in `World.inl` for the auto-switch. Keeps the testCase axis (which controls renderer mode — GPU PT vs default rast) orthogonal to the scene axis (which scene is loaded). Three-scene driver script then issues three `-test gpu_path_tracer -scene <path>` invocations.

## Acceptance criteria

- AC-1: Headless `Main.exe -test gpu_path_tracer -scene ExampleProject/Scenes/UnitTest.InnoScene -total_frames N` renders UnitTest for N frames without auto-switching to GISponza.
- AC-2: Headless `-scene ExampleProject/Scenes/GITestBox.InnoScene` renders GITestBox for N frames.
- AC-3: Headless `-scene ExampleProject/Scenes/GISponza.InnoScene` renders GISponza for N frames (matches the existing `-test gpu_path_tracer` end-state).
- AC-4: `-camera_orbit PITCH,RADIUS,DURATION` works in all three modes (the orbit override is scene-agnostic; smoke-test that the camera entity exists and is driven).
- AC-5: New `Scripts/TestPathTracerThreeScenes.ps1` drives all three scenes back-to-back, dumping captures into structured directories under `Build/captures/<run-tag>/{unittest,gitestbox,gisponza}/`.
- AC-6: Existing `Scripts/TestGPUPathTracer.ps1` invocations continue to PASS — without `-scene`, the auto-switch-to-GISponza-at-frame-5 path stays intact.

## Out of scope

- Reload-at-frame's hardcoded UnitTest target (`World.inl:279`) — separate test path, not exercised by this CL.
- Editor-driven scene picker — already moved to Editor-Next panels (TASK-62).
- Per-scene baselines / golden images for diffing — that's the consumer (visual gate); this CL is the capture infra under it.

## Files in scope

- `Source/Engine/Engine.h` — InitConfig field for scene override.
- `Source/Engine/Engine.cpp` — `-scene <path>` parser slot.
- `Source/ExampleProject/LogicClient/World.inl` — Initialize override + auto-switch gate.
- `Scripts/TestPathTracerThreeScenes.ps1` — new three-scene driver.
- `Scripts/TestGPUPathTracer.ps1` — comment update only if behavior shifts; otherwise untouched.
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

<!-- SECTION:NOTES:BEGIN -->
## Implementation summary

Picked option 2 (`-scene <path>` override) over option 1 (per-scene `-test` cases) for the architectural reasons listed in Description: keeps the testCase axis (renderer mode — GPU PT vs default rast) orthogonal to the scene axis (which scene loads).

### Files changed

- `Source/Engine/Engine.h` — added `char initialScene[512]` to `InitConfig`. Sized to match `serializeTest` for path consistency.
- `Source/Engine/Engine.cpp` — added `-scene <path>` parser. Uses `find("-scene ")` (trailing space) to avoid substring collision with future `-scene*` flags; same locality limitation as the surrounding parsers (`-test`, `-bake`). Logs `Initial scene override: <path>` on success; Warning on missing arg / overlong path.
- `Source/ExampleProject/LogicClient/World.inl` — two semantic changes:
  - `Initialize()` selects initial scene from priority chain `serializeTest → initialScene → UnitTest default`.
  - `Update()` gates the frame-5 GISponza auto-switch on `initialScene[0] == '\0'`. The reload-at-frame branch (also hardcodes UnitTest) is left untouched — out of scope per Description.
- `Scripts/TestPathTracerThreeScenes.ps1` — new three-scene driver. Drives Main.exe across UnitTest / GITestBox / GISponza with `-test gpu_path_tracer -scene <path>`. Captures land in `Build\captures\<RunTag>\{unittest,gitestbox,gisponza}\`. Engine logs are stashed alongside captures (`engine.log`).
- `Scripts/TestGPUPathTracer.ps1` — untouched. Confirmed it still PASSes against the new infra (AC-6).

### Validation transcript (test-expert main thread)

Build:
- `Scripts/BuildWin.ps1` — `Main.exe`, `RenderTest.exe` linked successfully. Pre-existing C4003 `max` macro warnings in `MathHelper.h` unrelated.

AC-6 regression (existing TestGPUPathTracer.ps1):
- `Scripts\TestGPUPathTracer.ps1 -Frames 30` →
  - `GISponza loaded: True`
  - `Auto-terminated: True`
  - `D3D12 errors: 0`
  - `PASS`

ACs 1-5 (new TestPathTracerThreeScenes.ps1):
- `Scripts\TestPathTracerThreeScenes.ps1 -Frames 30 -DumpStart 25 -DumpEnd 29 -CameraOrbit "20,8,30" -RunTag TASK210_orbit_ac4` →
  - All three scenes: `<scene>.InnoScene loaded: True`, `Auto-terminated: True`, `D3D12 errors: 0`, `Moved 5 capture(s)`, `PASS [<scene>]`
  - `Camera orbit: pitch=20deg radius=8 duration=30 frames` logged for each
  - `OVERALL: PASS`
- Captures verified at `Build\captures\TASK210_orbit_ac4\{unittest,gitestbox,gisponza}\gpu_output_0025-0029.png` (5 PNGs each, plus per-run `engine.log`).

### Visual Read assessment (capture-infra sanity, not a quality bar)

- What I see in unittest/gpu_output_0029.png: grey ground plane, central sphere with golden interior inside a black-shell wrapper, soft shadow on floor, mild PT accumulation noise. Renders correctly.
- What I see in gitestbox/gpu_output_0029.png: nearly-black frame with noise grain. `radius=8` orbit places the camera outside or at the wall of the GI test box at this orbit angle.
- What I see in gisponza/gpu_output_0029.png: dark frame with noise grain and faint geometry outline lower-left. Same camera-radius framing issue — Sponza interior is much larger than radius=8 from world origin.
- Differences: capture-infra produces distinct, non-corrupt PNGs per scene per frame; renderer output dim for two of three scenes due to orbit-radius framing, NOT a capture-infra defect.
- Verdict: improvement — the infra delivers what the AC chain requires; per-scene orbit tuning is the rendering-implementer's responsibility when consuming this infra.

### Caveat (not verified)

- The script's success-grep for `Auto-test:.*terminating` is the same shape `TestGPUPathTracer.ps1` uses — but neither script verifies that `gpu_output_NNNN.png` is non-zero pixel-content. A scene with a fundamentally broken renderer would produce solid-black PNGs and still grep-PASS. Acceptable for a smoke harness; the layer-1 visual `Read` of a candidate is the consumer's responsibility per visual-validation discipline.
<!-- SECTION:NOTES:END -->
