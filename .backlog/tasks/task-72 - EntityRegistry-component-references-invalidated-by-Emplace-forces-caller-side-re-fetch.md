---
id: TASK-72
title: >-
  EntityRegistry: component references invalidated by Emplace forces caller-side
  re-fetch
status: To Do
assignee: []
created_date: '2026-04-18 17:37'
labels:
  - architecture
  - ecs
  - api-design
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Source/ExampleProject/LogicClient/Player.inl` line ~123 has the canonical smell:

```cpp
// Re-fetch: Emplace above may have reallocated CameraComponent storage, invalidating m_PlayerCameraComponent
m_PlayerCameraComponent = l_Registry->Get<CameraComponent>(m_PlayerCameraEntity);
```

The caller cached a `CameraComponent*`, called `Emplace<CameraComponent>` for a different entity, and had to re-fetch. This is the engine leaking storage-layout churn through a nominally stable API — callers cannot safely hold component pointers across any `Emplace` of the same component type.

Either the usage pattern is wrong (callers should never cache pointers; always call `Get<T>(entity)` at the use site) or the storage is wrong (a `std::deque`-backed storage gives pointer stability on push_back; other component storages already use this trick — AssetService comment explicitly calls it out). Pick one and enforce it.

Three reasonable resolutions:
1. **Stable-pointer storage** — switch the component storage to a container with push-stable pointers (`std::deque`, chunked array, or slot-map with pointer-stable payloads). The caller's cached pointer is then always valid for the entity's lifetime.
2. **Handles, not pointers** — return an opaque `ComponentHandle<T>` from `Get`/`Emplace` that the caller can hold; resolve-to-pointer at each use site is cheap (index + generation). No cached raw pointer to invalidate.
3. **Status-quo formalized** — document "component pointers are invalidated by any `Emplace<T>` of the same type" as the contract, audit every cached-pointer site, and put an assert in debug builds that detects stale-pointer dereferences (e.g. a per-storage version counter).

(1) is the least invasive for callers. (2) is the cleanest long-term. (3) is a holding pattern. Pick before the next non-trivial ECS change lands.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Pick one of the three resolutions and document the rationale in EntityRegistry.h
- [ ] #2 No caller needs to re-fetch after unrelated Emplace calls
- [ ] #3 All existing `// Re-fetch: Emplace above …` comments can be deleted
- [ ] #4 RenderTest / Main 10-frame / reload regression green
<!-- AC:END -->
