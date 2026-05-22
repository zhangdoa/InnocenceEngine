---
id: TASK-23.6
title: 'DoubleBuffer: audit single-producer contract + tighten ordering'
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
updated_date: '2026-05-22 07:32'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 6000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/DoubleBuffer.h is used by exactly **one** real consumer in production code:
- `Source/Engine/Platform/WinWindow/WinWindowService.h:41` — `DoubleBuffer<std::vector<IWindowEvent*>> m_WindowEvents;`

(Note: `Source/Engine/Services/PhysicsSimulationService.cpp:5` `#include`s the header, but does NOT declare any `DoubleBuffer<>` member. It is a stale include — remove it as part of this subtask.)

Given there is one consumer with a fixed shape, the question is whether DoubleBuffer pays its way as a foundation primitive, or whether `WinWindowService::m_WindowEvents` should be a local pattern in WinWindow that doesn't need a shared header.

Header comment claims "Single-producer: no inter-writer locking; back-buffer index is derived from m_FrontIndex." Audit:

1. `Write(Func)` reads `m_FrontIndex` with `memory_order_relaxed` — fine if there's truly one producer and Flip is sequenced after Write. WinWindow has the Win32 message thread as sole producer, so this holds. Document.
2. `Read(Func)` increments `m_ReadersCount` with `acquire`, calls `p_Func(m_Buffers[front])`, decrements with `release`. Between the `fetch_add(acquire)` and reading `m_Buffers[front]`, there's no memory fence pairing with `Flip`'s `store(release)` on `m_FrontIndex`. Flip waits for readers to be 0 via `acquire` load, so the producer side does fence — but a reader that loaded the OLD front index just before the flip increment-then-flip race ordering is subtle. Verify with the C++ memory model.
3. `Flip()` busy-yields via `std::this_thread::yield()`. Document expected reader hold time.
4. No copy/move ctor — fine.

Decision options:
- **A. Keep as foundation primitive, harden + test** — useful if any future Physics-or-similar consumer is on the horizon.
- **B. Inline the pattern into WinWindowService, delete DoubleBuffer.h** — fewer foundation primitives; one consumer doesn't justify a header.

References:
- Source/Engine/Common/DoubleBuffer.h
- Source/Engine/Platform/WinWindow/WinWindowService.h:41 (sole real caller)
- Source/Engine/Services/PhysicsSimulationService.cpp:5 (stale include to remove)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Stale #include in PhysicsSimulationService.cpp removed.
- [ ] #2 Decision A vs B recorded with reason. If B, header deleted and pattern inlined; rest of ACs skip.
- [ ] #3 If A: Memory-ordering audit complete; documented in header comment.
- [ ] #4 If A: WinWindowService usage verified single-producer (Win32 message-loop thread is sole producer of m_WindowEvents).
- [ ] #5 If A: UnitTest exists: producer-flip-consumer pattern, 10^5 iterations on shared atomic counters; no torn reads.
- [ ] #6 If A: StressTest: 4 readers + 1 producer + flips; assertion on read-consistency holds.
- [ ] #7 Optional: TSan / Helgrind clean on the stress test (record verdict).
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
