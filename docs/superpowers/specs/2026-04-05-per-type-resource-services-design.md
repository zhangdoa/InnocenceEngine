# Per-Type Resource Services

## Goal

Replace the `GraphicsResourceService` monolith with 8 per-type resource services. Each service owns one resource type end-to-end: pool, name index, deferred initialization, and backend-specific GPU work. A new `NamedObjectPool<T>` replaces the current triple of TObjectPool + ThreadSafeUnorderedMap + ThreadSafeVector.

## Motivation

`GraphicsResourceService` is a 3500-line blob across 3 files. It has 8 copies of Add/Delete/Initialize for different component types, each routed through `InitializeImpl` virtual overloads. The GPUHandlePools struct maintains parallel LUT and pointer-vector containers for every type, most of which are never queried. Splitting by resource type gives:

1. No blob files — each service is focused and small
2. No N-way `InitializeImpl` overloads — each service has one
3. Unified interface — every service follows the same shape
4. No wasted container bookkeeping — NamedObjectPool provides what's needed, nothing more

## Architecture

### NamedObjectPool\<T\>

Wraps `TObjectPool<T>` with name-indexed allocation and live-object tracking.

```
template <typename T>
class NamedObjectPool {
    TObjectPool<T>* m_Pool;
    ThreadSafeUnorderedMap<std::string, T*> m_NameIndex;
    ThreadSafeVector<T*> m_LiveObjects;

    T* Allocate(const char* name);    // dedup by name, track
    void Release(T* ptr);             // remove from index, return to pool
    T* Find(const char* name);        // name lookup
    void ForEach(std::function<void(T*)>);  // iterate live objects
};
```

Replaces the current per-type pool + LUT + pointer vector. Every service gets the same capabilities; unused ones (e.g. name lookup on Sampler) are zero-cost — just never called.

### Service shape

Every per-type service follows the same pattern:

```
class FooResourceService : public IService {
public:
    bool Setup(IServiceConfig*) override;   // create pool
    bool Initialize() override;
    bool Update() override { return true; }
    bool Terminate() override;              // destroy pool

    FooComponent* Add(const char* name);
    virtual bool Delete(FooComponent* ptr);
    FooComponent* Find(const char* name);
    void ForEach(std::function<void(FooComponent*)>);

    // For types with deferred init:
    bool InitializeComponents();

protected:
    virtual bool InitializeImpl(FooComponent*, ...) { return false; }

    NamedObjectPool<FooComponent> m_Pool;
    // Optional: ThreadSafeQueue<InitTask> m_DeferredQueue;
};
```

DX12 backend:

```
class DX12FooResourceService : public FooResourceService {
protected:
    bool InitializeImpl(FooComponent*, ...) override;
    // DX12-specific resource storage
    DX12Context* m_ctx;
};
```

### Service list

| Service | Deferred init | Notes |
|---------|--------------|-------|
| MeshResourceService | Yes | Owns GPUMeshResource slots, MeshAsset allocation |
| TextureResourceService | Yes | Owns upload/default heaps, SRV/UAV, mipmaps |
| MaterialResourceService | Yes | References textures, owns material dedup |
| GPUBufferResourceService | Yes | Owns mapped memory, upload, CBV/SRV/UAV |
| RenderPassResourceService | Yes | Owns output merger targets, PSO, semaphores, fences |
| ShaderProgramResourceService | No | Sync init (shader compilation) |
| SamplerResourceService | No | Sync init |
| CommandListResourceService | No | Sync init |

### Cross-service dependencies

- **RenderPassResourceService** calls `TextureResourceService::Add()` for render targets, uses ShaderProgram/Sampler services during setup. Gets sibling services via `g_Engine->Get<>()`.
- **MaterialResourceService** references textures but does not own them.
- **FrameManagementService** calls each service's `InitializeComponents()` per frame (it is the coordinator).
- **Engine.cpp** creates all 8 services (+ DX12 impls), registers them in the singleton map.

### What gets eliminated

- `GraphicsResourceService` class — removed entirely
- `GPUHandlePools` struct — replaced by per-service NamedObjectPool
- `AllocateGPUHandle` / `ReleaseFromPoolStatic` templates — replaced by NamedObjectPool::Allocate/Release
- All `InitializeImpl` overloads on one class — each service has exactly one
- `InitializeComponents()` mega-drain — each service drains its own queue

### DX12-specific types

PSO, Semaphore, and OutputMergerTarget pools currently live on `DX12GraphicsResourceService`. After the split:

- **PSO pool** → `DX12RenderPassResourceService` (only render passes create PSOs)
- **Semaphore pool** → `DX12RenderPassResourceService` (render passes own semaphores) + `DX12GraphicsHardwareService` creates the global semaphore directly
- **OutputMergerTarget pool** → `DX12RenderPassResourceService` (output merger is part of render pass setup)

### Scene unloading

Each service with a deferred queue handles its own cleanup in an `OnSceneUnloading()` method. The coordinator (FrameManagementService or a scene lifecycle hook) calls each service's unloading method.

### Raytracing resources

TLAS buffer, scratch buffer, and raytracing instance buffer move to a dedicated concern — either `DX12RenderPassResourceService` or a future raytracing service. Not part of this refactor's scope.

### Migration path

Callers currently do `g_Engine->Get<GraphicsResourceService>()->AddTextureComponent(name)`. After: `g_Engine->Get<TextureResourceService>()->Add(name)`. The 34 render pass files and ThirdParty wrappers need mechanical updates.
