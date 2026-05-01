# Discipline: cpp-style

Applies to agents writing or modifying C++ in this project (low-level, platform, graphics-api, rendering-researcher's pass-level C++, test, software-architect, editor's native layer). Engine-specific naming, organisation, and abstraction conventions that differ from raw STL / Hungarian / generic-C++ style.

## How

### Naming

| Category | Convention | Example |
|----------|-----------|---------|
| Classes & structs | PascalCase | `ObjectPool`, `ArrayRangeInfo` |
| Template classes | `T` prefix | `TObjectPool<T>` |
| Interfaces | `I` prefix | `IObjectPool`, `ITask` |
| Member variables | `m_` + PascalCase | `m_HeapAddress`, `m_ElementCount` |
| Local variables | `l_` + PascalCase | `l_Object`, `l_StartTime` |
| Parameters | `in_` / `out_` + PascalCase | `in_ElementCount`, `out_Result` |
| Functions | PascalCase | `ConstructPool()`, `Spawn()` |
| Constants | PascalCase | `MaxPoolSize`, `DXGI_FORMAT_R32_UINT` |
| Enums | PascalCase type + values | `TimeUnit::Millisecond` |
| Namespaces | Single word, PascalCase | `namespace Inno {}` |
| Template params | `T` for types, descriptive for flags | `typename T`, `bool ThreadSafe` |

### File organisation

#### Include order

```cpp
#pragma once

// 1. Engine STL wrappers (never raw STL headers)
#include "STL14.h"
#include "STL17.h"

// 2. Engine common headers
#include "Memory.h"
#include "LogService.h"

// 3. Local project headers
#include "SpecificHeader.h"
```

#### Header vs. source separation

Functions that call engine APIs (`Log`, `g_Engine`, memory system) must be implemented in `.cpp` files, never inlined in headers.

```cpp
// Header — declaration only
class TestRunner
{
public:
    static void StartTest(const char* in_TestName);
};

// Source — engine API calls live here
void TestRunner::StartTest(const char* in_TestName)
{
    Log(Verbose, "Running test: ", in_TestName);
}
```

Rationale: prevents circular include dependencies and keeps headers lightweight.

### Formatting

Allman braces. Tab indentation (4-space width). Const-correctness throughout:

```cpp
class Example
{
    const auto size() const { return m_CurrentFreeIndex; }
    void ProcessData(const T& in_Data);
    void Function()
    {
        if (condition)
        {
            DoSomething();
        }
    }
};
```

### Engine abstractions — no raw equivalents

| Raw | Engine replacement |
|-----|-------------------|
| `#include <vector>`, `#include <mutex>`, etc. | `#include "STL14.h"` / `#include "STL17.h"` |
| `malloc` / `free` / `new[]` / `delete[]` | `g_Engine->Get<Memory>()->Allocate()` / `Deallocate()` |
| `std::cout` | `Log(Level, ...)` |
| Hungarian notation (`nCount`, `szName`) | Engine prefix convention (see Naming) |

### Templates — SFINAE patterns

```cpp
template <typename U = T&>
EnableType<U, ThreadSafe> operator[](size_t in_Position);

template <typename U = T&>
DisableType<U, ThreadSafe> operator[](size_t in_Position);
```

## Cross-references

- `safety-observability.md` — paired discipline for the runtime / observability rules (assertions, guard-clause logging, RAII, no magic numbers, no copy-paste).
- `threading-contracts.md` — applies on top of this discipline when the API surface is multi-threaded; thread-safety contract goes on the declaration.
- `fundamentals.md` — *be explicit in code* runs first; this discipline is the engine-specific operationalisation for C++.
