---
id: TASK-217
title: >-
  HIDService::Update intermittent AV at HIDService.cpp:66 (+0x110) on first-tick
  path
status: To Do
assignee: []
created_date: '2026-05-05 08:00'
labels:
  - bug
  - intermittent
dependencies: []
references:
  - 'Source/Engine/Services/HIDService.cpp:66'
  - Scripts/TestGPUPathTracer.ps1
  - >-
    .backlog/tasks/task-215 -
    ImGuiWrapper-Load-scene-button-uses-sync-Load-engine-invariants.md-violation.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Failure shape

`Inno::HIDService::Update` raises `0xC0000005` (EXCEPTION_ACCESS_VIOLATION) at `Source/Engine/Services/HIDService.cpp:66`, offset `+0x110`. Reading from memory; fault address has been observed at both `0x000000030000001A` (main session, 2026-05-05) and `0xFFFFFFFFFFFFFFFF` (TASK-215 implementer, 2026-05-04) — different concrete addresses but same instruction and offset, suggesting a torn read or stale pointer dereference rather than a fixed-bad-write.

Fatal — engine exits with code 1 in test mode before the auto-test path can complete its frame budget.

## Reproduction

Intermittent — does not fire on every run. Observed rates:

- Main session (2026-05-05): 1 fail / 2 attempts (`Bin/RelWithDebInfo/Main.exe -total_frames 30`).
- TASK-215 implementer (2026-05-04): 1 fail / 4 attempts (`Scripts/TestGPUPathTracer.ps1 -Frames 30`).

Reproduces on **clean master** (`git stash`-confirmed during TASK-215 implementer work and main-session 2026-05-05). NOT caused by TASK-215 or TASK-214.

## Hypothesis (not yet verified)

Use-after-free or torn map iteration on `m_ButtonEvents` — possibly racing `HIDService::Setup` callback registration against the first `Update()` tick. Worth bisecting against recent threading-contract changes (the engine-invariants doc note about render-thread vs HID callback timing implies HIDService is in scope of recent threading work).

## Why standalone

Surfaced during TASK-215 closure as a pre-existing flake the test-expert peer-review chain warned against. Filing now to stop "we'll just retry until it passes" being the de-facto practice — every flake intermittently masking a real bug costs more across sessions than it saves.

## Owner

`bug-fix` for reproduction + bisect; hand off to `code-impl` for fix.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause identified — stack trace at +0x110 traced to a specific source line in HIDService::Update, with clear reasoning for why the fault address pattern (0x3..1A and 0xFFFFFFFFFFFFFFFF) is consistent with the hypothesis.
- [ ] #2 Reproducer hardened — either a deterministic repro is constructed (e.g. by stress-driving the first-tick path) or the intermittent rate is measured precisely (e.g. 1/N over M trials with reasonable confidence).
- [ ] #3 Fix lands — race / UAF / iterator invalidation eliminated at the right layer; not papered over by sleep/retry/order-of-init hacks.
- [ ] #4 Regression coverage — the fixed code is exercised by an integration test that would have caught the original failure shape (engine startup → HID callback registration → first Update tick).
- [ ] #5 engine-invariants.md updated if the fix surfaces a load-bearing threading contract that should be anchored for future dispatchers.
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
