---
name: cpp-style
description: Use when writing engine C++. Defines naming (PascalCase, m_/l_/in_/out_), include order, and engine STL/memory/log replacements for raw STL.
---

# Skill: cpp-style

Engine-specific conventions that differ from raw STL / Hungarian / generic-C++ style.

## Naming

| Category | Convention | Example |
|---|---|---|
| Classes & structs | PascalCase | `ObjectPool` |
| Template classes | `T` prefix | `TObjectPool<T>` |
| Interfaces | `I` prefix | `IObjectPool` |
| Member variables | `m_` + PascalCase | `m_HeapAddress` |
| Local variables | `l_` + PascalCase | `l_Object` |
| Parameters | `in_` / `out_` + PascalCase | `in_ElementCount`, `out_Result` |
| Functions | PascalCase | `ConstructPool()` |
| Constants | PascalCase | `MaxPoolSize` |
| Enums | PascalCase type + values | `TimeUnit::Millisecond` |
| Namespaces | Single word, PascalCase | `namespace Inno {}` |
| Template params | `T` for types, descriptive for flags | `bool ThreadSafe` |

## Formatting

- Allman braces.
- Tab indentation (4-space width).
- Const-correctness throughout.

## Include order

1. Engine STL wrappers (`STL14.h`, `STL17.h`) — never raw STL headers.
2. Engine common headers (`Memory.h`, `LogService.h`).
3. Local project headers.

## Header / source separation

Functions calling engine APIs (`Log`, `g_Engine`, memory system) → `.cpp` only, never inlined in headers.

## No raw equivalents

| Raw | Engine replacement |
|---|---|
| `<vector>`, `<mutex>`, etc. | `STL14.h` / `STL17.h` |
| `malloc` / `new[]` / `free` / `delete[]` | `g_Engine->Get<Memory>()->Allocate()` / `Deallocate()` |
| `std::cout` | `Log(Level, ...)` |
| Hungarian (`nCount`, `szName`) | Engine prefix convention (see Naming) |

## SFINAE

```cpp
template <typename U = T&>
EnableType<U, ThreadSafe> operator[](size_t in_Position);

template <typename U = T&>
DisableType<U, ThreadSafe> operator[](size_t in_Position);
```

## Cross-references

- `safety-observability`, `threading-contracts` (user level), `fundamentals`.
