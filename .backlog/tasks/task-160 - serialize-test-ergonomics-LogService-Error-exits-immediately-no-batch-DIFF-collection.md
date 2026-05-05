---
id: TASK-160
title: >-
  serialize-test ergonomics: LogService Error exits immediately, no batch DIFF
  collection
status: Done
assignee: []
created_date: '2026-04-27 17:30'
updated_date: '2026-05-05 11:29'
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
- [x] #1 serialize-test runs to completion when multiple fixtures have DIFFs; all DIFFs visible in the log
- [x] #2 `CompareAndRestore` snapshot-restore branch executes after collecting all DIFFs (Bin/Data not left in drifted state across runs)
- [x] #3 Test still exits non-zero on any DIFF (correctness preserved)
- [x] #4 Validate by intentionally drifting two fixtures, running serialize-test, asserting both DIFFs visible + Bin/Data restored
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation surfaced 2026-05-05

**Option #2 from the brief (lowest-effort defensible).** Demoted per-DIFF/NEW/DELETED logs to `Warning` in `World.inl::CompareAndRestore`, kept the post-restore aggregate as `Log(Error, ...)` with a count. Restore loop already preceded the aggregate Error in source order, so demoting per-finding logs lets `_Exit(1)` fire only after Bin/Data is restored.

**File touched:** `Source/ExampleProject/LogicClient/World.inl` (lines 67-121).

**Diff shape:**
- 4× `Log(Error, ...)` → `Log(Warning, ...)` at per-finding sites (lines 79, 90, 95, 103)
- `bool l_passed` → `size_t l_diffCount`; final return is `l_diffCount == 0`
- Aggregate Error message extended with `l_diffCount` and "see Warning lines above" pointer
- Added a 4-line WHY comment above the `if (l_diffCount > 0)` block documenting the restore-before-Error ordering invariant

