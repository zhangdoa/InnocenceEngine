---
id: TASK-218
title: >-
  HIDService thread-safety — m_ButtonEvents shared_mutex / per-key snapshot
  (deeper fix for TASK-217)
status: Done
assignee: []
created_date: '2026-05-05 11:21'
updated_date: '2026-05-05 12:53'
labels:
  - bug
  - threading
  - engine
dependencies: []
references:
  - Source/Engine/Services/HIDService.cpp
  - Source/Engine/Services/HIDService.h
  - >-
    .backlog/tasks/task-217 -
    HIDService-Update-intermittent-AV-at-HIDService.cpp-66-0x110.md
  - .claude/state/engine-invariants.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

TASK-217 landed an offscreen-mode-aware `HIDService::Update` guard as the path-of-least-resistance — sidesteps the `m_ButtonEvents` torn-read race for the test/capture surface that was actively blocking 3-scene capture work. **Interactive mode still has the race.** Engine launches without `-offscreen` / `-total_frames` (regular editor sessions, manual smoke testing, dev work) remain susceptible to the same `0xC0000005` AV at `HIDService.cpp:66` (now `:77` post-TASK-217 line shifts) inside `m_ButtonEvents.find()`.

Per TASK-217's peer review (code-impl, 2026-05-05): "the threading contract 'HID callback registration is currently not thread-safe vs Update' is exactly the kind of load-bearing contract that needs to be anchored — to be done at the deeper-fix CL where it becomes load-bearing."

## Failure shape

Same as TASK-217: AV at `HIDService.cpp:77` (post-TASK-217 numbering) inside `unordered_map::find`'s bucket walk. Concurrent mutation of `m_ButtonEvents` from `AddButtonStateCallback` (line 127) and `ButtonStateCallback` (line 151) — writers come from logic-client thread (Player/World callback registrations during scene init / deferred logic update); reader is the engine `Update` tick. No synchronization on `m_ButtonEvents` between these threads.

In offscreen / capture mode: TASK-217's guard short-circuits before the race fires.
In interactive mode: race remains; observed hit rate 33-67% during TASK-213 closure, 50% in TASK-217 baseline.

## Fix path

Two options surveyed in TASK-217's recommended-fix-path discussion:

1. **`std::shared_mutex` on `m_ButtonEvents`** — readers (Update, find()) take shared lock, writers (AddButtonStateCallback, ButtonStateCallback) take unique lock. Standard reader-writer pattern. Cost: lock acquisition per Update tick (typically cheap on uncontended path, but every frame). Pro: minimal restructuring.

