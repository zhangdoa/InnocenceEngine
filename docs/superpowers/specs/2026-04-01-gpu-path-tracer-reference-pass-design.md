# GPU Path Tracer Reference Pass Design

## Goal

Add `GPUPathTracerPass`: a GPU progressive path tracer that accumulates samples over time and drives the swap chain output when active (B key). Serves as a realtime ground-truth reference to validate the probe-based `RadianceCacheRaytracingPass`.

Also removes the offline `GIResolvePass` (and its B key binding) which is superseded.

---

## Architecture

### New Files

| File | Purpose |
|------|---------|
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h` | Pass class declaration |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` | Pass C++ implementation |
| `Res/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | Iterative path loop (no DXR recursion) |
| `Res/Shaders/HLSL/GPUPathTracerClosestHit.hlsl` | Surface data return via payload |
| `Res/Shaders/HLSL/GPUPathTracerShadowClosestHit.hlsl` | Shadow hit: sets `isShadowed = true` |
| `Res/Shaders/HLSL/GPUPathTracerMiss.hlsl` | Sky radiance return |
| `Res/Shaders/HLSL/GPUPathTracerShadowMiss.hlsl` | Shadow miss: sets `isShadowed = false` |

### Removed Files

| File | Reason |
|------|--------|
| `Source/DefaultClient/RenderingClient/GIResolvePass.h` | Offline GI bake superseded |
| `Source/DefaultClient/RenderingClient/GIResolvePass.cpp` | Owns B key + scene callbacks — removal frees both |

CMakeLists uses `file(GLOB *.cpp)` so no build file edits are needed.

---

## Pass Ownership

`GPUPathTracerPass` owns:

- **`m_AccumulationBuffer`** — HDR `float4` UAV texture at screen resolution. Running average of path-traced radiance.
- **`m_ToneMapOutput`** — LDR `float4` output texture at screen resolution. Written by the tonemap dispatch; assigned to `m_Canvas` when the pass is active.
- **`m_FrameCountCB`** — 1-element `GPUBufferComponent` holding `uint frameCount`. Incremented each frame; reset to 1 on camera movement.
- **`m_MaterialDataBuffer`** — Structured buffer, `maxMeshes` elements of `PathTracerMaterialData { float3 albedo; float metalness; float roughness; }`. Populated from `DrawCallService::GetGPUModelData()` each frame by reading the material index into the existing per-material constant data.
- **`m_PrevViewMatrix`** — CPU-side `Mat4`. Compared against `PerFrameCB.v` each frame to detect camera movement.

---

## DXR Pipeline

### Shader Table Layout

Two hit group types, N instances total:

```
[0 .. N-1]     — Primary hit records: GPUPathTracerClosestHit
                  Local root signature: { VertexBufferSRV, IndexBufferSRV }

[N .. 2N-1]    — Shadow hit records: GPUPathTracerShadowClosestHit
                  No local root signature data needed

Miss[0]        — GPUPathTracerMiss       (sky color)
Miss[1]        — GPUPathTracerShadowMiss (isShadowed = false)
```

`InstanceContributionToHitGroupIndex` = sequential instance index (0, 1, …, N-1), so each instance maps to its own primary hit record carrying its vertex/index buffer SRVs.

Shadow rays are traced with `MissShaderIndex = 1` and hit group offset `N + instanceIndex`, pointing to the shadow hit group records.

### Payload Types

```hlsl
struct PathTracerPayload {
    float3 hitPos;      // world space
    float3 normal;      // interpolated vertex normal, world space
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;      // true when miss shader fired
};  // 44 bytes

struct ShadowPayload {
    bool isShadowed;
};  // 4 bytes
```

Max payload size: 48 bytes. Max attribute size: 8 bytes (barycentrics). Max recursion depth: 1.

---

## Algorithm (Ray Generation Shader)

Iterative Monte Carlo path tracer matching the CPU `RayTracer` algorithm:

```
for each pixel in dispatch:
    jitter = Halton2D(frameCount, pixel)
    ray = GenerateCameraRay(PerFrameCB, pixel + jitter)
    throughput = (1,1,1);  radiance = (0,0,0)

    for bounce in [0 .. 40]:
        payload = TraceRay(TLAS, ray, MissIndex=0, HitOffset=0)

        if payload.missed:
            radiance += throughput * SkyColor(ray.dir)
            break

        // NEE: direct sun
        shadowPayload = TraceRay(TLAS, ray from payload.hitPos toward sun_dir,
                                 Flags=ACCEPT_FIRST_HIT, MissIndex=1, HitOffset=N)
        if !shadowPayload.isShadowed:
            brdf = CookTorranceGGX(payload, -ray.dir, sun_dir)
            radiance += throughput * brdf * PerFrameCB.sun_illuminance

        // Sample next direction
        [next_dir, pdf] = ImportanceSampleGGX(payload, -ray.dir, Rand2())
        throughput *= CookTorranceGGX(payload, -ray.dir, next_dir)
                    * saturate(dot(payload.normal, next_dir)) / pdf

        // Russian Roulette after bounce 3
        if bounce >= 3:
            q = clamp(Luminance(throughput), 0.05, 0.95)
            if Rand() > q: break
            throughput /= q

        ray = SpawnRay(payload.hitPos, payload.normal, next_dir)

    // Running average accumulation
    accumBuffer[pixel] = lerp(accumBuffer[pixel], radiance, 1.0 / float(frameCount))
