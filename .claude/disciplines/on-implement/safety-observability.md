# Discipline: safety-observability

Runtime safety and observability for C++ and HLSL. Operationalises `always/fundamentals.md` *fail loudly* where the compiler can't catch silent corruption.

## Assertions

```cpp
assert(in_Position < m_ElementCount && "Out-of-boundary access.");
```

## Guard clauses must log

A silent `return false` hides bugs. Every early-out that rejects work logs why:

```cpp
if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
{
    Log(Warning, "WriteMappedMemory rejected for [", gpuBuffer->m_InstanceName,
        "]: ObjectStatus is ", static_cast<int>(gpuBuffer->m_ObjectStatus),
        ", expected Activated.");
    return false;
}
```

## Log restraint on hot success paths

Failure logs (`Warning` / `Error`) on hot paths stay mandatory. This rule restricts success noise.

Before adding a success-path log: answer "how often?" concretely.

- **Hot** (per frame, per tick, per input event, per inner-loop element, per slider step) → no `Success` / `Info` log. Acceptable: silence, log on state transition only, or `Verbose` gated by a runtime switch.
- **Cold** (once at startup, once per scene load, once per explicit user command) → success log fine.

## Null checks

```cpp
if (!in_Ptr) { Log(Error, "..."); return; }
```

## RAII

Always clean up in destructors:

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

## No magic numbers

Numeric literals with non-obvious meaning → named constants. Same-line comment only when a named constant is not feasible.

```cpp
static constexpr uint32_t MaxCSMSplits = 4;
// NOT: for (int i = 0; i < 4; i++) { ... }
```

## No copy-paste

Same logic in two+ places → extract. HLSL: `common/common.hlsl`. C++: a service or utility.

## Cross-references

- `always/fundamentals.md`, `on-implement/cpp-style.md`, `on-implement/shader-standards.md`.
