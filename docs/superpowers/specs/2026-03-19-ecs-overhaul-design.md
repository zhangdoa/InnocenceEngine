# ECS Overhaul — Architecture Design

**Status:** Approved 2026-03-19
**Branch:** `ecs-overhaul`
**Scope:** Full replacement of the component/entity/system layer with a principled data-oriented design

---

## Core Principles

**Entity** — an opaque `uint32_t` identifier. Nothing more. It groups components that belong together. It is not a game actor, not a scene node, not a class instance.

**Component** — a plain data struct (POD). No methods beyond default constructors. No engine API calls. No pointers to other components. Inter-entity relations use `EntityID`, not raw pointers.

**System** — a pure operation on one or more component pools. It reads and writes component data. It owns no component fields itself beyond what it needs to produce its outputs.

---

## Phase 1 — Foundation: EntityRegistry + ComponentStorage

### EntityID

```cpp
// Source/Engine/Common/EntityID.h
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;
constexpr uint32_t MAX_ENTITIES   = 65536;
```

Dense integer. No UUIDs for entity identity at runtime. The `uint64_t` UUID scheme on `Object` is replaced for entities. Components that currently use `uint64_t` UUID lookups (`FindByUUID`) are migrated to `EntityID` lookups.

---

### ComponentStorage\<T\>

The storage primitive. Replaces `TComponentFactory<T>` + `TObjectPool<T>` + `ThreadSafeVector` + `ThreadSafeUnorderedMap`.

```
Three arrays, always in sync:

m_dense[i]   — the component data at slot i
m_owners[i]  — which EntityID owns m_dense[i]
m_sparse[e]  — m_sparse[entityID] = dense index for that entity
               (UINT32_MAX = entity has no component of this type)
```

**API:**
```cpp
template<typename T>
class ComponentStorage
{
    void        Add(EntityID entity, const T& data = {});
    void        Remove(EntityID entity);
    T*          Get(EntityID entity);                    // O(1), null if absent
    bool        Has(EntityID entity) const;
    T&          GetOrAdd(EntityID entity);

    std::span<T>        All();                           // cache-friendly sequential scan
    std::span<EntityID> AllOwners();                     // parallel to All()

    void        CleanUp(ObjectLifespan lifespan);        // removes matching entries

    size_t      Size() const;
};
```

**Complexity:**
- `Add` / `Remove` — O(1) swap-and-pop
- `Get` / `Has` — O(1), one array read
- `All()` — linear scan of packed dense array, fully cache-friendly
- `m_sparse` is fixed-size `MAX_ENTITIES` — allocated once, no rehashing

**Memory footprint:** Each `ComponentStorage<T>` allocates `uint32_t m_sparse[MAX_ENTITIES]` = 256 KB per component type regardless of how many entities carry that component. With ~14 component types in the canonical list this is ~3.5 MB of sparse index arrays — an acceptable constant cost for O(1) lookup with no rehashing.

**Lifespan tracking:** Each slot in `m_dense` carries a parallel `m_lifespans[i]` array (`ObjectLifespan` enum). `CleanUp(lifespan)` removes all matching entries in one pass.

**Thread-safety contract:** `ComponentStorage<T>` is not thread-safe. All reads and writes occur on the main engine thread during the ECS update phase. Asset loading that previously called `TComponentFactory::Load` on a background thread is restructured: the background thread loads raw data into a staging buffer, then the main thread calls `EntityRegistry::Emplace<T>` during its next update cycle. The `ThreadSafeVector` / `ThreadSafeUnorderedMap` in the old `TComponentFactory` is eliminated — synchronization happens at the staging-buffer boundary, not inside component storage.

---

### EntityRegistry

Replaces both `ComponentManager` and `EntityManager`. Is an `ISystem` registered with the engine.

