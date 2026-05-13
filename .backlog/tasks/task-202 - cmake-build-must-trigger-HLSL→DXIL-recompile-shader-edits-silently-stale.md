---
id: TASK-202
title: cmake build must trigger HLSL→DXIL recompile (shader edits silently stale)
status: Done
assignee: []
created_date: '2026-04-29 09:55'
updated_date: '2026-05-13 22:39'
labels:
  - build
  - shaders
  - tooling
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Discovered during TASK-183 validation (2026-04-29).** `cmake --build Build --config RelWithDebInfo --target Main` does not invoke `Scripts/HLSL2DXIL_NoPause.ps1`. After editing an HLSL source, the C++ side rebuilds and the DeployRuntimePayload step copies the *existing* DXIL into `Bin/<Config>/Shaders/DXIL/`, but the HLSL never recompiles. The engine then loads stale DXIL and the shader edit silently has no effect.

This is exactly the trap described in TASK-146's regression chain (mirror-deploy semantics catches stale DXIL when files are *deleted*, but not when files are *edited*).

### Symptom

A render researcher edits `Source/Shaders/HLSL/lightPass.comp`, runs `cmake --build`, sees green build, runs the engine, observes no visual change. They debug everything else (CB layout, binding indices, root signature) before checking whether the shader was actually compiled. ~30 minutes of bisect wasted per occurrence.

### Manual workaround

```
powershell -Command "& Scripts/HLSL2DXIL_NoPause.ps1"
cmake --build Build --config RelWithDebInfo --target Main
```

The DeployRuntimePayload then mirrors the freshly-compiled DXIL into the per-config dir.

### Fix shape

Add a CMake custom target or POST_BUILD step on Main.vcxproj (or its dependency chain) that invokes `Scripts/HLSL2DXIL_NoPause.ps1` whenever any `Source/Shaders/HLSL/**/*.{hlsl,comp,frag,vert}` file is newer than the corresponding DXIL in `Bin/Shaders/DXIL/`. Alternatively make HLSL2DXIL_NoPause.ps1 idempotent (it already is — uses dependency timestamps internally) and unconditionally invoke it before the deploy step.

### Owner

`software-architect` (cmake structure) or `low-level-expert` (build script integration).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `cmake --build` after editing `lightPass.comp` produces a fresh `lightPass.comp.dxil` without manual intervention
- [x] #2 Idempotent — running `cmake --build` twice in a row with no edits skips the HLSL compile (no spurious rebuild)
- [x] #3 No regression in DeployRuntimePayload mirror semantics (TASK-146)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
TASK-183 hit this directly. Researcher spent ~25 minutes diagnosing a shader-side feature that was actually correct, because the DXIL on disk was the previous build's. Symptom triangulated by checking dxil file timestamp vs HLSL source timestamp.

This is a process-discipline regression: TASK-146 fixed the *delete* case, this is the *edit* case. Same root cause family.

2026-05-14 — Picked up autonomously after TASK-222 closure. Lay-of-the-land mapped: `Scripts/HLSL2DXIL_NoPause.ps1` is already idempotent via `Scripts/Lib/Compile-HLSL.psm1` (per-file `LastWriteTimeUtc` staleness + mirror semantics). `Scripts/BuildWin.ps1` invokes it pre-msbuild; `cmake --build` bypasses it (confirmed in `Scripts/CLAUDE.md`). Targets `Main` and `RenderTest` are declared in `Source/Engine/Platform/WinMain/CMakeLists.txt` and call `inno_deploy_runtime_payload()` (POST_BUILD mirror to per-config Bin). Fix-shape candidate: add a top-level `add_custom_target(CompileHlslShaders ALL)` invoking the script, wired as a build dependency of `Main`/`RenderTest`. Mirror semantics in `DeployRuntimePayload.cmake` (POST_BUILD) then propagate fresh DXIL — preserves TASK-146 AC #3.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## TASK-202 Final Summary

### What shipped

New CMake module `CMake/CompileHlslShaders.cmake` defines an `ALL` custom target that invokes `Scripts/HLSL2DXIL_NoPause.ps1` (which is already idempotent via `Scripts/Lib/Compile-HLSL.psm1`'s per-shader `LastWriteTimeUtc` checks). Wired as a build-graph dependency of `Main` and `RenderTest` via `add_dependencies` in `Source/Engine/Platform/WinMain/CMakeLists.txt`, immediately preceding the `inno_deploy_runtime_payload()` POST_BUILD step so the existing TASK-146 mirror semantics propagate fresh DXIL into `Bin/<Config>/Shaders/DXIL/` automatically.

Windows-only (`if(NOT WIN32) return()`). The new target coexists with `Scripts/BuildWin.ps1`'s existing pre-msbuild HLSL step — both paths invoke the same idempotent script.

### AC evidence

- AC #1 (HLSL edit → DXIL recompile): touched `lightPass.comp` mtime, `cmake --build --target Main` advanced `Bin/Shaders/DXIL/lightPass.comp.dxil` mtime (22:34:14). HLSL mtime restored.
- AC #2 (idempotent): second `cmake --build` printed "Skipping unchanged shader" for all 52 shaders; DXIL mtime unchanged; Main.exe link did not fire.
- AC #3 (no TASK-146 mirror regression): per-config `Bin/RelWithDebInfo/Shaders/DXIL/lightPass.comp.dxil` mtime advanced via the existing POST_BUILD wipe-and-recopy step (22:23:51 → 22:34:32).

### Peer review

ADVISORY (no blockers). Two notes:
- VS Solution Explorer folder name changed from `CMakePredefinedTargets` to `Engine/Shaders` per consistency with other custom targets in the tree (applied before commit).
- `USES_TERMINAL` flag kept (reviewer's recommendation): the per-shader "Compiling/Skipping" output is diagnostically load-bearing for staleness visibility — exactly what TASK-202 exists to surface.

### Files changed

- `CMake/CompileHlslShaders.cmake` (new, 50 lines)
- `CMakeLists.txt` (+2: include statement)
- `Source/Engine/Platform/WinMain/CMakeLists.txt` (+8: `add_dependencies` guards on Main + RenderTest)
- `.backlog/tasks/task-202 - ...md` (rename due to title-arrow slugification; AC checks + final summary)

### What was NOT verified

- Full clean reconfigure (`rm -rf Build && cmake -S . -B Build`) — incremental verification proves the dependency wiring; clean rebuild would re-prove it at higher cost. Reviewer reasoned through the configure-order via include-position (`CMakeLists.txt:101`, well before `add_subdirectory(Source/...)` runs `WinMain/CMakeLists.txt`).
- Debug / Release configs — verified RelWithDebInfo only. By reasoning the script writes config-independent output and `DeployRuntimePayload` mirrors per-config, so the chain works identically.
- Non-Windows platforms — gated out via `if(NOT WIN32) return()`. No HLSL2DXIL pipeline exists there.
<!-- SECTION:FINAL_SUMMARY:END -->
