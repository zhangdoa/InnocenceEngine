---
name: threading-contracts
description: Use when designing a container, service, or API that may be called from multiple threads. Default contract is "caller synchronizes" — declare any deviation explicitly.
---

# Threading contracts

Containers are lightweight and non-thread-safe by default. Thread safety is the caller's responsibility unless declared otherwise.

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

## Reader/writer split with shared_mutex

When a container is read on one thread and written on another:

- Reader takes `std::shared_lock`.
- Writer takes `std::unique_lock`.
- Reader path snapshots matching events into a local container under shared_lock; dispatches user callbacks **outside** the critical section so callbacks cannot deadlock against the writer side.
- MSVC SRW-backed `std::shared_mutex` does not allow a thread holding shared_lock to acquire unique_lock on the same mutex.

## Anti-patterns

- **Implicit thread-safety claim.** API doc says "thread-safe" without saying which guarantees (read-many vs read-write, or what the owner mutex is). State the contract concretely.
- **Holding the lock across user callbacks.** Snapshot the matching set, drop the lock, then call.
- **Mixing internally-synchronised and caller-synchronised methods on one type.** Pick one model per type; consumers can't reason about half-thread-safe APIs.