```cpp
class EntityRegistry : public ISystem
{
public:
    INNO_CLASS_CONCRETE_NON_COPYABLE(EntityRegistry);

    // ISystem
    bool Setup(ISystemConfig*) override;
    bool Initialize() override;
    bool Update() override;
    bool Terminate() override;
    ObjectStatus GetStatus() override;

    // Entity lifecycle
    EntityID    Spawn(ObjectLifespan lifespan, const char* name = nullptr);
    void        Destroy(EntityID entity);
    bool        IsValid(EntityID entity) const;
    const char* GetName(EntityID entity) const;
    EntityID    FindByName(const char* name) const;      // linear scan; editor/scene-load use only

    // Component operations
    template<typename T> T&   Emplace(EntityID entity, T data = {});
    template<typename T> void Remove(EntityID entity);
    template<typename T> T*   Get(EntityID entity);          // null if absent
    template<typename T> bool Has(EntityID entity) const;

    // Pool access — for systems that need bulk iteration
    template<typename T> ComponentStorage<T>& Storage();

    // Scene transition
    // Destroys all entities with matching lifespan, then removes all their components.
    // Components are removed first (before entity slots are freed) to prevent dangling EntityID reads.
    void CleanUp(ObjectLifespan lifespan);
};
```

**Entity metadata** (name, lifespan, validity) is stored in parallel flat arrays indexed by EntityID, not on the entity itself.

**`ComponentStorage<T>` registration** is implicit — `Storage<T>()` creates the storage on first call, no `RegisterType` boilerplate required.

**`FindByName`** performs a linear scan over the name array. It is only called from scene loading and editor code, never from hot paths.

**Migration from ComponentManager:**

| Old | New |
|-----|-----|
| `Get<ComponentManager>()->Spawn<T>(entity, ...)` | `Get<EntityRegistry>()->Emplace<T>(entityID)` |
| `Get<ComponentManager>()->Find<T>(entity)` | `Get<EntityRegistry>()->Get<T>(entityID)` |
| `Get<ComponentManager>()->FindByUUID<T>(uuid)` | `Get<EntityRegistry>()->Get<T>(entityID)` |
| `Get<ComponentManager>()->GetAll<T>()` | `Get<EntityRegistry>()->Storage<T>().All()` |
| `Get<ComponentManager>()->CleanUp(lifespan)` | `Get<EntityRegistry>()->CleanUp(lifespan)` |
| `Get<ComponentManager>()->Load<T>(path)` | `AssetService` loads asset data → main thread calls `Emplace<T>(entityID, data)` |
| `Get<EntityManager>()->Spawn(...)` | `Get<EntityRegistry>()->Spawn(lifespan, name)` |
| `Get<EntityManager>()->Find(name)` | `Get<EntityRegistry>()->FindByName(name)` |

**Asset loading migration detail:** `TComponentFactory<T>::Load` deduplicates by filename and caches the result. After migration, `AssetService` retains its own asset cache keyed by file path, but the live component data lives in `ComponentStorage<T>` rather than the factory pool. `AssetService::Load<T>` returns a populated `T` value; the caller is responsible for calling `EntityRegistry::Emplace<T>` on the appropriate entity.

---

## Phase 2 — Component POD Redesign

Every existing component struct is stripped to data only. No virtual methods, no engine API calls, no `m_ObjectStatus`, no `m_Serializable`, no `m_UUID` (lifecycle is managed by the registry, not the component).

**Phase 2 dependency note:** `LightComponent`'s CSM runtime fields (`m_ViewMatrices`, `m_ProjectionMatrices`, etc.) are not removed in Phase 2. They are removed in Phase 4 once `LightDataService` is rewritten to compute them internally. Phase 2 only strips the `Object` base class fields and engine API methods from `LightComponent`, leaving the CSM data fields in place.

### Transform (split into two)

```cpp
struct TransformComponent
{
    Vec3 m_localPosition = {0, 0, 0};
    Quat m_localRotation = {};           // identity
    Vec3 m_localScale    = {1, 1, 1};
};

struct WorldTransformComponent            // written only by TransformService
{
    Mat4 m_worldMatrix;
    Mat4 m_worldRotationMatrix;
};
```

### Canonical component list (target state)

