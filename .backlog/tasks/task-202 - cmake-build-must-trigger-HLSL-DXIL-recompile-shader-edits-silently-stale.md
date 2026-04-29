---
id: TASK-202
title: cmake build must trigger HLSL→DXIL recompile (shader edits silently stale)
status: To Do
assignee: []
created_date: '2026-04-29 09:55'
updated_date: '2026-04-29 09:55'
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
- [ ] #1 `cmake --build` after editing `lightPass.comp` produces a fresh `lightPass.comp.dxil` without manual intervention
- [ ] #2 Idempotent — running `cmake --build` twice in a row with no edits skips the HLSL compile (no spurious rebuild)
- [ ] #3 No regression in DeployRuntimePayload mirror semantics (TASK-146)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
TASK-183 hit this directly. Researcher spent ~25 minutes diagnosing a shader-side feature that was actually correct, because the DXIL on disk was the previous build's. Symptom triangulated by checking dxil file timestamp vs HLSL source timestamp.

This is a process-discipline regression: TASK-146 fixed the *delete* case, this is the *edit* case. Same root cause family.
<!-- SECTION:NOTES:END -->
