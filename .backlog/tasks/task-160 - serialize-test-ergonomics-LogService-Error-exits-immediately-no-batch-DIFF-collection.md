---
id: TASK-160
title: 'serialize-test ergonomics: LogService Error exits immediately, no batch DIFF collection'
status: To Do
assignee: []
created_date: '2026-04-27 17:30'
labels:
  - infrastructure
  - testing
  - ergonomics
dependencies: []
references:
  - Source/Engine/Common/LogService.h
  - Source/Engine/Test/SerializeTest.*
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by software-architect during TASK-152 fixture sweep, 2026-04-27.**

`LogService::Print` calls `_Exit(1)` on `LogLevel::Error` when running in test mode (`Source/Engine/Common/LogService.h:25-36`). The serialize-test path uses `Log(Error, ...)` to flag each `[serialize-test] DIFF:` finding — so the test terminates at the **first** DIFF instead of collecting all of them.

### Symptoms (concrete pain)

- Bin/Data drift across runs accumulates because `CompareAndRestore`'s "restore snapshot" branch never executes.
- Batch sweeps (e.g. TASK-152 fixture sweep over 47 files) become tedious: fix the first DIFF, re-run, see the next DIFF, fix that, re-run again. N runs for N drifted files.
- The test reports "FAIL" with one diff visible, masking how much else is misaligned.

### Required fix — options

1. **Collect-then-fail.** Aggregate all DIFFs into a vector during `CompareAndRestore`, log them at Verbose/Warning level (no auto-exit), and emit a single `Log(Error, ...)` summary at the end. Restore the snapshot before the final Error.
2. **Loglevel-distinct DIFF marker.** Demote per-DIFF logs to Warning, keep only the final aggregate as Error. Same outcome as #1 but less restructuring.
3. **Test-mode flag for `_Exit` suppression.** A scoped `LogService::SetCollectMode(true)` that buffers Errors instead of exiting, with a flush at the end. Heavier but reusable for other batch-test scenarios.

Option 2 is the lowest-effort defensible fix. Option 1 is cleaner. Option 3 is over-engineered for one caller.

### Why low priority

Functionally, the test still works — it correctly reports a fixture is drifted. The ergonomics hit is on the developer running batch sweeps, which is rare. A one-shot improvement would help future schema-evolution tasks (TASK-66, TASK-138, TASK-149 all triggered fixture drift).

### Owner

`test-expert` (owns serialize-test) or `low-level-expert` (owns LogService).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 serialize-test runs to completion when multiple fixtures have DIFFs; all DIFFs visible in the log
- [ ] #2 `CompareAndRestore` snapshot-restore branch executes after collecting all DIFFs (Bin/Data not left in drifted state across runs)
- [ ] #3 Test still exits non-zero on any DIFF (correctness preserved)
- [ ] #4 Validate by intentionally drifting two fixtures, running serialize-test, asserting both DIFFs visible + Bin/Data restored
<!-- AC:END -->
