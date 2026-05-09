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

## Serialized enums tail-append

Enum types serialized by integer value into asset metadata (`Bin/Data/Generated/Components/*.json`, binary blobs) commit a contract: each tag's int identity is part of the asset format. Mid-inserting a new tag shifts every subsequent value and silently invalidates every persisted asset.

**Rule**: new tags in serialized-by-integer enums tail-append. Never mid-insert. Position is the contract until the asset format gains a versioned format-name table.

**Why**: prereq commit `815f23af` mid-inserted `RGB10A2` at slot 6 in `TexturePixelDataFormat`, shifting `Depth`/`DepthStencil`/`BC1..BC5`. JSON metadata stored under the original enum (`"PixelDataFormat": 8` = `BC1`) read post-insert as `DepthStencil`; DX12 mapper returned `DXGI_FORMAT_UNKNOWN`, GISponza fatal-exited at scene-load. Forward-only fix at `0da9e278`.

**Affected enums** (audit candidates): `TexturePixelDataFormat`, `TexturePixelDataType`, `TextureSampler`, `TextureUsage`, `TextureFilterMethod`, `TextureWrapMethod` in `Source/Engine/Common/GraphicsPrimitive.h`. Verify by grepping for the enum's name across `Bin/Data/Generated/**/*.json` before declaring an enum runtime-only.

**Antipattern**: "Insert in the matching tag-style band (next to BC*, RGBA, etc.)." Tag-grouping is documentation; serialized identity is the contract. Doc-style ordering does not take precedence over format stability.

## SFINAE

```cpp
template <typename U = T&>
EnableType<U, ThreadSafe> operator[](size_t in_Position);

template <typename U = T&>
DisableType<U, ThreadSafe> operator[](size_t in_Position);
```

## Cross-references

- `safety-observability`, `threading-contracts` (user level), `fundamentals`.
