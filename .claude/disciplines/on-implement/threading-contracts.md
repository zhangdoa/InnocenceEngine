# Discipline: threading-contracts

Containers are lightweight and non-thread-safe by default. Thread safety is the caller's responsibility.

## Convention

```cpp
class ObjectPool {
    T* Spawn();              // Not thread-safe
    void Destroy(T* in_Ptr); // Not thread-safe
};

// Caller synchronises:
std::mutex l_PoolMutex;
{
    std::lock_guard<std::mutex> l_Lock(l_PoolMutex);
    auto l_Obj = pool.Spawn();
}
```

When adding a new container or API that may be called from multiple threads → state the thread-safety contract on the declaration. Default assumption: "caller synchronises". Any deviation (internally synchronised, lock-free, reader-writer-lock-protected) must be explicit.

## Cross-references

- `on-implement/cpp-style.md` — naming and engine-abstraction conventions used in the example.
- `on-implement/safety-observability.md` — *fail loudly* applies to thread-contract violations.
- `always/fundamentals.md` — *explicit contracts* is the underlying quality bar.