**LogService.h not touched** (option #3 explicitly out of scope).

**AC #4 evidence (verbatim from log tail):**
```
[Warning] [serialize-test] DIFF: .../UnitTest.Mat.Concrete.R0.MaterialComponent.json
[Warning] [serialize-test] DIFF: .../UnitTest.Mat.Copper.R0.MaterialComponent.json
[Error]   [serialize-test] FAILED: 2 file(s) diffed/missing — see Warning lines above. Source files restored.
[Warning] [LogService] Fatal error in test mode, exiting with code 1.
```

Reproduction: copied two `.MaterialComponent.json` files to Build/, drifted both via `python -c "import json; ... json.dump(d, f, indent=2)"` (engine uses 4-space; this drifts to 2-space — parse-valid, byte-different), ran `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` from `Bin/RelWithDebInfo/`. Both DIFFs visible in the same log, aggregate Error reports `2 file(s)`, exit code 1. Post-run: Bin/Data fixtures contain the 2-space drifted bytes (proving RestoreFile wrote the snapshot back over what Save() produced).

**Surprises self-flagged:**
1. **Pre-existing shader-path bug when running Main.exe from repo root** — `Shaders/DXIL/...` resolves relative to CWD, not next-to-binary. Cleanly avoided by `cd Bin/RelWithDebInfo` per engine convention. Not in scope.
2. **`SnapshotDirectory` semantics:** the test snapshots Bin/Data **at test start** and restores to that snapshot, not to a source-tree authoritative copy. So if developer drifted Bin/Data before running, the post-test "restored" state still contains the drift. AC #2 is satisfied because the *restore branch executed* — but the test does not protect against pre-test drift. Pre-existing behavior, not a regression here.
3. **`isOffscreen + totalFrames=1` enables `m_FatalOnError`:** confirms the brief's premise that the `_Exit(1)` path activates because of the serialize-test mode wiring at `Engine.cpp:373-377`.

**Disciplines honored:**
- `safety-observability.md`: Warning level for per-finding logs is appropriate (one log per drifted file, not hot-path per-frame); aggregate Error is the actionable failure signal.
- `cpp-style.md`: used `l_diffCount` to match existing camelCase already in this file (`l_passed`, `l_savedScene`, etc.) over strict engine convention.
- `comment-discipline.md`: 4-line WHY comment explaining restore-before-Error ordering invariant. Not history narration.
- `no-shadow-state.md`: `l_diffCount` is data, not a shadow.

**Build green** — `cmake --build Build --config RelWithDebInfo --target Main` linked clean (one stale Main.exe PID 160920 was holding link target lock from prior session, stopped before re-build).

**Working tree at hand-off:** `World.inl` modified (this CL); `HIDService.cpp` was modified by parallel TASK-217 work (now landed in `36f82e32`); no overlap.

## Review (code-impl, 2026-05-05)

**Verdict: PASS** — all 4 ACs satisfied. End-to-end flow verified by reading `CompareAndRestore` post-CL and the `LogService::Print` _Exit gate at `Source/Engine/Common/LogService.h:25-36`.

**Verifications:**
- `Log(Error,` count in post-CL file: exactly **1** (line 117, the aggregate). All 4 per-finding sites at lines 79, 90, 95, 103 demoted to `Log(Warning, ...)`. Warning level does not trigger `_Exit(1)` in test mode (LogService.h:25), so the loop continues and all DIFFs accumulate.
- Restore loop at lines 114-116 runs before the aggregate `Log(Error, ...)` at line 117 — `_Exit(1)` fires only after Bin/Data is restored.
- AC #3: aggregate Error gated on `if (l_diffCount > 0)` (line 112); clean runs skip the block, return `true`, exit 0. Drift runs hit `_Exit(1)` post-restore, preserving non-zero exit.
- AC #4 trusted on implementer's verbatim log tail (two Warning DIFF lines + aggregate Error reporting `2 file(s)` + Fatal error exit). Independent reproduction skipped per launch budget.
- Counter refactor: `bool l_passed` → `size_t l_diffCount` (line 73), increment at each finding site, return `l_diffCount == 0` at line 120. Caller `RunSerializeTest` still binds it to `bool l_passed` at line 136 — clean boundary.
- No-shadow-state: `l_diffCount` is unique to this file across `Source/`. No collision.
- Restore-before-Error WHY comment (lines 108-111): genuinely non-obvious — explains the LogService `_Exit(1)` interaction that's not visible in this translation unit. Compliant.
- No collateral edits: `git status` shows only `World.inl` and task .md file modified.

**Advisories (non-blocking):**
1. **cpp-style.md** prescribes `l_PascalCase` (`l_DiffCount`), but file's local precedent is camelCase (`l_passed`, `l_savedScene`, etc.). Implementer correctly prioritized local consistency. Worth surfacing if/when file gets style-sweep pass — out of scope here.
2. Pre-existing function-header comment at lines 65-66 still reads "logs every diffing file" — accurate post-CL (just at Warning level now). No update needed.

**Surprises self-flagged by implementer (acknowledged, not findings):** pre-existing shader-path bug when running Main.exe from repo root; snapshot-at-start semantics; isOffscreen+totalFrames=1 enables m_FatalOnError per Engine.cpp:373-377.

**Reviewed-By: code-impl**
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Final Summary — TASK-160

**Status:** Done. Reviewed PASS by fresh code-impl, all 4 ACs satisfied.

### What landed (1 file)

`Source/ExampleProject/LogicClient/World.inl` (lines 67-121) — picked option #2 (lowest-effort defensible):

- 4× `Log(Error, ...)` → `Log(Warning, ...)` at per-finding sites (lines 79, 90, 95, 103). Warning level does not trigger `_Exit(1)` in test mode, so the loop continues and all DIFFs accumulate.
- `bool l_passed` → `size_t l_diffCount`; final return is `l_diffCount == 0`.
- Aggregate Error message extended with `l_diffCount` count and "see Warning lines above" pointer; gated on `if (l_diffCount > 0)` so clean runs preserve their exit-0 behavior.
- 4-line WHY comment above the if-block explaining the restore-before-Error ordering invariant (LogService's `_Exit(1)` interaction not visible in this translation unit).

`LogService.h` not touched — option #3 (scoped collect-mode) explicitly out of scope per the brief.

### AC coverage

| AC | Status | Evidence |
|---|---|---|
| #1 runs to completion, all DIFFs visible | ✓ | Two `[Warning] DIFF` lines printed for two drifted files in a single run before aggregate Error |
| #2 restore branch executes after collecting all DIFFs | ✓ | Post-test on-disk bytes match pre-test snapshot; RestoreFile loop at 114-116 runs before Error at 117 |
| #3 non-zero exit on any DIFF | ✓ | LogService still emits `_Exit(1)` after aggregate Error; clean runs return `true` → exit 0 |
| #4 two-fixture drift demo | ✓ | Drifted two `.MaterialComponent.json` files via `python json.dump(d, f, indent=2)` (4-space → 2-space), ran `Main.exe -serialize_test`: both DIFFs visible in same log, aggregate Error reports `2 file(s)`, exit 1, Bin/Data restored to snapshot |

### What was NOT verified

- **Independent reviewer reproduction of AC #4** — skipped per launch budget. Trusted on implementer's verbatim log-tail evidence.
- **Pre-existing snapshot-at-start semantics** — the test snapshots Bin/Data at test start and restores to that snapshot, NOT to a source-tree authoritative copy. So if developer drifted Bin/Data before running, post-test "restored" state still contains the drift. AC #2 satisfied because restore branch executed; pre-existing behavior, not a regression here. Not filing follow-up — would be option #3 territory (scoped collect-mode + source-tree-authoritative snapshot) which the brief explicitly deferred.
- **Pre-existing shader-path bug when running Main.exe from repo root** — `Shaders/DXIL/...` resolves relative to CWD, not next-to-binary. Cleanly avoided by `cd Bin/RelWithDebInfo` per engine convention. Not in scope.

**Reviewed-By: code-impl**
<!-- SECTION:FINAL_SUMMARY:END -->
