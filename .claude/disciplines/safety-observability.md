# Discipline: safety-observability

Applies to agents authoring C++ or HLSL.

## Assertions (debug-time)

```cpp
assert(in_Position < m_ElementCount && "Out-of-boundary access.");
```

## Guard clauses must log

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

## Null checks before use

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

## RAII — always clean up in destructors

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

Numeric literals with non-obvious meaning must use named constants. If a named constant is not feasible, a same-line comment must explain the value.

```hlsl
cmd.m_IndexFormat = DXGI_FORMAT_R32_UINT; // named constant in common.hlsl
// NOT: cmd.m_IndexFormat = 42;
```

```cpp
static constexpr uint32_t MaxCSMSplits = 4;
// NOT: for (int i = 0; i < 4; i++) { ... }
```

## No copy-paste — extract shared logic

When the same logic appears in two or more places, extract it into a shared function or include. Duplication signals a missing abstraction. In HLSL, shared helpers belong in `common/common.hlsl`. In C++, shared logic belongs in a service or utility.
