# GPU Resource Handle Redesign — Design Spec

## Goal

Strip Component/Object inheritance from all 8 GPU resource handle types, move their pool
ownership into IRenderingServer, and delete ComponentManager.h once it has no remaining
registrants.

## Background

After ECS Phases 1–4 all gameplay and scene components migrated to EntityRegistry.
The only remaining ComponentManager consumers are 8 GPU resource handle types registered
by IRenderingServer: MeshComponent, TextureComponent, MaterialComponent, RenderPassComponent,
ShaderProgramComponent, SamplerComponent, GPUBufferComponent, CommandListComponent.

GPU handles are not entities. They don't need owner EntityIDs, UUIDs, serializability
flags, or lifespan tracking. Two of the eight (MeshComponent, MaterialComponent) are
already plain structs — the rest still inherit Component (via GPUResourceComponent) or
Component directly, which is accidental coupling that needs to be cut.

## Design — Approach A (Approved)

### Component header changes

**Six types lose their base class and become structs:**

| Type | Current base | After |
|------|-------------|-------|
| `TextureComponent` | `GPUResourceComponent` → `Component` | plain struct |
| `GPUBufferComponent` | `GPUResourceComponent` → `Component` | plain struct |
| `SamplerComponent` | `GPUResourceComponent` → `Component` | plain struct |
| `RenderPassComponent` | `Component` | plain struct |
| `ShaderProgramComponent` | `Component` | plain struct |
| `CommandListComponent` | `Component` | plain struct |

**Fields to strip** (from Object/Component base):
- `m_UUID` — no longer needed; material→texture cross-refs switch to asset path strings
- `m_Serializable`, `m_ObjectLifespan`, `m_Owner` — pool-lifecycle concerns, handled by IRenderingServer

**Fields to retain inline** (still used at call sites):
- `m_ObjectStatus` — checked before GPU operations (DX12, VK rendering paths)
- `m_InstanceName` — used in logging throughout

For the three GPUResourceComponent children (Texture, GPUBuffer, Sampler), their shared
GPUResourceComponent fields are retained through their base class relationship.
**GPUResourceComponent.h is kept** (not deleted) because `IRenderingServer::BindGPUResource`
takes a `GPUResourceComponent*` parameter — this polymorphic base is still needed for the
binding API. GPUResourceComponent loses its `Component` base and becomes a standalone struct,
gaining `m_ObjectStatus` and `m_InstanceName` inline (stripped from the old Object chain).
The three child types continue to inherit GPUResourceComponent and acquire these fields
transitively.

MaterialComponent already is a plain struct; it manually duplicates the GPUResourceComponent
fields. No change needed there except the `m_TextureComponents` field below.

**MaterialComponent.m_TextureComponents:**
- Before: `std::vector<uint64_t>` — UUID keys into ComponentManager's LUT
- After: `std::vector<std::string>` — asset path strings resolved through IRenderingServer's
  name LUT

### IRenderingServer pool ownership

IRenderingServer grows a private `GPUResourcePools` struct that owns:
- One `TObjectPool<T>*` per type (8 pools, sized by RenderingCapability limits)
- One `ThreadSafeUnorderedMap<std::string, T*>` per type for name-based lookup (8 LUTs)
- One `ThreadSafeVector<T*>` per type for `GetAll()` iteration (8 pointer lists)

`IRenderingServer::Setup()` creates these pools instead of calling
`ComponentManager::RegisterType<T>()`.

The `AddComponent<T>` free function in IRenderingServer.cpp is replaced by a template
method on `GPUResourcePools` that: allocates from the pool, sets `m_ObjectStatus = Created`,
sets `m_InstanceName`, inserts into the pointer list and name LUT, returns the pointer.

`Delete(T*)` drains the same three data structures.

`GetAll<T>()` returns the raw vector from the pointer list.

No EntityRegistry entities are spawned for GPU resource allocations — they aren't entities.

### Consumer updates

| Consumer | What changes |
|----------|-------------|
| `IRenderingServer.cpp` | Remove 8 `RegisterType` calls; replace `AddComponent<T>` with pool alloc |
| `JSONSerializer_Components.cpp` | `FindByUUID<TextureComponent>` → `FindTextureByPath(path)` via IRenderingServer LUT; read/write path strings instead of UUIDs |
| `TemplateAssetService.cpp` | `ComponentManager::Spawn<T>` → `IRenderingServer::AddXComponent(name)` |
| `worldexplorer.cpp` | Same: Spawn → AddXComponent; Destroy → Delete |
| `SceneService.cpp` | Remove `ComponentManager::CleanUp(ObjectLifespan::Scene)` call (was already a no-op for GPU resources; remove the ComponentManager include) |
| `DrawCallService.cpp` | Uncomment the dead `FindByUUID<TextureComponent>` if still relevant, or delete it |

### ComponentManager.h deletion

After all 8 GPU resource types are removed from ComponentManager, the service has no
registered types. ComponentManager.h and its ISystem registration in Engine.cpp are deleted.

## What does NOT change

- The 8 `AddXComponent()` public API methods on IRenderingServer — signatures unchanged
- All `Initialize()`, `Delete()`, `Execute()` etc. — signatures unchanged
- DX12/VK concrete implementations — they receive the same plain-struct pointers they
  always did; they already don't depend on the Component base
- MeshComponent, MaterialComponent — already plain structs; only MaterialComponent needs
  the `m_TextureComponents` field type change

## Testing

After each task: full rebuild + `Test.exe` pass + `RenderTest.exe draw_instanced` pass.
No new test cases are required — the existing draw_instanced test exercises the full
GPU resource allocation/initialization/execution/teardown pipeline.
