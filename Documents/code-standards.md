# INNOCENCEENGINE CODE STANDARDS
**Version:** 1.1 - CRITICAL UPDATE: Inline Function Dependencies  
**Owner:** Code Architect  
**Status:** MANDATORY - All team members must follow

---

## 🎯 NAMING CONVENTIONS

### Classes & Structs
```cpp
// PascalCase for class names
class ObjectPool;
class TaskScheduler;
struct ArrayRangeInfo;

// Template classes with T prefix
template<typename T>
class TObjectPool;

// Interface classes with I prefix  
class IObjectPool;
class ITask;
```

### Variables & Functions
```cpp
// Member variables: m_ prefix + PascalCase
class Example {
    T* m_HeapAddress;
    size_t m_ElementCount;
    std::mutex m_PoolMutex;
};

// Local variables: l_ prefix + PascalCase
auto l_Object = pool.Spawn();
auto l_NewFreeChunk = new Chunk();
auto l_StartTime = Timer::GetCurrentTimeFromEpoch();

// Function names: PascalCase
void ConstructPool();
T* Spawn();
void Destroy(T* ptr);

// Parameters: PascalCase (no prefix)
void Function(size_t ElementCount, T* HeapAddress);
```

### Constants & Enums
```cpp
// Constants: PascalCase
static const size_t MaxPoolSize = 1024;

// Enums: PascalCase for type and values
enum class TimeUnit {
    Microsecond,
    Millisecond,
    Second
};
```

### Namespaces
```cpp
// Single word, PascalCase
namespace Inno {
    // All engine code lives here
}
```

---

## 📁 HEADER INCLUSION PATTERN

### STL Headers - MANDATORY
```cpp
// NEVER include STL headers directly
#include <mutex>        // ❌ WRONG
#include <vector>       // ❌ WRONG

// ALWAYS use engine STL wrappers
#include "STL14.h"      // ✅ CORRECT - C++14 standard library
#include "STL17.h"      // ✅ CORRECT - C++17 standard library
```

### Include Order
```cpp
#pragma once

// 1. Engine STL wrappers first
#include "STL14.h"
#include "STL17.h"

// 2. Engine common headers
#include "Memory.h"
#include "LogService.h"

// 3. Local project headers
#include "SpecificHeader.h"
```

---

## 🔧 CODE STYLE

### Inline Functions - CRITICAL RULE
```cpp
// NEVER inline functions that depend on engine APIs in headers
class TestRunner
{
public:
    static void StartTest(const char* testName);  // ✅ Declaration only
    // Implementation goes in .cpp file
};

// ❌ WRONG - inline function using g_Engine in header
inline void BadExample()
{
    Log(Success, "This creates compilation dependencies!");  // Uses g_Engine
}

// ✅ CORRECT - engine-dependent functions in .cpp files only
// TestRunner.cpp:
void TestRunner::StartTest(const char* testName)
{
    Log(Verbose, "Running test: ", testName);  // Safe in .cpp
}
```

**Rationale:** 
- Prevents circular include dependencies
- Avoids g_Engine pointer compilation issues
- Keeps headers lightweight and fast to compile
- Engine APIs (Log, Memory, etc.) should only be called from .cpp files

### Braces & Indentation
```cpp
// Opening brace on same line for functions/classes
class Example
{
    void Function()
    {
        if (condition)
        {
            // 4-space indentation (tabs)
            DoSomething();
        }
        else
        {
            DoSomethingElse();
        }
    }
};
```

### Const Correctness
```cpp
// Const member functions
const auto size() const { return m_CurrentFreeIndex; }
const auto begin() const { return m_HeapAddress; }

// Const parameters where applicable
void ProcessData(const T& data);
void Initialize(const std::string& name);
```

### Logging Pattern
```cpp
// Use engine logging, not std::cout
Log(Success, "Object pool allocated at ", this, ".");
Log(Error, "Run out of object pool!");
Log(Warning, "Last free chunk has been allocated!");
```

### Memory Management
```cpp
// Use engine memory system
auto ptr = g_Engine->Get<Memory>()->Allocate(size);
g_Engine->Get<Memory>()->Deallocate(ptr);

// NOT raw malloc/free
// malloc(size);  // ❌ WRONG
// free(ptr);     // ❌ WRONG
```

---

## 🧵 THREADING PHILOSOPHY

### Container Design
```cpp
// Containers should be lightweight and non-thread-safe by default
// Thread safety is handled at higher levels by user code
class ObjectPool {
    // No internal mutexes
    T* Spawn();           // Not thread-safe
    void Destroy(T* ptr); // Not thread-safe
};

// Users handle synchronization:
std::mutex pool_mutex;
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    auto obj = pool.Spawn();
}
```

---

## 📝 TEMPLATE PATTERNS

### Template Parameter Naming
```cpp
template <typename T>           // Generic type
template <bool ThreadSafe>      // Boolean flag
template <class T, bool Flag>   // Multiple parameters
```

### SFINAE Patterns (existing in codebase)
```cpp
template <typename U = T&>
EnableType<U, ThreadSafe> operator[](size_t pos);

template <typename U = T&>  
DisableType<U, ThreadSafe> operator[](size_t pos);
```

---

## 🚨 MANDATORY PRACTICES

### Error Handling
```cpp
// Use assertions for debug-time checks
assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");

// Use logging for runtime issues
if (!m_CurrentFreeChunk) {
    Log(Error, "Run out of object pool!");
    return nullptr;
}
```

### Resource Management
```cpp
// RAII pattern - always clean up in destructors
~ObjectPool()
{
    if (m_HeapAddress) {
        g_Engine->Get<Memory>()->Deallocate(m_HeapAddress);
        m_HeapAddress = nullptr;
    }
}
```

### Null Checks
```cpp
// Always check pointers before use
void Destroy(T* ptr)
{
    if (!ptr)
        return;
    // ... rest of function
}
```

---

## ❌ FORBIDDEN PATTERNS

```cpp
// DON'T use raw STL includes
#include <vector>           // ❌

// DON'T use Hungarian notation  
int nCount;                 // ❌
char* szName;              // ❌

// DON'T use raw memory management
malloc(size);              // ❌
new T[count];              // ❌

// DON'T use std::cout for logging
std::cout << "message";    // ❌

// DON'T make containers thread-safe by default
std::mutex m_InternalMutex; // ❌ (in containers)

// DON'T inline functions that use engine APIs in headers
inline void BadFunction()
{
    Log(Error, "This breaks compilation!");     // ❌ g_Engine dependency
    g_Engine->Get<Memory>()->Allocate(100);    // ❌ Engine API in header
}
```

---

**🎯 ENFORCEMENT:** Code reviews will check compliance with these standards.  
**📋 UPDATES:** Standards evolve - check for updates before major changes.  
**🤝 QUESTIONS:** Ask Code Architect for clarification on edge cases.