| Component | Key data | Notes |
|-----------|----------|-------|
| `TransformComponent` | localPos, localRot, localScale | Gameplay-writable |
| `WorldTransformComponent` | worldMatrix, worldRotationMatrix | TransformService-owned output |
| `BoundingBoxComponent` | worldAABB | Derived from mesh AABB + world transform |
| `MeshComponent` | vertexBufferHandle, indexBufferHandle, indexCount, vertexStride, localAABB | GPU handles only |
| `MaterialComponent` | materialAttributes, textureIndices[N] | No texture pointers |
| `TextureComponent` | GPU handle, dimensions, format | |
| `LightComponent` | type, color, intensity, range, colorTemperature | CSM matrix fields remain until Phase 4 |
| `CameraComponent` | fov, zNear, zFar, aspectRatio, widthScale, heightScale, aperture, shutterTime, ISO, frustum, projectionMatrix, viewMatrix | frustum/projection/view written by CameraSystem; aperture/shutterTime/ISO feed per-frame exposure |
| `SkeletonComponent` | boneCount, boneOffsetMatrices[], m_meshEntity (EntityID of the entity owning the MeshComponent) | GPU upload methods removed; inter-entity link uses EntityID not pointer |
| `AnimationComponent` | clip + keyframe data | Playback logic removed |
| `AnimationStateComponent` | **New** — currentClip, playbackTime, looping, blendWeight | Written by AnimationSimulationService |
| `RigidBodyComponent` | **New** — mass, velocity, angularVelocity, restitution, isKinematic | |
| `CollisionShapeComponent` | **New** — shapeType, halfExtents/radius/meshHandle | |
| `VisibilityComponent` | **New** — visibilityMask flags | Replaces ObjectStatus on renderables |

**Deleted:**
- `ModelComponent` — dissolved; an entity with MeshComponent + MaterialComponent IS a renderable. Physics state (`m_SimulationProxy`, `m_CollisionPrimitives`) moves to `RigidBodyComponent` + `CollisionShapeComponent`.
- `DrawCallComponent` — same

**`SkeletonComponent.m_meshEntity`:** The current `SkeletonComponent` holds a raw `MeshComponent*`. This is replaced with `EntityID m_meshEntity` — the entity that owns the `MeshComponent` for this skeleton. Systems that need to correlate skeleton and mesh data look up `registry.Get<MeshComponent>(skeletonComp->m_meshEntity)`.

---

## Phase 3 — TransformService + HierarchyGraph

### HierarchyGraph

Stored inside `TransformService`. Not a component. An inter-entity relation owned by a system.

```cpp
struct HierarchyNode
{
    EntityID m_parent      = INVALID_ENTITY;
    EntityID m_firstChild  = INVALID_ENTITY;
    EntityID m_nextSibling = INVALID_ENTITY;
    uint32_t m_depth       = 0;
};
```

`HierarchyGraph` keeps a `ComponentStorage<HierarchyNode>` for O(1) entity→node lookup and a `m_traversalOrder` flat buffer (entity IDs sorted parents-before-children). The traversal buffer is rebuilt lazily (dirty flag) on any reparent operation.

**API:**
```cpp
void     SetParent(EntityID child, EntityID parent);
void     ClearParent(EntityID child);
EntityID GetParent(EntityID child) const;
EntityID GetFirstChild(EntityID parent) const;
EntityID GetNextSibling(EntityID entity) const;
// Child enumeration: walk firstChild → nextSibling → nextSibling until INVALID_ENTITY
```

`GetFirstChild` and `GetNextSibling` expose the linked-list structure for scene serialization and editor tree-view traversal. `SceneQueryService` uses these to reconstruct `SceneHierarchyMap` without `HierarchyGraph` needing to maintain a separate children-list index.

### TransformService::Update()

```
if (m_hierarchyGraph.IsDirty())
    m_hierarchyGraph.RebuildTraversalOrder();   // topological sort

for each entityID in m_traversalOrder (roots first):
    local = registry.Get<TransformComponent>(entityID)
    world = registry.Get<WorldTransformComponent>(entityID)
    parent = hierarchyGraph.GetParent(entityID)

    if parent == INVALID_ENTITY:
        world->m_worldMatrix = local->ToMatrix()
    else:
        parentWorld = registry.Get<WorldTransformComponent>(parent)
        world->m_worldMatrix = parentWorld->m_worldMatrix * local->ToMatrix()

    world->m_worldRotationMatrix = ExtractRotation(world->m_worldMatrix)
```

