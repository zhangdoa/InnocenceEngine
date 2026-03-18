# RenderingContextService Split — Design Spec

**Date:** 2026-03-18
**Branch:** ecs-overhaul
**Status:** Approved

## Problem

`RenderingContextService` is ~29K lines and does at least six unrelated things: per-frame camera/jitter data, light CB packing, draw call assembly, animation draw call assembly, billboard draw call assembly, and debug draw call assembly. It owns 12 GPU buffers spanning four unrelated data domains.

The root cause is the same as every blob: there was no enforced boundary. Each new GPU data type got appended to the same impl struct.

## Principle

A service owns and manipulates data from one component type (or one coherent GPU data domain). It reads component data directly via `ComponentManager`, packs into CPU-side vectors, and uploads to its own GPU buffers. It does not read another service's processed output.

---

## Proposed Services

### 1. `PerFrameDataService`

**Owns:** camera matrices, TAA jitter (Halton), sun direction/illuminance, active CSM cascade index, viewport params
**GPU buffers:** `PerFrameCBuffer` (ping-pong × 2)
**Reads:** `CameraComponent` (active camera), `LightComponent[0]` (sun direction/illuminance only, read directly), `RenderingConfigurationService`, frame count from `IRenderingServer`
**Note on cascade index:** `activeCascade` is a rolling counter cycling `0..maxCSMSplits-1`. It is computed from `RenderingCapability.maxCSMSplits` — no dependency on `LightDataService`.
**Public API:**
```cpp
const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
GPUBufferComponent* GetCurrentFrameBuffer();
GPUBufferComponent* GetPreviousFrameBuffer();
```
**Estimated size:** ~200 lines

---

### 2. `LightDataService`

**Owns:** point/sphere light CB packing, CSM matrix packing
**GPU buffers:** `PointLightCBuffer`, `SphereLightCBuffer`, `CSMCBuffer`, `GICBuffer`
**Reads:** `LightComponent` (all, via `ComponentManager::GetAll<LightComponent>()`)
**Note on CSM matrices:** `LightSystem::Update()` computes and stores CSM view/projection matrices directly on `LightComponent` (`m_ViewMatrices`, `m_ProjectionMatrices`, `m_LitRegion_WorldSpace`). `LightDataService` reads these fields from the component — no `CameraSystem` access.
**Note on GICBuffer:** Currently a stub (`sizeof(GIConstantBuffer)`, element count 1, never written). Allocated and initialized here for forward compatibility. Written by a future GI system.
**Public API:**
```cpp
GPUBufferComponent* GetPointLightBuffer();
GPUBufferComponent* GetSphereLightBuffer();
GPUBufferComponent* GetCSMBuffer();
GPUBufferComponent* GetGIBuffer();
```
**Estimated size:** ~250 lines

---

### 3. `DrawCallService`

**Owns:** visible mesh/material/transform packing
**GPU buffers:** `GPUModelDataBuffer`, `TransformBuffer` (ping-pong × 2), `MaterialCBuffer`
**Reads:** `ComponentManager` (ModelComponent, DrawCallComponent, MeshComponent, MaterialComponent, TextureComponent)
**Note:** Currently reads all models without culling filter (culling integration is a future step). The `PhysicsSimulationService` culling results can be plumbed in later without changing this service's API.
**Public API:**
```cpp
const std::vector<GPUModelData>& GetGPUModelData();
GPUBufferComponent* GetGPUModelDataBuffer();
GPUBufferComponent* GetCurrentFrameTransformBuffer();
GPUBufferComponent* GetPreviousFrameTransformBuffer();
GPUBufferComponent* GetMaterialBuffer();
```
**Estimated size:** ~350 lines

---

### 4. `AnimationDrawCallService`

**Owns:** animation instance → draw call assembly
**GPU buffers:** `AnimationCBuffer`
**Reads:** `AnimationService` (animation instances), `ComponentManager` (AnimationComponent)
**Note:** The animation draw call path is currently commented out in `UpdateDrawCalls()`. This service implements it cleanly as a first-class path.
**Public API:**
```cpp
const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();
GPUBufferComponent* GetAnimationBuffer();
```
**Estimated size:** ~200 lines

---

### 5. `BillboardDrawCallService`

**Owns:** editor icon billboard draw calls (one instance group per light type)
**GPU buffers:** `BillboardCBuffer`
**Reads:** `TemplateAssetService` (icon textures, fetched once on scene load), `ComponentManager` (LightComponent, for per-light transform)
**Public API:**
```cpp
const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
GPUBufferComponent* GetBillboardBuffer();
```
**Estimated size:** ~150 lines

---

### 6. `DebugDrawCallService`

**Owns:** debug geometry draw call list for the current frame
**GPU buffers:** none (CPU-side only)
**Clear contract:** The list is cleared at the start of each `Update()`. Any shape submitted in frame N is visible for exactly frame N. Callers re-submit every frame if they want persistent shapes. No persistent shape registry.
**Public API:**
```cpp
const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
void Submit(const DebugPassDrawCallInfo&);
```
**Estimated size:** ~100 lines

---

## What Goes Away

- `RenderingContextService` class (deleted after all six are extracted and callers updated)
- `GPUBufferUsageType` enum (replaced by typed getters on each owning service)
- The 29K-line impl struct

---

## Dependency and Update Order

The six services have no cross-service data dependencies in `Update()` — each reads component data directly. They do depend on upstream systems having already run for the current frame:

**Required update order:**
1. `LightSystem::Update()` — writes CSM matrices into `LightComponent`
2. `PhysicsSimulationService::Update()` — produces culling results (future use by DrawCallService)
3. **Any order:** `PerFrameDataService`, `LightDataService`, `DrawCallService`, `AnimationDrawCallService`, `BillboardDrawCallService`, `DebugDrawCallService`
4. Render passes (DefaultClient) — reads from all six services

This is identical to the constraint on the old `RenderingContextService`. No new ordering requirements are introduced.

---

## Migration Strategy

Extract one service at a time. Each extraction is atomic:
1. Create the new service file
2. Move the relevant data members and update functions out of `RenderingContextService`
3. Update all callers (render passes in `DefaultClient`) to use the new service in the same commit
4. Delete the moved code from `RenderingContextService`
5. Build + run `RenderTest.exe draw_instanced` — must pass before the next extraction starts

`RenderingContextService` shrinks with each step. When it reaches zero, delete it and its registration. No forwarding adapters, no dual-call transitional state — each step leaves the codebase in a clean, working state.

**Extraction order** (dependencies first):
1. `PerFrameDataService`
2. `LightDataService`
3. `DrawCallService`
4. `AnimationDrawCallService`
5. `BillboardDrawCallService`
6. `DebugDrawCallService`
