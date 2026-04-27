---
id: TASK-158
title: 'Scripts/TestGIScene.ps1: loglevel-vs-grep mismatch + -frames flag typo'
status: Done
assignee: []
created_date: '2026-04-27 17:00'
updated_date: '2026-04-27 15:14'
labels:
  - infrastructure
  - testing
  - bug
dependencies: []
references:
  - Scripts/TestGIScene.ps1
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-156 closure, 2026-04-27.**

`Scripts/TestGIScene.ps1` is internally inconsistent and reports FAIL even when the engine runs cleanly:

1. **Loglevel-vs-grep mismatch.** The script invokes `Main.exe` with `-loglevel 2` (Warning level) but its grep success criteria look for Success-level markers (`"GITestBox.InnoScene has been loaded"`, `"Auto-test:.*terminating"`). At loglevel 2, those markers are suppressed by the engine's logger, so grep never matches even on a successful run.

2. **`-frames` vs `-total_frames` flag typo.** The script passes `-frames N` to the engine, but the engine flag is `-total_frames`. Result: the param is silently ignored and the engine never auto-terminates within the script's expected window.

Combined effect: the script either hangs (no auto-terminate) or fails the grep (no Success markers visible). Either way, it reports FAIL on a working engine.

### Required fix

1. Either lower `-loglevel` to `0` (Verbose) or `1` (Success) so the grep markers fire, OR change the grep to look for markers that are emitted at Warning level.
2. Fix `-frames N` → `-total_frames N`.
3. Add a script self-test step (run against a known-good engine, assert PASS) so the next regression of this kind is caught immediately.

### Why low priority

The script is auxiliary developer tooling. Engine + main test suites are unaffected. But it's misleading: any developer running the script today gets a false-FAIL signal even when the engine works.

### Owner

`ci-build-expert` — owns automation scripts.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `-loglevel` and grep markers reconciled (either log level lowered or grep updated to match Warning-level output)
- [x] #2 `-frames` typo fixed to `-total_frames`
- [x] #3 Script self-test: run against current engine, assert PASS
- [x] #4 Documented in script comments why these knobs need to stay in sync
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Root causes (three, not two)**

1. `-loglevel 2` set the filter to `LogLevel::Warning`. The `Print` filter in `Source/Engine/Common/LogService.h:17` is `if (logLevel < GetDefaultLogLevel()) return;`, and the enum order is `Verbose=0, Success=1, Warning=2, Error=3` (`LogService.h:6`), so `Success` messages were dropped. Both grep markers (`Scene ... has been loaded.` from `SceneService.cpp:99` and `Auto-test: ... terminating` from `World.inl:286`) are emitted at `Log(Success, ...)`. Fix: lower to `-loglevel 1` (Success) — keeps logs cleaner than Verbose while letting both markers through.
2. `-frames N` is not a recognized flag. Engine parses `-total_frames` only (`Engine.cpp:325`). Renamed in the script.
3. Bonus bug surfaced by the self-test: the script grepped for `GITestBox.InnoScene has been loaded`, but the auto-test path in `World.inl:271` actually loads `ExampleProject/Scenes/GISponza.InnoScene`. So even after fixing #1 + #2 the script still false-FAILs. Updated grep + log lines + comment header to reference `GISponza`.

**Verification**

Ran `Scripts/TestGIScene.ps1` against the current build: exit 0, `GISponza loaded: True`, `Auto-terminated: True`, `D3D12 errors: 0`, `MAE: 0.367 (threshold: 0.45)`, `PASS`.

**Sync-warning comment**

Added a header block to the script (per AC #4) calling out the three knobs that must stay aligned with the engine: loglevel <= 1, flag is `-total_frames`, scene name must match `World.inl`. Each item names the source-of-truth file so the next regression points at the right place to look.

**Sibling script drift**

`Scripts/TestGPUPathTracer.ps1` has the same `-frames` typo. Loglevel is already correct there (uses `-loglevel 0`, so markers fire). Out of scope for this single-purpose CL — filed as follow-up.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fixed `Scripts/TestGIScene.ps1` to actually pass against a working engine.

- `-loglevel 2` -> `-loglevel 1` so `Log(Success, ...)` markers ("Scene ... has been loaded.", "Auto-test: ... terminating") survive the filter.
- `-frames N` -> `-total_frames N` so the engine actually auto-terminates.
- Bonus fix surfaced by self-test: grep was looking for `GITestBox.InnoScene` but `World.inl` loads `GISponza.InnoScene` in the auto-test path. Updated grep + display strings.
- Header comment now spells out the three engine-side knobs (loglevel filter, total-frames flag, scene name) the script depends on, with file-line citations to the source of truth.

Self-test: script returns PASS (exit 0, MAE 0.37 vs threshold 0.45) on the current `RelWithDebInfo` build.

Sibling drift: `TestGPUPathTracer.ps1` has the same `-frames` typo; filed as a separate low-priority follow-up to keep this CL single-purpose.
<!-- SECTION:FINAL_SUMMARY:END -->
