---
id: TASK-23.4
title: >-
  Engine-native HashMap: new container backed by Allocator (successor to
  std::unordered_map inside ThreadSafeUnorderedMap)
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 4000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add `Inno::HashMap<Key, T>` (or similar name — record the chosen name in the implementation notes) to Source/Engine/Common/. Intended to replace `std::unordered_map` inside `ThreadSafeUnorderedMap` and at direct call sites.

Implementation choices (record the pick + why):
- **Open addressing, linear probing** — best cache locality, simplest. Robin-hood / hopscotch variants for better worst-case.
- **Open addressing, quadratic probing** — fewer clustering issues than linear.
- **Chained buckets with intrusive list** — std::unordered_map shape; bigger but predictable.

Recommended: open addressing + robin-hood — it's the modern default (matches absl::flat_hash_map / boost::unordered_flat_map). But the user picks.

Surface to expose:
- insert / emplace / try_emplace, erase, find, contains (C++20), [], at, size, empty, reserve, rehash, clear, iteration.
- Hash policy: customizable; default `std::hash<Key>` for primitive + string-like keys.

References:
- Source/Engine/Common/Array.h (template for shape)
- Source/Engine/Common/ThreadSafeUnorderedMap.h
- All `std::unordered_map` usages in Source/Engine/
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Source/Engine/Common/HashMap.h (or chosen filename) exists with Inno::HashMap<Key,T>.
- [ ] #2 Implementation choice documented in header comment block (open-addressing-robin-hood / chained / etc.).
- [ ] #3 Uses Allocator for memory (depends on TASK-23.1).
- [ ] #4 Surface includes: insert, emplace, try_emplace, erase, find, contains, operator[], at, size, empty, reserve, rehash, clear, iterator/const_iterator.
- [ ] #5 Rehash correctly handles non-trivially-copyable Key and T.
- [ ] #6 UnitTest covers: insert/find/erase, collision resolution, rehash trigger, iterator stability semantics (documented and tested), copy/move, non-trivial Key (e.g. std::string).
- [ ] #7 StressTest: 10^6 insert + 50% erase + lookup pass without leak.
- [ ] #8 Perf-vs-STL: insert N + lookup N + erase N vs std::unordered_map. Target competitive (within 1.5× — and ideally faster than std::unordered_map for trivial keys).
- [ ] #9 Direct std::unordered_map usages outside ThreadSafeUnorderedMap swapped where reasonable (count holdouts).
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
