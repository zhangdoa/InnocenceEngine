---
id: TASK-120
title: '-gpu_validation incompatible with -total_frames — SetFatalOnError upgrades GBV sentinels to fatal'
status: To Do
assignee: []
created_date: '2026-04-23 18:57'
labels:
  - rendering
  - testing
  - harness
dependencies: []
references:
  - Source/Engine/Services/LogService.cpp
  - Source/Engine/Platform/WinMain/WinMain.cpp
  - Source/Engine/Services/DX12/DX12RenderingServer.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Main.exe -offscreen -total_frames N -gpu_validation` exits with code 1 on an otherwise-clean run because:

1. D3D12 GPU-based validation emits `Incompatible texture barrier layout` with `Layout: UNKNOWN (N)` where N > 30 (`D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON`). This is a documented false positive from Release shaders (TASK-37, CLAUDE.md's "Known imprecision" note) — GBV's Shader Patch Mode NONE can't recover the real layout and emits a sentinel value.
2. The D3D12 debug callback routes D3D12 ERROR messages through the engine's `LogService` at `[Error]` level.
3. `LogService::SetFatalOnError` is enabled whenever `totalFrames > 0` (see TASK-112 commit message: "because totalFrames>0 enables LogService::SetFatalOnError, any Error log kicks the process into exit-1 before Terminate completes").
4. Result: any frame-count-driven integration test that also runs `-gpu_validation` dies on the sentinel, even though no real error has occurred.

Reproduction: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10 -gpu_validation` — exits 1, log contains `Fatal error in test mode, exiting with code 1`. Without `-gpu_validation`, the same command exits 0 with zero `[Error]` lines.

This matters because `-gpu_validation` is the CLAUDE.md-mandated path to chase silent corruption / barrier / resource-state bugs. Today that diagnostic is only usable with `-total_frames 0` (interactive mode) or by scraping the log post-hoc and ignoring the exit code — both brittle.

Fix directions (not yet chosen):
- **Classify D3D12 debug messages before forwarding.** Intercept layout-UNKNOWN-with-N>30 in the D3D12 debug callback, demote to `[Warning]` with a single-shot `[Error]`-level "suppressed N GBV Release-shader sentinels" summary at shutdown. Keeps real barrier errors fatal, silences the known false positive. Cleanest at the callback layer — `SetFatalOnError` continues to mean what it says.
- **Teach `SetFatalOnError` a skip-list.** Allow callers to register message fingerprints that log at `[Error]` level but are not fatal. Broader mechanism, but adds an escape hatch that can drift from its justification.
- **Require shader rebuild with `/Zi /Od` for `-gpu_validation` runs.** Avoids the sentinel entirely but pays a large build-time cost and changes what `-gpu_validation` actually validates.

Option 1 is narrowest and preserves the invariant. TASK-37 is the natural parent for the fix.
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
