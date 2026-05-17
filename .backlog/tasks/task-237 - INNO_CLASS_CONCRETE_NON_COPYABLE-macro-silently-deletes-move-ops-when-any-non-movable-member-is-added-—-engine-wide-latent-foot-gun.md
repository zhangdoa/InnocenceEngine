---
id: TASK-237
title: >-
  INNO_CLASS_CONCRETE_NON_COPYABLE macro silently deletes move ops when any
  non-movable member is added — engine-wide latent foot-gun
status: To Do
assignee: []
created_date: '2026-05-17 16:57'
labels:
  - engine
  - macros
  - cpp-style
  - foot-gun
  - followup
dependencies: []
references:
  - 8355775a
  - Source/Engine/Common/ClassTemplate.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient_Internal.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-233 peer review (commit `8355775a`).

`Source/Engine/Common/ClassTemplate.h:11-17` defines `INNO_CLASS_CONCRETE_NON_COPYABLE` to:
- delete copy ctor / copy-assign,
- explicitly default move ctor / move-assign (`= default`).

When any class using this macro adds a non-movable member (most commonly `std::atomic<T>`, also mutexes, condition variables, `std::unique_ptr` with custom deleter, etc.), the defaulted move ops are **implicitly deleted** by the compiler. The class compiles silently because nothing in tree currently moves it, but:

1. clangd surfaces `-Wdefaulted-function-deleted` warnings that read as scary regressions on otherwise-clean CLs (the TASK-233 review brief alleged a regression on this basis; turned out to be pre-existing).
2. Any future code that vectors, moves, or uses move-only semantics on the affected class hits a confusing error far from the actual cause.
3. The "= default" declaration is misleading — it asserts movability while the compiler silently disagrees.

Affected classes confirmed: `ExampleRenderingClientImpl` (TASK-233 added atomics here). Likely affected: any other `INNO_CLASS_CONCRETE_NON_COPYABLE` user with a `std::atomic`, mutex, or similar member. **Survey scope is part of this task.**

## Fix options (pick during task scoping)

1. Change macro to `= delete` the move ops (asserts move-not-supported, fails loudly if attempted).
2. Drop the move ops entirely from the macro (let the compiler decide implicitly, no default declaration to be wrong about).
3. Split macro into two variants: `..._NON_COPYABLE_MOVABLE` vs `..._NON_COPYABLE_NON_MOVABLE`.

Option 2 is the simplest; option 3 is the most explicit. Implementer evaluates.

Pre-existing engine-wide issue, not a regression. Surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Engine-wide survey of `INNO_CLASS_CONCRETE_NON_COPYABLE` users with non-movable members completed; list captured in task notes
- [ ] #2 Macro fix shape chosen and applied (delete-moves, drop-moves, or split-macro)
- [ ] #3 All `-Wdefaulted-function-deleted` warnings attributable to the macro cleared
- [ ] #4 Build green across all configurations
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
