---
id: TASK-240
title: >-
  C++23 migration — lift CMake CXX_STANDARD from 17 to 23 and clear ISO C++23
  pedantic issues
status: To Do
assignee:
  - ci-build-impl
  - code-impl
created_date: '2026-06-15'
labels:
  - engine
  - cmake
  - cpp-standard
  - migration
  - build
dependencies: []
references:
  - CMakeLists.txt
  - Source/
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Lift the engine from C++17 (set at `CMakeLists.txt:29`) to C++23. C++23 is GA
(ISO/IEC 14882:2024, published October 2024); MSVC 17.8+, clang 17+, gcc 13+
all ship C++23 support. C++20 is already in practice usable but the upgrade
path is the same — pick the floor that gives the most useful features.

### Why now

- The codebase has been on C++17 since 2024. The engine is 5+ years behind.
  C++20 (concepts, ranges, modules, spaceship) and C++23 (deducing `this`,
  `std::expected`, `std::flat_map`, `std::move_only_function`, `std::print`,
  `std::mdspan`, `if consteval`, `static operator()`, multidimensional
  subscripts) all unlock code-shape wins that compound.
- TASK-23 (foundation) is touching the same headers; a std upgrade before
  engine-native containers land reduces churn (the foundation migration can
  target C++23 idioms directly).
- TASK-237 (macro fix) needs C++17 → same shape works in C++23; no blocker.
- C++23 is the natural floor once MSVC 17.8+ is the dev baseline (we are
  on MSVC 2022 / 17.x; C++23 /std:c++20 was added in 17.0, /std:c++latest
  since 17.8; /std:c++23 is the documented alias in 17.8+).

### What this task covers (proposed)

1. **CMake lift** — `CMAKE_CXX_STANDARD` 17 → 23; verify `CMAKE_CXX_STANDARD_REQUIRED`
   is `ON`; ensure `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` is unaffected.
2. **Compiler flag pass** — MSVC `/std:c++23` (or `/std:c++latest` until
   `/std:c++23` is a primary alias in 17.10+; verify at dispatch); clang/gcc
   `-std=c++23` (clang 17, gcc 13). Per-compiler shim if the project supports
   non-MSVC builds.
3. **Build green across all configs** — Debug / Release / RelWithDebInfo.
4. **Code-shape cleanup passes** (each as a focused sub-CL; do NOT bundle):
   - **C++20 features** to adopt opportunistically: `concepts` (replace
     enable_if SFINAE), `ranges` (replace raw iterator loops where
     clarity improves), `consteval` / `constexpr` extensions, designated
     initializers, `[[likely]]` / `[[unlikely]]` on hot branches.
   - **C++23 features** to adopt opportunistically: `std::expected`
     (error path in I/O + parser code), `std::move_only_function` (replace
     `std::function` where copying is wrong), `std::flat_map` /
     `std::flat_set` (hot-path small maps), `deducing this` (recursive
     CRTP cleanup), `if consteval` (compile-time branch verification),
     `static operator()` (lambda static-call operator).
   - **Source compatibility** — fix ISO C++23 pedantic issues. The
     typicals: implicit capture of structured bindings (deprecated),
     `this` capture in lambdas (deprecated), `volatile` qualifiers on
     typedefs (removed), redundant `typename` (pedantic), non-ISO
     `_Float16` etc.
5. **MSVC preprocessor compat** — C++23's tighter preprocessor rules
   can break headers that use the MSVC legacy preprocessor. Verify the
   engine's `Source/Engine/ThirdParty/` + `Source/External/` are clean
   under `/Zc:preprocessor` (we already use this flag — confirm).
6. **CI / clangd / clang-tidy** — bump baseline docs; verify
   `.clangd` + clang-tidy configs still parse under C++23.

### Non-goals (for this task; filed as follow-ups if needed)

- Modules (C++20 modules + C++23 std module). Significant CMake +
  header-organization churn; out of scope here. A separate TASK-241
  candidate for "if a future cycle wants modules."
- `std::mdspan` adoption (multidimensional GPU buffer mapping) —
  useful but a focused task, not bundled.
- Replacing the engine's own containers (`TASK-23` successor work) —
  unrelated to the language standard.

### Risk profile

- **Low**: header-only stdlib features (concepts, ranges, deducing this)
  are opt-in; no codebase-wide rewrite needed. Lift the flag, fix what
  the compiler reports, ship.
- **Medium**: `std::expected` adoption is shape-changing where used —
  focus the rollouts where the call sites are dense (parser, I/O
  paths). Don't sprinkle.
- **Medium**: MSVC preprocessor tightening can break third-party
  headers. Pin to `/Zc:preprocessor` (already on); inspect each failure.
- **High**: a one-shot adoption of C++20 AND C++23 features bundles
  risk. Prefer phased rollouts per feature with their own review.

### Decompose during planning (recommended sub-tasks)

- **TASK-240.0 — RFC**: confirm the target compiler matrix (MSVC / clang
  / gcc minimum versions), the C++23 feature set to adopt, the
  exclusion list, the rollout order. File at `.backlog/docs/doc-N-task-240-cpp23-migration-rfc.md`.
- **TASK-240.1 — flag lift + build green**: just `CMAKE_CXX_STANDARD`
  change + clear pedantic issues. No new std features adopted.
- **TASK-240.2 — C++20 features**: concepts / ranges / consteval.
- **TASK-240.3 — C++23 features**: std::expected / move_only_function /
  deducing this / flat_map etc, in priority order from the RFC.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 RFC filed at `.backlog/docs/doc-N-task-240-cpp23-migration-rfc.md` covering: target compiler matrix, feature adoption order, exclusion list, per-feature rollout risk
- [ ] #2 `CMAKE_CXX_STANDARD 17 → 23` lands; build green in Debug / Release / RelWithDebInfo on Windows + Linux + Mac (or explicit "Windows-only is acceptable at this stage" decision)
- [ ] #3 No `ISO C++23` pedantic warnings remain in engine source; third-party headers exempted via `SYSTEM` include or explicit `pragma` wher
- [ ] #4 TestSuite green (113/113 + new tests from per-feature sub-tasks)
- [ ] #5 TestGIScene / TestPathTracerThreeScenes green; no visual regression
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this is the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