Cost: reparent = O(1) + dirty flag. Static scenes = zero sort cost after load. Dynamic scenes = one topological sort per frame that had hierarchy changes.

---

## Phase 4 — System Cleanup

### CSM consolidation
- `LightSystem` → `LightSimulationService`: keeps only color temperature conversion + attenuation radius derivation. All CSM matrix data deleted from `LightComponent` in this phase (not Phase 2).
- `CameraSystem`: removes `GenerateCSMSplitFactors` / `SplitVertices`. The cascade split vertices written to `CameraComponent` are removed — `LightDataService` computes CSM data directly from `LightComponent` + `CameraComponent` projection parameters.
- `LightDataService`: absorbs the full shadow cascade pipeline. `GetCSMBuffer()` continues to exist; the buffer is now populated by `LightDataService` internally rather than read from component fields.

### CullingService (extracted from PhysicsSimulationService)
- Inputs: `WorldTransformComponent` + `BoundingBoxComponent` pools, active camera frustum (read from `CameraComponent::m_frustum`, which `CameraSystem` keeps up to date)
- Output: `std::vector<EntityID>` visible list
- `PhysicsSimulationService` is left with only PhysX tick + force application
- `PhysicsSimulationService::AddForce` signature changes from `AddForce(ModelComponent*)` to `AddForce(EntityID entity, Vec3 force)` — it reads `RigidBodyComponent` from the registry internally

### BVHService migration
- `BVHService` currently stores `ModelComponent*` in every `BVHNode`. After Phase 2 deletes `ModelComponent`, `BVHNode` stores `EntityID` instead.
- BVH build reads `BoundingBoxComponent::m_worldAABB` (written by a pre-culling pass each frame) rather than `ModelComponent::m_MeshCustomMat4s`.
- This migration is part of Phase 5 (alongside `CullingService` extraction).

### AnimationSimulationService (split from AnimationService)
- Pure time-step simulation: advances `AnimationStateComponent` each frame
- `AnimationService` → `AnimationResourceService`: skeleton + animation GPU buffer allocation and upload only

### DrawCallService query change
- Currently iterates `ModelComponent` pool, reads `ModelComponent::m_Transform` (a `TTransform`)
- After Phase 2: iterates `EntityRegistry::Storage<MeshComponent>()`, cross-references `MaterialComponent` and `WorldTransformComponent` to build GPU draw calls
- `WorldTransformComponent::m_worldMatrix` (already a `Mat4`) is uploaded directly into the existing double-buffered `TransformConstantBuffer`. The double-buffer scheme and `GetCurrentFrameTransformBuffer()` / `GetPreviousFrameTransformBuffer()` / `GetMaterialBuffer()` output interface are preserved unchanged.

### SceneService
- `getSceneHierarchyMap()` extracted to `SceneQueryService` (editor-facing). `SceneQueryService` reconstructs the hierarchy map by walking `HierarchyGraph::GetFirstChild` / `GetNextSibling` on all root entities.
- Core `SceneService` keeps only load/save lifecycle

---

## Implementation Order

1. **EntityRegistry + ComponentStorage** — storage foundation, no behavior changes
2. **Component POD redesign** — strip behavior, delete ModelComponent/DrawCallComponent (LightComponent CSM fields stay)
3. **TransformService + HierarchyGraph** — new system, connect to GPU transform upload
4. **CSM consolidation** — clean up LightSystem + CameraSystem coupling; remove LightComponent CSM fields
5. **CullingService extraction + BVHService migration** — split from PhysicsSimulationService; BVHNode → EntityID
6. **AnimationSimulationService split** — clean AnimationService boundary

Each phase is independently buildable and GPU-gate verifiable.

---

## Serialization Contract

Scene files may change format. The visual contract is preserved: camera position/orientation, mesh geometry, material colors, and light placement must produce the same rendered output. Internal IDs, file layout, and field names are free to change.
