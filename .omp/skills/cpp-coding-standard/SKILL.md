---
name: cpp-coding-standard
description: "C++ conventions for InnocenceEngine: naming, engine abstractions, header/source split, threading, file-split shape."
---

## Naming

| Category | Convention | Example |
|---|---|---|
| Class / struct | PascalCase | `ObjectPool` |
| Template class | `T` prefix | `TObjectPool<T>` |
| Interface | `I` prefix | `IObjectPool` |
| Member | `m_` + PascalCase | `m_ElementCount` |
| Local | `l_` + PascalCase | `l_Object` |
| Param | `in_` / `out_` + PascalCase | `in_ElementCount`, `out_Result` |
| Function | PascalCase | `ConstructPool()` |
| Constant | PascalCase | `MaxPoolSize` |
| Enum | PascalCase type + values | `TimeUnit::Millisecond` |
| Namespace | one word, PascalCase | `Inno` |

## Engine abstractions (no raw equivalents)

| Raw | Engine |
|---|---|
| `<vector>`, `<mutex>`, … | `STL14.h` / `STL17.h` |
| `malloc` / `free` / `new[]` / `delete[]` | `g_Engine->Get<Memory>()->Allocate()` / `Deallocate()` |
| `std::cout` | `Log(Level, …)` |

## Structure

- Functions calling engine APIs (`Log`, `g_Engine`, Memory) live in `.cpp`, never inlined in headers.
- Containers are non-thread-safe by default; the caller synchronizes (`std::lock_guard`).
- RAII: release engine resources in the destructor (`Memory->Deallocate`, then null the handle).

## File split (commit-guard blocks at > 300 lines and growing)

| Unit | Shape |
|---|---|
| Translation unit | `Foo.cpp` + `Foo_<Responsibility>.cpp` (e.g. `Mesh_Allocation.cpp`) |
| Header | split by domain `Foo_<Domain>.h`; `Foo.h` umbrella `#include`s the parts |
| Suffix | the responsibility, never `Impl` / `2` |

After adding files: `cmake -B Build -S .` — GLOB re-picks new files only on configure.