```

`CookTorranceGGX` uses the same GGX NDF, Smith-G2, and Schlick-Fresnel as the CPU tracer. `ImportanceSampleGGX` uses GGX-importance-sampled half-vector. Diffuse component uses cosine-weighted hemisphere sampling, split by Fresnel weight (energy conservation).

### Sky Model

Simple gradient: `lerp(float3(0.1, 0.15, 0.2), float3(0.5, 0.7, 1.0), saturate(ray.dir.y))`, matching the CPU tracer's background.

---

## Closest-Hit Shader

Uses local root signature bindings (set per shader table record):
- `ByteAddressBuffer VertexBuffer` — mesh vertex data (stride = `sizeof(Vertex)`)
- `ByteAddressBuffer IndexBuffer`  — mesh index data (stride = 4 bytes, uint32)

Interpolation:
```hlsl
uint3 indices = LoadTriangleIndices(IndexBuffer, PrimitiveIndex());
Vertex v0 = LoadVertex(VertexBuffer, indices.x);
Vertex v1 = LoadVertex(VertexBuffer, indices.y);
Vertex v2 = LoadVertex(VertexBuffer, indices.z);
float2 bary = BuiltInTriangleIntersectionAttributes.barycentrics;
float3 localNormal = v0.normal * (1 - bary.x - bary.y)
                   + v1.normal * bary.x
                   + v2.normal * bary.y;
payload.normal = normalize(mul((float3x3)ObjectToWorld3x4(), localNormal));
payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
```

Material lookup: `PathTracerMaterialData mat = in_MaterialDataBuffer[InstanceID()]`. `InstanceID()` equals the sequential draw index set during TLAS build (changed from EntityID to draw-order index to match the material buffer layout).

---

## Accumulation and Reset

Each frame before dispatch:
1. Read `PerFrameCB.v` (view matrix).
2. If `v != m_PrevViewMatrix`: upload `frameCount = 1`, dispatch a clear of `m_AccumulationBuffer` to zero.
   Else: upload `frameCount++`.
3. Store `m_PrevViewMatrix = v`.

---

## Tonemap Pass

After the path tracer dispatch, a small compute shader reads `m_AccumulationBuffer` and writes `m_ToneMapOutput`:

```hlsl
float3 hdr = accumBuffer[pixel].rgb;
float3 exposed = hdr * ComputeAutoExposure(accumBuffer);  // luminance average via parallel reduction
float3 tonemapped = ACESFilmic(exposed);
float3 srgb = LinearToSRGB(tonemapped);
output[pixel] = float4(srgb, 1);
```

`ComputeAutoExposure` uses a one-frame-behind exposure value stored in a 1-element buffer (updated by a separate reduce pass), matching the CPU tracer's luminance-averaging exposure.

---

## Integration with DefaultRenderingClient

`DefaultRenderingClient` adds:
- `bool m_GPUPathTracerActive = false`

`Setup()`: call `GPUPathTracerPass::Get().Setup()`
`Initialize()`: call `GPUPathTracerPass::Get().Initialize()`
`Update()`: call `GPUPathTracerPass::Get().Update()` (uploads material buffer, frame count CB)

`ExecuteCommands()`:
```cpp
if (m_GPUPathTracerActive) {
    // Skip radiance cache pipeline
    GPUPathTracerPass::Get().Execute();
    m_Canvas = GPUPathTracerPass::Get().GetResult();
} else {
    // Normal pipeline (radiance cache, TAA, final blend)
    ...
    m_Canvas = FinalBlendPass::Get().GetResult();
}
```

**Key binding** in `WorldSystem::Setup()`:
```cpp
f_toggleGPUPathTracer = [&]() {
    DefaultRenderingClient* rc = ...;
    rc->ToggleGPUPathTracer();  // flips m_GPUPathTracerActive, resets accumulation
};
g_Engine->Get<HIDService>()->AddButtonStateCallback(
    ButtonState{ INNO_KEY_B, true },
    ButtonEvent{ EventLifeTime::OneShot, &f_toggleGPUPathTracer });
```

**Scene reload callback**: `GPUPathTracerPass` registers an `AddSceneLoadedCallback` that resets `frameCount` and rebuilds the material buffer, so a new scene accumulates cleanly from frame 1.

---

## TLAS Instance ID Change

`DX12GraphicsService_EngineComponent_Protected.cpp` currently sets `instanceDesc.InstanceID = EntityID`. The path tracer's closest-hit shader needs `InstanceID()` to index `m_MaterialDataBuffer` which is ordered by draw call index (0, 1, 2, …).

Change: set `instanceDesc.InstanceID = drawCallIndex` (sequential index matching GPUModelData buffer order). No existing shader reads `InstanceID()`, so this is safe.

---

## Testing

1. `draw_instanced` GPU validation test must still pass after `GIResolvePass` removal.
2. Manual: press B, let accumulate 128+ frames on `UnitTest.InnoScene`, compare against `cpu_reference.png` produced by the CPU path tracer (N key). Lighting structure, shadow shapes, and indirect bounce colour should match within Monte Carlo noise.
3. Press B again to return to normal pipeline; radiance cache pass resumes correctly.