2. **Per-key snapshot on Update tick** — at the top of `Update`, copy the relevant `m_ButtonEvents` subset (or the whole map) under a brief lock; iterate the snapshot for the rest of the tick. Pro: lock-held duration is minimised to just the copy. Con: copy cost, plus a different consistency model (a callback registered mid-tick won't fire that tick).

(1) is structurally cleaner and matches the existing `std::mutex`-on-bus-events pattern elsewhere in the engine. (2) is what high-throughput HID systems sometimes do but probably overkill here.

**Recommend (1) `std::shared_mutex`** unless the implementer finds reason otherwise during implementation.

## Scope

- Replace direct `m_ButtonEvents` access in `HIDService.cpp` with shared/unique-mutex-guarded access.
- Audit `AddButtonStateCallback` and `ButtonStateCallback` writers — both must take unique lock.
- Audit `Update`'s `find()` reader — must take shared lock.
- TASK-217's offscreen-mode early-return guard can stay (defensive defense-in-depth, near-zero cost) OR can be removed once the deeper fix is in place. Implementer's call. **Recommend keeping the guard** — it's near-zero cost and protects against any unforeseen regressions in the lock plumbing.
- Update `engine-invariants.md` to anchor the threading contract: "HID callback registration is registered from logic-client thread; HIDService::Update reads from render/main-engine thread; access to `m_ButtonEvents` is synchronized via shared_mutex (reader/writer)."

## Repro / verification (AC #4-shape requirement)

Pre-fix: `Bin/RelWithDebInfo/Main.exe -renderer 0 -loglevel 0` (interactive launch, no `-total_frames`). Currently expected to AV at `HIDService.cpp:77` ~33-67% of launches in the first ~30 seconds. (Specific repro details: launch the editor, let it idle for 5-10s, observe console for `EXCEPTION_ACCESS_VIOLATION ... HIDService::Update`.)

Post-fix: 0 AVs across 10 consecutive interactive launches × 30s of idle each. Document the rate measurement with the same 10-trial harness pattern TASK-217 used.

## Why standalone

TASK-217 closes the offscreen scope-out path. The interactive race is structurally distinct (different threads, different code path) and the fix is non-trivial enough to merit its own CL with proper threading-contract anchoring in `engine-invariants.md`.

## Owner

`code-impl` (low-level / threading family).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 m_ButtonEvents access in HIDService.cpp synchronized via shared_mutex (or alternative chosen with documented reasoning) — readers (Update, find()) take shared lock, writers (AddButtonStateCallback, ButtonStateCallback) take unique lock.
- [ ] #2 Pre-fix interactive-mode AV reproducer documented (rate observed at ~33-67% baseline; reproduce with the TASK-217 10-trial harness pattern).
- [x] #3 Post-fix interactive-mode AV rate measured: 0 AVs across 10+ consecutive interactive launches × 30s idle each.
- [x] #4 Existing offscreen path continues to work — TASK-213 / TASK-215 / TestGPUPathTracer.ps1 / TestPathTracerThreeScenes.ps1 all continue to PASS.
- [x] #5 engine-invariants.md updated with the load-bearing threading contract: HID callback registration thread vs Update tick thread, and the synchronization mechanism.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation surfaced 2026-05-05

**Approach: shared_mutex with snapshot-then-dispatch pattern.** `m_ButtonEvents` reader/writer-synchronised via `std::shared_mutex m_ButtonEventsMutex`. Reader paths (`Update`, `ButtonStateCallback`) snapshot relevant events into local `std::vector<ButtonEvent>` under shared_lock and dispatch `ExecuteEvent` outside the critical section so user callbacks cannot deadlock against the writer side. Writer path (`AddButtonStateCallback`) takes unique_lock. TASK-217's offscreen guard retained as defense-in-depth.

**Files touched:**
- `Source/Engine/Services/HIDService.h` (+5; `mutable std::shared_mutex m_ButtonEventsMutex`. `<shared_mutex>` pulled in transitively via `STL14.h`.)
- `Source/Engine/Services/HIDService.cpp` (net +24, +47/-23. Reader path snapshots-then-dispatches under shared_lock; writer takes unique_lock.)
- `.claude/state/engine-invariants.md` (+12. New "HIDService::m_ButtonEvents access is shared_mutex-synchronised" section.)

**Pushback on the brief:** Brief mis-classified `ButtonStateCallback` as a writer of `m_ButtonEvents`. Reading the code, it only reads (`m_ButtonEvents.find()` at line 151) — it writes `m_PreviousFrameButtonStates[].m_isPressed`, not `m_ButtonEvents`. Treated as reader (shared_lock). Reviewer independently confirmed.

**Re-entrancy / deadlock audit:** Audited every registered callback (Player.inl, World.inl, PhysXWrapper.cpp, GUIService.cpp) — none calls `AddButtonStateCallback`. Snapshot-then-dispatch is defense-in-depth against future re-registration; MSVC's SRW-backed `std::shared_mutex` does NOT allow shared→unique upgrade (Engine.h:121 documents the same constraint).

**`m_MouseMovementEvents` parallel race** has the same shape. Brief scopes work to `m_ButtonEvents`. No AV observed (example clients only register mouse callbacks once during single-shot Setup). Documented in engine-invariants.md as future work.

**AC #3 hit rate:** 10/10 interactive launches × 30s alive each, all logs clean of `EXCEPTION_ACCESS_VIOLATION` / `0xC0000005` / `Fatal error`. Reviewer independently scanned same logs, zero matches.

**Build green** — `Scripts\BuildWin.ps1 -SkipShaderCompile` linked Main.exe + RenderTest.exe.

## Review (code-impl, 2026-05-05)

**Verdict: PASS** with two ADVISORY observations.

**Anchored-invariant checks (all PASS):**
- m_ButtonEvents race eliminated for interactive mode — every read site takes `std::shared_lock`, single write site takes `std::unique_lock`. RAII, `mutable` mutex.
- Snapshot-then-dispatch outside critical section — `Update` (HIDService.cpp:79-101) and `ButtonStateCallback` (:161-186) both collect into local vector under shared_lock, drop the lock, then iterate `ExecuteEvent`.
- TASK-217 offscreen guard retained at HIDService.cpp:53-55, comment refreshed at :49-52.
- AC #3 reviewer-independently confirmed: 10/10 logs clean. Main.exe timestamp 14:32:24 precedes log batch start 14:35.
- engine-invariants.md anchor documents thread split, mechanism, snapshot-then-dispatch rationale, m_MouseMovementEvents scope-out.
- Collateral edits: `git diff --name-only` shows only the three target files plus the explicitly-excluded TASK-199 Scripts/ set.

**Discipline checks (all PASS):** RAII; `mutable` on mutex; no shadow state (mutex IS the primitive); no in-code TASK-IDs in comments; no log changes needed for lock plumbing.

**ADVISORY (both non-blocking):**

1. **Defaulted-move-ops implicitly deleted** (HIDService.h:44 via `INNO_CLASS_CONCRETE_NON_COPYABLE` macro). Adding the non-movable `std::shared_mutex` member causes `= default` move-ctor / move-assign to become implicitly deleted (-Wdefaulted-function-deleted). Structurally harmless: HIDService never moved (only `Get<HIDService>()` / `new`). Same situation already tolerated for every other engine service holding a mutex (e.g. Memory.h:30). Implementer's "no warnings introduced" wording was loose; merge unaffected. Project-wide cleanup would be `INNO_CLASS_CONCRETE_NON_COPYABLE_NON_MOVABLE` macro variant — out of scope.

2. **`m_MouseMovementEvents` parallel race left as TODO.** Same shape as the fixed `m_ButtonEvents` race; not yet observed to AV. Correctly scoped out per brief and documented in engine-invariants.md.

**Reviewed-By: code-impl**
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Final Summary — TASK-218

**Status:** Done. Reviewed PASS by fresh code-impl with two non-blocking ADVISORY observations.

### What landed (3 files)

- `Source/Engine/Services/HIDService.h` (+5) — `mutable std::shared_mutex m_ButtonEventsMutex` member.
- `Source/Engine/Services/HIDService.cpp` (+47/-23 net) — readers (Update, ButtonStateCallback) snapshot under shared_lock then dispatch ExecuteEvent outside critical section; writer (AddButtonStateCallback) takes unique_lock. TASK-217 offscreen guard retained as defense-in-depth.
- `.claude/state/engine-invariants.md` (+12) — new section anchors the threading contract: logic-client-thread writers vs engine-tick readers, std::shared_mutex mechanism, snapshot-then-dispatch rationale, m_MouseMovementEvents parallel race scoped out as future work.

### Why snapshot-then-dispatch

MSVC's SRW-backed `std::shared_mutex` does NOT allow shared→unique upgrade. Naive shared-lock-around-iteration would deadlock if a future callback re-registers mid-`ExecuteEvent`. Snapshot-then-dispatch eliminates the deadlock class entirely: collect under shared_lock, drop lock, iterate. Same pattern documented at `Engine.h:121` for `singletons_mutex_`.

### AC coverage

| AC | Status | Evidence |
|---|---|---|
| #1 shared_mutex plumbing (readers shared, writers unique) | ✓ | Diff |
| #2 Pre-fix interactive AV reproducer | skipped | TASK-217 baseline (50%) carries over; offscreen guard doesn't apply to interactive launches |
| #3 Post-fix AV rate 0/10 interactive | ✓ | 10 launches × 30s clean; reviewer-independent scan confirmed |
| #4 Offscreen path continues to work | ✓ | TestGPUPathTracer.ps1 PASS, Main.exe -total_frames 30 exit 0 |
| #5 engine-invariants.md update | ✓ | New section per TASK-217's reviewer recommendation |

### What was NOT verified

- **Pre-fix AV rate freshly measured** — relied on TASK-217 / TASK-213 baselines (50%, 33-67%).
- **m_MouseMovementEvents race** — scoped out per brief; documented in engine-invariants.md. Not filing follow-up per `don't pile on backlog tasks` (no AV observed).
- **Defaulted-move-deleted warnings** — pre-existing engine-wide pattern (Memory.h:30 etc.); structurally harmless. Right fix is project-wide macro variant cleanup, out of scope.

**Reviewed-By: code-impl**
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
