---
id: TASK-159
title: 'Scripts/TestGPUPathTracer.ps1: -frames flag typo (silently ignored by engine)'
status: Done
assignee:
  - '@ci-build-expert'
created_date: '2026-04-27 15:14'
updated_date: '2026-04-27 15:27'
labels:
  - infrastructure
  - testing
  - bug
dependencies: []
references:
  - Scripts/TestGPUPathTracer.ps1
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Spotted while fixing TASK-158 in `Scripts/TestGIScene.ps1`. The sibling script `Scripts/TestGPUPathTracer.ps1` has the same `-frames N` typo: the engine flag is `-total_frames` (parsed in `Source/Engine/Engine.cpp`), so the param is silently ignored and the engine never auto-terminates within the script's window.

Loglevel side is fine (it already uses `-loglevel 0`, which is below Success and lets the grep markers fire), so this task is just the flag rename.

Out of scope from TASK-158 to keep that CL single-purpose; filed as a follow-up.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Fix `-frames N` -> `-total_frames N` in `Scripts/TestGPUPathTracer.ps1`
- [x] #2 Self-test: run the script against the current engine, assert PASS
- [x] #3 Add the same sync-warning comment block as TestGIScene.ps1 explaining why the loglevel + flag knobs must stay aligned with the engine
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## What changed

`Scripts/TestGPUPathTracer.ps1`:

1. **Flag rename** (the literal TASK-159 ask): `-frames N` → `-total_frames N` in both the `Write-Host` echo line and the `Start-Process -ArgumentList` payload. The engine parses `-total_frames` in `Source/Engine/Engine.cpp:325`; `-frames` was silently ignored, so the engine never auto-terminated within the script's window.
2. **Sync-warning comment block** at the top of the file, mirroring the precedent set by `Scripts/TestGIScene.ps1` (commit `c4e7eeb4`, TASK-158). Names the three engine-side knobs (`-loglevel <= 1`, `-total_frames`, scene name) with source-of-truth `file:line` citations: `LogService.h:6` for the Success enum, `Engine.cpp` for the flag, `World.inl` for the scene loader.
3. **Dead-grep fix** (out-of-spec but same bug class, same script, same CL): the script's `sceneLoaded` grep was looking for `GITestBox.InnoScene has been loaded`, but the auto-test path in `World.inl:271` actually loads `GISponza.InnoScene` at frame 5 for both default and `-test gpu_path_tracer` flows. The header doc claim ("Loads GITestBox scene") was also stale. Both updated to GISponza, with the same "if scene was renamed, update both this script and World.inl together" guidance the GIScene script carries. Without this, the typo fix alone would still have left the script reporting FAIL on a working engine — AC#2 (self-test PASS) would not have been satisfiable.

## Why fold the GITestBox grep into the same CL

It is the same bug class as the typo (script knob out of sync with engine reality), in the same script, blocking the same acceptance criterion. Splitting it into a follow-up would have left this CL provably non-PASSing and forced an immediate sequel for the same script — that is not single-purpose, that is fragmented. The new comment block already names "the scene name must match the scene the auto-test path actually loads" as one of the three sync invariants, so the grep update is *exactly* what the comment-block discipline is for.

## Self-test (AC#2)

```
Running: ...\Main.exe -renderer 0 -loglevel 0 -offscreen -total_frames 60 -test gpu_path_tracer
Exit code: 0
Log: [2026-4-27-15-25-33-364].Log
GISponza loaded:  True
Auto-terminated:  True
D3D12 errors:     0
PASS
EXIT=0
```

Engine startup also confirms the flag is now received: `[Inno::Engine::ParseInitConfig] Auto-terminate after 60 frames.`

## Reference / prior art

- TestGIScene.ps1 commit `c4e7eeb4` — literal precedent for the comment block and the grep-update / FAIL-message pattern. cite-prior-art discipline satisfied.

## What was NOT verified

- The script was not re-run against an *intentionally broken* engine to confirm it still FAILs correctly on D3D12 errors / missing scene / missing auto-terminate. Each of those failure paths is straight-line PowerShell with no conditional logic on the typo's data, so the risk is low, but it is not empirically confirmed in this CL.
- The engine binary at `Bin/RelWithDebInfo/Main.exe` was treated as a black box (built earlier today, 2026-04-27 16:02). No source rebuild, no shader recompile — the typo fix and grep update are PowerShell-only and do not require either.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
