# Component Access Model — Data Ops, No Raw References

## Goal

Replace the current `EntityRegistry::Get<T>(e) → T*` public API with a Read/Write/Modify surface that operates on components as **data keyed by `(EntityID, type)`**, never as objects with identity the caller holds. Combined with preflight-reserved component storage on scene load and tombstone-based removal, this makes the entire "dangling component pointer" bug class (TASK-72) structurally impossible.

## Motivation

The pointer-caching hazard is a repeat offender:

- `Player.inl:~123` — cached `CameraComponent*`, had to re-fetch after an unrelated `Emplace<CameraComponent>` realloc'd storage. Workaround documented as a comment, not fixed at the API layer.
- `JSONWrapper::LoadChildScene` — cached `TransformComponent*` across a per-child `Emplace<TransformComponent>` loop. Storage realloc on the 5th ShaderBall child dangled the pointer; reads returned subnormal-float garbage (rot=9.91e-39…) on some runs, coincidentally-zeroed on others. Filed as TASK-109, narrowed during the session on 2026-04-20, patched in commit `cfc1c8d1` (snapshot-by-value), root cause traced to this same class.
- `TComponentStorage::Remove` also moves a dense element via swap-and-pop. Any held `T*` dangles even without a realloc.

Every new caller has to independently rediscover "don't hold component pointers across any `Emplace`/`Remove` of the same type." The API offers a raw pointer that contradicts its own contract.

The fix is to not offer the pointer.

## Goals

1. Public `EntityRegistry` API offers no way to escape a raw `T*` for a component slot.
2. Scene load does not reallocate component storage — component counts are known from the scene JSON, storage is reserved to the exact count before loading begins.
3. `Remove` and `CleanUp` do not shuffle dense-array indices during a scene's lifetime. Within a scene, dense storage only grows (monotonic).
4. Hot-path iteration (`LightDataService`, `DrawCallService`, `TransformService`, etc.) keeps contiguous-array performance.

## Non-Goals

- Access contracts (`Reads<T>`/`Writes<T>` system declarations, scheduler validation). Sequential service dispatch today makes this unnecessary; revisit when/if parallel system dispatch is introduced.
- Generation-tagged `EntityID`. The pre-allocation + tombstone model removes the need for lifetime-validation bits during normal operation; a stale `EntityID` resolves to "slot is empty" which is already observable via the API.
- Per-entity locks or lock-free atomics. Single-threaded component access today; no concurrency to arbitrate.
- Migrating `AssetHandle<T>`, the asset-layer reference. It already solves its own layer's problem and is out of scope.
- Baked flattened optimized storage (SoA, archetype chunks). Viable later as a pure performance pass; not needed now.

## Design

### Three registry access ops

```cpp
namespace Inno
{
    class EntityRegistry
    {
    public:
        // Returns a value copy of the component. Asserts the slot exists
        // (callers check with Has<T>(e) first when existence is uncertain).
        template<typename T> T Read(EntityID e) const;

        // Overwrites the whole component. Asserts the slot exists.
        template<typename T> void Write(EntityID e, T value);

        // Scope-bounded mutable access. The T& is valid only within fn's
        // scope; it cannot escape because the lambda cannot capture a
        // longer-lived reference to it. This is the only path that hands
        // out a T& at all.
        template<typename T, typename Fn> void Modify(EntityID e, Fn&& fn);

        // Existence check (unchanged).
        template<typename T> bool Has(EntityID e) const;

        // Adds a new component slot. Asserts the slot does not exist.
        // Thin wrapper around TComponentStorage<T>::Add; used by scene
        // load and by editor-driven component addition.
        template<typename T> void Emplace(EntityID e, T data = {});

        // Removes the slot (tombstones, see below). Safe to call on a
        // non-existent slot.
        template<typename T> void Remove(EntityID e);
    };
}
```

No `Get<T>(e) → T*` in the public API. The previous `Get` signature is removed. Any callsite that needs to read ten fields should `auto t = registry.Read<T>(e);` and read locally; any callsite that needs to mutate a field should use `Modify<T>(e, [&](T& t){ ... })`.

### Raw iteration for hot paths — named, scoped, contracted

```cpp
template<typename T> std::span<T>       RawIterate();        // mutable
template<typename T> std::span<const T> RawIterate() const;  // read-only
```

The name is deliberately loud. The contract, documented in header:

1. Single-threaded: no parallel access to the storage while a span is outstanding.
2. No structural mutation of the iterated storage (`Emplace<T>`, `Remove<T>`, `CleanUp`) during iteration. Caller is responsible; debug-mode mutation counter asserts on violation.
3. Elements with `m_Owners[i] == INVALID_ENTITY` are tombstones; iteration must skip them.

Current expected consumers: `LightDataService::UpdateLightData`, `DrawCallService`, `AnimationSimulationService`, `TransformService` propagation. Each migrates to `RawIterate<T>()` with a tombstone-skip loop.

### `TComponentStorage<T>::Reserve` + scene-load preflight

```cpp
template<typename T> class TComponentStorage {
public:
    void Reserve(size_t n) {
        m_Dense.reserve(n);
        m_Owners.reserve(n);
        m_Lifespans.reserve(n);
    }
    // ...
};
```

`SceneService::LoadSync` gains a preflight pass:

1. Parse the root scene JSON into an in-memory `nlohmann::json` tree. Recursively walk the `ChildScene` references, parsing those too.
2. Walk the tree counting components by type.
3. Call `registry.ReserveComponentStorage<T>(count)` (a thin wrapper that reaches into `TComponentStorage<T>::Reserve`) for each component type seen.
4. Only then run the actual emplace loop over the parsed tree.

The preflight parse holds the full JSON tree in memory for the duration of load. Scene files are small (tens of KB), this is cheap.

After preflight + reserve, **no `Emplace<T>` during scene load can reallocate the dense vector.** Combined with the no-raw-pointer API, the `LoadChildScene` bug class is structurally dead — both because no caller is offered a pointer to dangle, and because the vector backing cannot move.

### `Remove` becomes a tombstone

```cpp
void Remove(EntityID entity) {
    if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES) return;
    if (m_Sparse[entity] == InvalidIndex) return;

    const uint32_t denseIdx = m_Sparse[entity];
    m_Owners[denseIdx] = INVALID_ENTITY;   // tombstone sentinel
    m_Sparse[entity]   = InvalidIndex;
    // Dense element retained in place. No swap, no pop, no realloc.
}
```

Invariant: **dense storage only grows during a scene's lifetime.** Indices within a dense array are stable for the scene's lifetime (not promised in the API, but an internal property that simplifies reasoning). Destroying an entity does not shift any other entity's dense position.

Memory cost: a scene that churns entities during play accumulates tombstones. In this engine entities are mostly static after load; steady-state tombstone count is near zero. A `Compact()` entry point can be added later if a workload ever needs it.

### `CleanUp(ObjectLifespan::Scene)` becomes bulk

```cpp
void CleanUp(ObjectLifespan lifespan) {
    if (lifespan == ObjectLifespan::Scene) {
        m_Dense.clear();       // preserves capacity, drops size to 0
        m_Owners.clear();
        m_Lifespans.clear();
        m_Sparse.fill(InvalidIndex);
        return;
    }
    // Persistent and other lifespans: fall back to per-entry removal
    // (same logic as before, limited to the matching lifespan).
    for (int32_t i = static_cast<int32_t>(m_Dense.size()) - 1; i >= 0; --i) {
        if (m_Lifespans[i] == lifespan)
            Remove(m_Owners[i]);
    }
}
```

Bulk clear replaces the current O(n) per-entity Remove loop at scene unload. Capacity is preserved across unload→reload so the new scene's reserve often requires no reallocation at all.

## Migration

~50-150 callsites today cache component pointers. All get nuked in this CL series:

### Audit tooling

Grep patterns to enumerate sites:

- `Get<.*Component>\(` — component-pointer fetches.
- `\*\s*m_\w*Component\b` — cached pointer members.
- `// Re-fetch` — self-documented workarounds for the exact bug.

Results go into a migration checklist appended to TASK-72. Each site moves to one of:

- `registry.Read<T>(e)` if it's reading once and not mutating.
- `registry.Modify<T>(e, Fn)` if it's doing a read-modify-write on a few fields.
- `registry.Write<T>(e, value)` if it's overwriting wholesale (setter-from-JSON paths).
- `RawIterate<T>()` if it's a hot-path sweep over all components of one type.

### Split into reviewable commits

The full migration is large; it splits naturally by subsystem:

1. API introduction + `TComponentStorage` changes + preflight load (engine core, no callsite migration yet — `Get<T>` stays available as an internal/deprecated alias).
2. Services migration (LightDataService, DrawCallService, TransformService, AnimationSimulationService, AnimationDrawCallService, etc.).
3. Scene load / save migration (JSONWrapper, AssetService).
4. Editor / IPC callsite migration.
5. Player / LogicClient migration.
6. `Get<T>` removal (public API delete).

Each commit is atomic, passes tier-2 and tier-3 integration, and the serialize-determinism gate when touching JSON/Scene/Asset code.

### Test strategy

- Existing tier-1 / tier-2 / tier-3 gates must stay green at every commit.
- New behavioral test: after a scene load, iterate every entity and verify no component's stored data matches the uninitialized-memory signature (subnormal floats, sentinel bit patterns). Catches both "forgot to reserve" and "new realloc-introducing path sneaked in."
- Serialize-determinism (tier 5) remains green after migration — no change to save/load semantics, only to how the code reaches the data.

## Risks and Residuals

- **`RawIterate` is the one remaining surface where the old bug class can hide.** Debug-mode mutation counter in `TComponentStorage` (bumped by `Emplace`/`Remove`/`CleanUp`/`Reserve`) asserts if a caller holds a span across a mutation. Production builds skip the check. Cost: `uint32_t` increment per mutation.
- **Tombstone accumulation in long editor sessions** is a theoretical concern. No data-driven threshold needed today; `Compact()` is a one-pass future addition if ever required.
- **`Modify`'s lambda cannot be a coroutine / cannot await.** Fine for now — ECS access is synchronous in this engine. Documented as non-goal.
- **Preflight parse doubles the JSON memory cost briefly.** Scene files are tens of KB; negligible. If it ever bites, streaming preflight (count-only pass reading JSON character by character) is a drop-in replacement.

## Open Questions

None. All design questions resolved in 2026-04-20 brainstorming dialogue.

## References

- TASK-72 — EntityRegistry: component references invalidated by Emplace forces caller-side re-fetch
- TASK-109 — Scene reload regression; root cause shared with this design
- Commit `cfc1c8d1` — targeted patch of `JSONWrapper::LoadChildScene` (the triggering incident)
