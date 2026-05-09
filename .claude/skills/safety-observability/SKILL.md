---
name: safety-observability
description: Use when writing engine C++ runtime safety. Engine-specific patterns for assertions, guard-clause Log() patterns, RAII destructors that release engine memory, and log restraint on hot success paths.
---

# Skill: safety-observability

Generic safety principles (assertions, guard-clauses must log, no magic numbers, no copy-paste) live in user-level `safety-principles` skill. This skill covers engine-specific operationalisations.

## Assertions

```cpp
assert(in_Position < m_ElementCount && "Out-of-boundary access.");
```

## Guard clauses must log via engine `Log()`

A silent `return false` hides bugs. Every early-out that rejects work logs why, using the engine `Log()` macro:

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

## RAII for engine-managed memory

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

## HLSL: no copy-paste (engine venue)

HLSL: `common/common.hlsl`. C++: a service or utility.

## Cross-references

- `cpp-style`, `shader-standards` — naming and engine-abstraction conventions.
- User-level `safety-principles` — the abstract rules this operationalises.
