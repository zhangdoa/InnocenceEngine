---
id: TASK-218
title: >-
  HIDService thread-safety — m_ButtonEvents shared_mutex / per-key snapshot
  (deeper fix for TASK-217)
status: To Do
assignee: []
created_date: '2026-05-05 11:21'
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
- [ ] #1 m_ButtonEvents access in HIDService.cpp synchronized via shared_mutex (or alternative chosen with documented reasoning) — readers (Update, find()) take shared lock, writers (AddButtonStateCallback, ButtonStateCallback) take unique lock.
- [ ] #2 Pre-fix interactive-mode AV reproducer documented (rate observed at ~33-67% baseline; reproduce with the TASK-217 10-trial harness pattern).
- [ ] #3 Post-fix interactive-mode AV rate measured: 0 AVs across 10+ consecutive interactive launches × 30s idle each.
- [ ] #4 Existing offscreen path continues to work — TASK-213 / TASK-215 / TestGPUPathTracer.ps1 / TestPathTracerThreeScenes.ps1 all continue to PASS.
- [ ] #5 engine-invariants.md updated with the load-bearing threading contract: HID callback registration thread vs Update tick thread, and the synchronization mechanism.
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
