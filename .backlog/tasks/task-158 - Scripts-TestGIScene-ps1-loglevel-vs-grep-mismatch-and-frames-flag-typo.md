---
id: TASK-158
title: 'Scripts/TestGIScene.ps1: loglevel-vs-grep mismatch + -frames flag typo'
status: To Do
assignee: []
created_date: '2026-04-27 17:00'
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
- [ ] #1 `-loglevel` and grep markers reconciled (either log level lowered or grep updated to match Warning-level output)
- [ ] #2 `-frames` typo fixed to `-total_frames`
- [ ] #3 Script self-test: run against current engine, assert PASS
- [ ] #4 Documented in script comments why these knobs need to stay in sync
<!-- AC:END -->
