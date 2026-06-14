# Systematic reinterpret_cast elimination (engine-wide)

## Status
Phase 1 (rendering, resource hierarchy) DONE — commit 1bb7d378.
Remaining phases open.

## Why
reinterpret_cast for polymorphism is the lazy practice that caused the
LightCulling transition crash (commit 56545d84): it casts on a caller's
claimed type, compiles unconditionally, and corrupts memory silently when
the claim is wrong. Replace with checked downcasts that validate a runtime
discriminator.

## Taxonomy (full inventory ~120 sites, 50 files)

### Class A — resource-hierarchy downcast — DONE (phase 1)
`GPUResourceComponent*` -> `Texture/Buffer/Sampler`, decided by caller's
declared binding type. HIGH risk. Fixed via `GPUResourceComponent::As<T>()`
/ `TryAs<T>()` (validates m_GPUResourceType, static_cast). 10 sites.

### Class B — backend component downcast — OPEN (phase 2, rendering)
`TextureComponent*`->`VKTextureComponent*`, `OutputMergerComponent*`->
`DX12OutputMergerTarget*`, `PipelineStateObject*`->`DX12PipelineStateObject*`,
`ISemaphore*`->`DX12Semaphore*`/`VKSemaphore*`, `IDeviceMemory*`->
`DX12DeviceMemory*`, `IMappedMemory*`->`DX12MappedMemory*`,
`IRaytracingInstanceDescList*`->`DX12RaytracingInstanceDescList*`.
~60 sites across DX12* and VK*. Type-safe by construction today (one backend
builds them), but fragile. Approach: a backend cast helper with a debug-only
type tag, or make the base interfaces carry a discriminator like
GPUResourceComponent does. Decide per-hierarchy; VK and DX12 are independent.

### Class C — pointer<->uint64 handle — OPEN (phase 3)
`CommandListComponent::m_CommandList` is a uint64_t holding a raw
ID3D12GraphicsCommandList* / VkCommandBuffer. ~15 sites pun in and out.
Approach: a typed handle wrapper, or store the concrete pointer behind an
accessor. Lower priority — deliberate type erasure, no silent-corruption
path (the int is only ever the one backend's handle).

### Class D — void* boundary — LEAVE (legitimate)
OS/config/bridge hooks, PhysX userData, GetProcAddress fn-ptrs, HID event
handle. Genuine void* crossings; reinterpret_cast is correct.

### Class E — allocator / pool raw memory — LEAVE (correct)
ObjectPool / Allocator byte casts + placement-new. Low-level, unavoidable.

### Class F — third-party glue — LEAVE
ImGui device/context, Vulkan SPIR-V `const uint32_t*`, PIX. Appropriate.

## Acceptance per phase
Build clean + TestGIScene 64 frames + 0 D3D12 errors, behavior identical.
One commit per phase (or per backend within phase 2).
