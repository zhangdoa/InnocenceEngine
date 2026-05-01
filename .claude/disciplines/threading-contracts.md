# Discipline: threading-contracts

Applies to agents working on code that can be called from multiple threads or from worker pools. Containers are lightweight and non-thread-safe by default; thread safety is the caller's responsibility.

## How

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

When adding a new container or API that may be called from multiple threads, state the thread-safety contract on the declaration — not in a separate comment file, not implicitly. The default assumption is "caller synchronises"; any deviation (internally synchronised, lock-free, reader-writer-lock-protected) must be explicit.

## Cross-references

- `cpp-style.md` — naming conventions and engine-abstraction rules used in the example.
- `safety-observability.md` — *fail loudly* applies to thread-contract violations: a function that quietly corrupts under racy use is the failure mode this discipline prevents.
- `fundamentals.md` — *explicit contracts* is the underlying quality bar; threading is one of the contract surfaces this discipline makes explicit.
