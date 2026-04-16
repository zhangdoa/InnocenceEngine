# INNOCENCE ENGINE CODE STANDARDS

**Status:** MANDATORY — All team members must follow

---

## 0. Principles

- **Functional programming** — avoid complex state machines; inputs generate outputs in a deterministic way, unless quantum-randomized.
- **Data-oriented programming** — everything should serve, observe, manipulate, and deliver data, rather than processing data for the sake of processing.
- **Design by contract** — enforce data in and out with pre- and post-conditions; predict the possibilities rather than react to the realities.
- **Avoid premature abstraction** — be pragmatic when modeling; everything eventually comes from and goes to hardware. Don't OOP because you know `class`.
- **Avoid premature optimization** — always define a clear performance baseline and measure; optimize only if it doesn't match. Don't write obscure algorithms just because you can.
- **Be explicit in code** — modern code is for humans to read first and machines to execute second; otherwise, everybody should write in instruction sets or assembly.
- **Be explicit in domain-specific code** — not everyone is an expert in your domain, and sometimes even you aren't, a couple thousand commits ago or later.
- **Save your time by proactively refactoring** — only leave a piece of code alone if all of the above are satisfied.

## 1. Naming

| Category | Convention | Example |
|----------|-----------|---------|
| Classes & structs | PascalCase | `ObjectPool`, `ArrayRangeInfo` |
| Template classes | `T` prefix | `TObjectPool<T>` |
| Interfaces | `I` prefix | `IObjectPool`, `ITask` |
| Member variables | `m_` + PascalCase | `m_HeapAddress`, `m_ElementCount` |
| Local variables | `l_` + PascalCase | `l_Object`, `l_StartTime` |
| Parameters | `in_`/`out_` + PascalCase | `in_ElementCount`, `out_Result` |
| Functions | PascalCase | `ConstructPool()`, `Spawn()` |
| Constants | PascalCase | `MaxPoolSize`, `DXGI_FORMAT_R32_UINT` |
| Enums | PascalCase type + values | `TimeUnit::Millisecond` |
| Namespaces | Single word, PascalCase | `namespace Inno {}` |
| Template params | `T` for types, descriptive for flags | `typename T`, `bool ThreadSafe` |

---

## 2. File Organization

### Include order

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

### Header vs. source separation

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

**Rationale:** Prevents circular include dependencies and keeps headers lightweight.

---

## 3. Formatting

### Braces & indentation

Allman style. Tab indentation (4-space width).

```cpp
class Example
{
    void Function()
    {
        if (condition)
        {
            DoSomething();
        }
        else
        {
            DoSomethingElse();
        }
    }
};
```

### Const correctness

```cpp
const auto size() const { return m_CurrentFreeIndex; }
void ProcessData(const T& in_Data);
```

---

## 4. Engine Abstractions

Use engine systems instead of raw equivalents. No exceptions.

| Raw | Engine replacement |
|-----|-------------------|
| `#include <vector>`, `#include <mutex>`, etc. | `#include "STL14.h"` / `#include "STL17.h"` |
| `malloc`/`free`/`new[]`/`delete[]` | `g_Engine->Get<Memory>()->Allocate()` / `Deallocate()` |
| `std::cout` | `Log(Level, ...)` |
| Hungarian notation (`nCount`, `szName`) | Engine prefix convention (see Naming) |

---

## 5. Safety & Observability

All rules in this section apply to both C++ and HLSL unless noted.

### Assertions (debug-time)

```cpp
assert(in_Position < m_ElementCount && "Out-of-boundary access.");
```

### Guard clauses must log

A silent `return false` hides bugs. Every early-out that rejects work must log why:

```cpp
if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
{
    Log(Warning, "WriteMappedMemory rejected for [", gpuBuffer->m_InstanceName,
        "]: ObjectStatus is ", static_cast<int>(gpuBuffer->m_ObjectStatus),
        ", expected Activated.");
    return false;
}
```

### Null checks before use

```cpp
void Destroy(T* in_Ptr)
{
    if (!in_Ptr)
    {
        Log(Error, "Unable to destroy ", in_Ptr);
        return;
    }
    // ...
}
```

### RAII — always clean up in destructors

```cpp
~ObjectPool()
{
    if (m_HeapAddress)
    {
        g_Engine->Get<Memory>()->Deallocate(m_HeapAddress);
        m_HeapAddress = nullptr;
    }
}
```

### No magic numbers

Numeric literals with non-obvious meaning must use named constants. If a named constant is not feasible, a same-line comment must explain the value.

```hlsl
cmd.m_IndexFormat = DXGI_FORMAT_R32_UINT; // named constant in common.hlsl
// NOT: cmd.m_IndexFormat = 42;
```

```cpp
static constexpr uint32_t MaxCSMSplits = 4;
// NOT: for (int i = 0; i < 4; i++) { ... }
```

### No copy-paste — extract shared logic

When the same logic appears in two or more places, extract it into a shared function or include. Duplication signals a missing abstraction. In HLSL, shared helpers belong in `common/common.hlsl`. In C++, shared logic belongs in a service or utility.

---

## 6. Threading

Containers are lightweight and non-thread-safe by default. Thread safety is the caller's responsibility:

```cpp
class ObjectPool {
    T* Spawn();              // Not thread-safe
    void Destroy(T* in_Ptr); // Not thread-safe
};

// Caller synchronizes:
std::mutex l_PoolMutex;
{
    std::lock_guard<std::mutex> l_Lock(l_PoolMutex);
    auto l_Obj = pool.Spawn();
}
```

---

## 7. Templates

### SFINAE patterns (existing in codebase)

```cpp
template <typename U = T&>
EnableType<U, ThreadSafe> operator[](size_t in_Position);

template <typename U = T&>
DisableType<U, ThreadSafe> operator[](size_t in_Position);
```

---

## 8. GPU / Shader

### Matrix multiplication convention

The engine uploads **row-major** matrices. HLSL default storage is **column-major** (GPU sees the transpose). Always use row-vector * matrix order:

```hlsl
float4 viewPos = mul(myVector, g_Frame.p_inv);     // CORRECT
// NOT: mul(g_Frame.p_inv, myVector);               // transposes the operation
```

Reference: `common/skyResolver.hlsl` uses this convention consistently.

### Default transform must be identity, not zero

When a transform is unavailable, fall back to **identity**, never `Mat4{}` (all zeros collapses geometry):

```cpp
Mat4 l_Transform = l_world ? l_world->m_WorldMatrix : Math::generateIdentityMatrix<float>();
```

### Cross-queue UAV writes require DeviceMemoryBarrier

Any compute shader that writes a UAV buffer consumed by another queue must issue `DeviceMemoryBarrier()` after all writes. Fence signaling guarantees command list retirement, not write visibility through the GPU memory hierarchy.

```hlsl
u_DrawCommandBuffer[objectIndex] = BuildIndirectDrawCommand(objectIndex, modelData, isVisible);
DeviceMemoryBarrier(); // writes visible to graphics queue after fence
```

---

**ENFORCEMENT:** Code reviews check compliance.
**UPDATES:** Check for updates before major changes.
