---
id: TASK-133
title: >-
  Build deploy: Bin/<Config>/Shaders + Bin/Data missing — engine cannot run from
  fresh checkout without manual junctions
status: To Do
assignee: []
created_date: '2026-04-25 15:00'
labels:
  - build
  - ci
  - regression
dependencies: []
references:
  - TASK-21
  - ff28c0be
  - Scripts/HLSL2DXIL.ps1
  - Bin/RelWithDebInfo/Main.exe
  - TASK-125
priority: medium-high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

Running `Bin/RelWithDebInfo/Main.exe` from a fresh `git clone` + build fails to load shader DXIL files and data assets. The build produces the executable but does not deploy the runtime payload (compiled shaders + scene/material data) into the per-config output directory.

Specifically, the following directories are not produced by the build:

- `Bin/RelWithDebInfo/Shaders/` (DXIL outputs the engine resolves at runtime).
- `Bin/Data/{Engine,ExampleProject,UnitTest,Components}` (source-tree asset roots).

`Bin/Data/` exists but contains only `Generated/`; the four source-tree subdirs are missing.

## Surfaced by

TASK-21 (`ff28c0be`). The test-expert was unblocked locally with NTFS junctions:

- `mklink /D Bin/RelWithDebInfo/Shaders → Bin/Shaders` (or similar).
- Junctions linking `Bin/Data/{Engine,ExampleProject,UnitTest,Components}` to repo `Data/*` siblings.

These are filesystem-only workarounds — not in source control, not portable, will not work on Linux/macOS, will not work in CI.

## Symptoms (per TASK-21 closure note)

- Engine resolves `Shaders//DXIL//*.dxil` against CWD/Shaders. Works only if `Bin/RelWithDebInfo/Shaders/` exists or CWD is set to `Bin/`.
- Engine resolves the data dir as `CWD/../Data` which lands at `Bin/Data` when running from `Bin/RelWithDebInfo` — but `Bin/Data/` only contains `Generated/`, missing the source-tree `Engine|ExampleProject|UnitTest|Components` subdirs.

## Likely root causes (candidates, not conclusions)

- CMake post-build copy step missing or broken for the RelWithDebInfo configuration. Other configs may still work — needs verification per-config.
- Shader-compile script (`Scripts/HLSL2DXIL.ps1`) outputs to `Bin/Shaders/` but no symlink/copy step into `Bin/<Configuration>/`.
- Working-directory assumption changed in a recent CL; may correlate with TASK-125 GI-pass additions.

## Reproduction

```
git clone <fresh>
cmake --build Build --config RelWithDebInfo --target Main
Bin/RelWithDebInfo/Main.exe -mode 0
```

Should fail to load shaders.

## Scope

Build-system / deploy fix. Not a feature. No parent task.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Build produces a runnable `Bin/RelWithDebInfo/` directory without manual junctions — fresh clone + build + run reaches a rendered frame
- [ ] #2 The same applies to other build configurations actually used (Debug, Release) — verified per-config, not assumed
- [ ] #3 Works on Windows (NTFS) end-to-end; the design path for Linux/macOS is documented in Implementation Notes (cross-platform implementation may follow in a separate task, but the chosen mechanism — copy vs. symlink vs. CMake-managed deploy — must not be Windows-only by construction)
- [ ] #4 TASK-21 captures (`Build/captures/TASK21_*.png`) are reproducible from a clean checkout without manual filesystem setup
- [ ] #5 Root cause identified and documented — which step (CMake post-build, shader script, install rule, or CWD assumption) was responsible, and why it regressed
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
