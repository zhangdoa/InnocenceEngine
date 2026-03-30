# GI Validation with CPU Path Tracer Reference

**Date:** 2026-03-30
**Branch:** ecs-overhaul
**Status:** Approved for implementation

## Goal

Establish an automated, regression-capable GI quality test that compares the GPU renderer's output against a CPU Monte Carlo path tracer reference render of the same scene and camera. The test runs headlessly via `TestGIScene.ps1` and exits 0 (pass) or 1 (fail).

## Context

- `TestGIScene.ps1` already runs `Main.exe -frames N`, loads `GITestBox.InnoScene`, and checks for D3D12 errors and clean shutdown.
- `GITestBox.InnoScene` has 17 entities (buildings, walls, ceiling, lights, camera) using `UnitCubeMesh` / `UnitSquareMesh` geometry and custom PBR materials.
- `RayTracer.cpp` (448 lines) contains a working recursive Monte Carlo path tracer with Lambertian/Metal materials, 8 SPP, and pinhole+DoF camera — but uses hardcoded sphere geometry and does not read the live scene.
- `BVHService` builds an entity-level AABB BVH from live scene geometry but is not wired to the ray tracer.
- `FinalBlendPass` does **not** have any existing CPU readback path; GPU frame capture must be built from scratch.
- `ITask::Wait()` is the correct mechanism for blocking until a submitted task completes (`Task.h:73`).

## Architecture

```
TestGIScene.ps1
  └─ Main.exe -frames N
       ├─ [Fix]  LoadScene: MeshComponent GPU buffer fix (root cause TBD, see §1)
       ├─        WorldSystem renders N frames of GITestBox
       ├─ [New]  Frame N-1: GPU readback → Bin/RelWithDebInfo/gpu_output.png
       ├─ [New]  Shutdown: CPU path tracer → Bin/RelWithDebInfo/cpu_reference.png
       └─ exit
  └─ magick compare gpu_output.png cpu_reference.png
       ├─ mean absolute error (MAE) logged
       ├─ NaN/Inf check
       └─ PASS if MAE < threshold, else FAIL
```

Four workstreams, each independently buildable, executed strictly in order:

1. **Mesh GPU buffer fix** — prerequisite; everything else blocked on geometry being visible
2. **GPU frame capture** — new readback infrastructure
3. **CPU path tracer scene integration** — wire to live scene, fix cube normals
4. **TestGIScene.ps1 comparison** — wires (2) and (3) together

## Components

### 1. Mesh GPU Buffer Fix

**Background:** `JSONWrapper::Load(fileName, MeshComponent&)` (JSONSerializer_Components.cpp:126–171) already performs a full struct copy from the template: `component = *TemplateAssetService::GetMeshComponent(meshShape)`. The comment in `JSONWrapper::LoadScene` — "restore after template copy" — confirms the copy was intentional.

**Problem:** GITestBox entities still exhibit zero vertex stride after scene load. The copy occurs but the resulting buffer views are null or zero.

**Probable root cause:** `TemplateAssetService` initializes its mesh templates on a task thread. If `LoadScene` runs before that thread completes GPU initialization of the templates, the struct copy succeeds but copies null GPU buffer views.

**Diagnostic step (required before fix):** In `JSONWrapper::Load(MeshComponent)`, immediately after line 135, log `component.m_VertexBufferView.m_StrideSize` and `component.m_IndexBufferView.m_Count`. If both are zero, confirm the race hypothesis by checking whether delaying scene load (waiting on TemplateAssetService status) resolves it.

**Fix strategy (conditional on diagnostic):**
- If race: In `WorldSystem::Setup()` or wherever scene load is triggered, spin-wait or add a dependency on `TemplateAssetService` reaching `ObjectStatus::Activated` before calling `SceneService::Load`.
- If not race: Investigate whether `EntityRegistry::Emplace<MeshComponent>` re-zeros the component after the copy and whether the copy is even being reached.

**Acceptance:** All GITestBox mesh entities have non-zero vertex stride; buildings and walls render visible geometry.

---

### 2. GPU Frame Capture

**No existing readback path exists in `FinalBlendPass`.** This is new infrastructure.

**Mechanism:**
1. Allocate a CPU-visible D3D12 readback heap buffer sized for the full render target during `FinalBlendPass::Initialize()` (only when `g_Engine->getInitConfig().maxFrames > 0`).
2. On the designated capture frame, issue `CopyTextureRegion` from the `FinalBlendPass` result texture into the readback buffer on the graphics command list, immediately after the final blend draw.
3. After `ExecuteCommandLists` completes and the per-frame fence is signaled, map the readback buffer, read the pixel data, and write `Bin/RelWithDebInfo/gpu_output.png` via `stb_image_write_png`.
4. Unmap the readback buffer (do not release; it is reused or freed in `Terminate`).

**D3D12 row-pitch alignment:** `CopyTextureRegion` requires the destination row pitch to be aligned to `D3D12_TEXTURE_DATA_PITCH_ALIGNMENT` (256 bytes). The mapped buffer row stride will be `ALIGN_UP(width * 4, 256)`, which is wider than `width * 4` for most resolutions. Pass this padded stride as the `stride_in_bytes` argument to `stb_image_write_png`. Failure to account for this produces a sheared image.

**Trigger:** `FinalBlendPass` maintains its own `m_CaptureFrameIndex` counter (incremented each `Execute()` call when `maxFrames > 0`). It self-triggers when `m_CaptureFrameIndex == maxFrames - 1`. This keeps all capture logic inside `FinalBlendPass` with no cross-layer flag from `WorldSystem`.

**Ownership:** The readback buffer, frame counter, and PNG write logic are all members of `FinalBlendPassNS` (the file-scope namespace used by `FinalBlendPass.cpp`, following the engine's namespace-as-PIMPL pattern).

**Error contract:** If readback or write fails, log an error and write a 1×1 black PNG so the downstream comparison fails loudly rather than silently skipping.

**Acceptance:** `gpu_output.png` appears in `Bin/RelWithDebInfo/` after test run and contains a non-trivial image (not all black).

---

### 3. CPU Path Tracer Scene Integration

Extends `RayTracer.cpp`. Preserves the existing Monte Carlo structure. Three sub-tasks:

#### 3a. Fix HitableCube face normals

**Current bug:** `hitResult.HitNormal = hitResult.HitPoint - m_AABB.m_center` (lines 149, 154) produces a diagonal vector from the box center to the hit point — not a face normal. This corrupts Lambertian scattering on all cube faces, making indirect lighting meaningless.

**Fix:** Track the winning axis index during the `tmin` computation — do not recover it afterward by floating-point equality comparison (inexact). Replace the three-step `std::max` chain with an explicit comparison that records which axis produced `tmin`:

```cpp
float tXmin = std::min(t1, t2);   // X-slab entry
float tYmin = std::min(t3, t4);   // Y-slab entry
float tZmin = std::min(t5, t6);   // Z-slab entry

int   axis   = 0;
float tmin_v = tXmin;
if (tYmin > tmin_v) { tmin_v = tYmin; axis = 1; }
if (tZmin > tmin_v) { tmin_v = tZmin; axis = 2; }

// face normal: unit vector along `axis`, pointing away from the ray's approach
InnoMath::TVec4<float> normal = {};
normal[axis] = (r.m_direction[axis] < 0.0f) ? 1.0f : -1.0f;
hitResult.HitNormal = normal;
```

For the **exterior hit** (`tmin_v >= 0`): use the axis-tracking pseudocode above — the face hit is the one that produced `tmin_v`.

For the **interior hit** (`tmin_v < 0`): the hit face is the one that produced `tmax`. Track it with a parallel index computation:

```cpp
float tXmax = std::max(t1, t2);   // X-slab exit
float tYmax = std::max(t3, t4);   // Y-slab exit
float tZmax = std::max(t5, t6);   // Z-slab exit

int   axis_max   = 0;
float tmax_v = tXmax;
if (tYmax < tmax_v) { tmax_v = tYmax; axis_max = 1; }
if (tZmax < tmax_v) { tmax_v = tZmax; axis_max = 2; }

// interior normal: points inward (away from ray direction)
InnoMath::TVec4<float> normal_interior = {};
normal_interior[axis_max] = (r.m_direction[axis_max] < 0.0f) ? 1.0f : -1.0f;
```

Use `axis_max` / `normal_interior` for the `tmin_v < 0` branch (lines 146–150 of the original code).

#### 3b. Scene population

Replace the hardcoded sphere/cube scene in `ExecuteRayTracing()` with a loop over `EntityRegistry`:

- Collect all entities that have both `MeshComponent` (with non-null buffer views) and `TransformComponent`.
- For each, construct a `HitableBox` (axis-aligned AABB in world space) from the entity's `m_AABB` scaled by `m_LocalScale` and translated by `m_LocalPos`. Rotation is approximated by expanding the AABB to bound the oriented box (acceptable for indirect diffuse GI comparison at 8 SPP).
- Map `MaterialComponent::m_materialAttributes` to `Lambertian` (roughness > 0.5) or `Metal` (roughness <= 0.5). Map emissive materials (luminance > 0) to a new `Emissive` hitable type that returns the emission color directly from `Scatter`.
- Add all `HitableBox` instances to a `HitableList` (the existing aggregate type in `RayTracer.cpp`) for linear-scan ray traversal. Do **not** use the engine's `BVHNode` type — it is an entity-level spatial index for the engine's culling pipeline, unrelated to the path tracer's `Hitable` hierarchy. A `HitableList` linear scan over 17 GITestBox entities at 8 SPP is adequate; a path-tracer-local BVH can be added later if needed.

#### 3c. Camera

Read from the entity in `EntityRegistry` that has a `CameraComponent`:
- FOV from `CameraComponent::m_FOVX`.
- Position and orientation from the entity's `TransformComponent`.

#### 3d. Output and shutdown sequence

- Write result to `Bin/RelWithDebInfo/cpu_reference.png` via `stb_image_write_png`.

**Execution site:** Call `g_Engine->Get<RayTracer>()->Execute()` from `WorldSystem::Terminate()`. `Engine::Terminate()` waits on the render-loop task before calling `LogicClient::Terminate()`, so the GPU capture (which happens inside the render loop) is guaranteed complete before `WorldSystem::Terminate()` runs. `IGraphicsService::Terminate()` runs after `LogicClient::Terminate()`, so all GPU resources remain valid during path tracing.

**Blocking — implementation notes:**
- `RayTracer::Execute()` keeps its `bool` return type (no interface change to `IRayTracer`).
- Inside `Execute()`, store the task handle in `RayTracerNS` (the file-scope namespace in `RayTracer.cpp`): add `Handle<ITask> m_LastTask` to the namespace alongside the existing `m_isWorking` and `m_TextureComp` variables. Assign: `RayTracerNS::m_LastTask = g_Engine->Get<TaskScheduler>()->Submit(...)`.
- `RayTracer::Terminate()` calls `RayTracerNS::m_LastTask->Wait()` before setting `ObjectStatus::Terminated`. Call syntax: `m_LastTask->Wait()` — `Handle<T>::operator->()` (`Handle.h:69`) returns the underlying `T*`, so this forwards correctly to `ITask::Wait()` (`Task.h:73`).
- **Latent fragility note:** `TaskScheduler::Freeze()` and `Reset()` are called in `Engine::Terminate()` after `LogicClient::Terminate()` completes. If that ordering ever changes, `Wait()` could deadlock. This is not an action item now but should be documented in the code near the `Wait()` call.

**Error contract:** If `EntityRegistry` yields no mesh entities (e.g., scene not loaded), write a 1×1 black PNG and log an error.

**Acceptance:** `cpu_reference.png` exists, is non-black, and shows visible indirect bounce lighting (colored walls casting light into the scene interior).

---

### 4. TestGIScene.ps1 Comparison Logic

**Tool:** ImageMagick `magick compare`. Script checks for its presence and exits 1 with an install instruction if absent.

**Metrics:**
- Mean Absolute Error (MAE) across all pixels on the luminance channel: `magick compare -metric MAE gpu_output.png cpu_reference.png null:`.
- Presence check: both PNGs must exist and be > 100 bytes.
- NaN/Inf check: `magick identify -verbose gpu_output.png` — fail if reported max channel value is "undefined" or "infinity".

**Threshold:** Set empirically on the first passing run after the HitableCube normal fix (§3a) is in place, so the reference is physically meaningful. Initial value: 0.20 (loose). The threshold is committed as a constant in the script; it is tightened as GPU GI quality improves. **Note:** a threshold calibrated before §3a is fixed will be meaningless (the reference has wrong normals), so do not commit the baseline until §3a is confirmed working.

**Output:** Script prints the actual MAE on every run for trend tracking regardless of pass/fail.

**Acceptance:** Script exits 0 on a clean run; exits 1 with a descriptive message for any failure mode (missing file, MAE exceeded, NaN, ImageMagick absent).

---

## Implementation Order

Each step is a prerequisite for the next. Do not proceed past a step that has not met its acceptance criteria.

1. **Diagnose and fix mesh GPU buffer views** → confirm GITestBox geometry renders
2. **Build GPU frame capture** → confirm `gpu_output.png` appears and is non-trivial
3. **Fix HitableCube normals** → confirm cube faces scatter correctly (unit test with a single lit cube)
4. **Integrate CPU path tracer with live scene and camera** → confirm `cpu_reference.png` shows inter-reflections
5. **Add comparison to TestGIScene.ps1** → tune and commit MAE threshold

---

## Error Handling Summary

| Failure Mode | Behavior |
|---|---|
| Mesh template race not resolved by wait | Log error, skip geometry — scene will be empty but test still runs |
| GPU readback allocation fails | Log error, write 1×1 black PNG |
| GPU readback map/copy fails | Log error, write 1×1 black PNG |
| CPU path tracer: no mesh entities | Log error, write 1×1 black PNG |
| CPU path tracer task still running at shutdown | `Wait()` blocks until complete — no timeout |
| ImageMagick not found | Script exits 1 with install instructions |
| Either PNG missing or < 100 bytes | Script exits 1 |
| MAE > threshold | Script exits 1, prints actual MAE |
| NaN/Inf in GPU output | Script exits 1 |

---

## Success Criteria

- `TestGIScene.ps1` exits 0 with both PNGs present and MAE logged
- CPU reference shows correct face normals on all cube surfaces (confirmed via single-cube test before full integration)
- CPU reference shows visible indirect bounce lighting (colored walls casting color into scene interior)
- MAE threshold committed as regression baseline only after HitableCube normal fix is in place
- No D3D12 validation errors (existing criterion preserved)
- All four steps in CLAUDE.md workflow (build → runtime test → shader test → peer review) pass for each workstream

---

## Out of Scope

- Triangle-level BVH (AABB box primitives are sufficient for GITestBox's cube geometry)
- Dielectric/glass materials (not present in GITestBox)
- Importance sampling (8 SPP Lambertian is sufficient for comparison purposes)
- Golden reference image checked into git (threshold-based MAE is the regression mechanism)
- Perceptual metrics (SSIM, etc.) — MAE on luminance is sufficient for now
- Timeout in `RayTracer::Terminate()::Wait()` — path tracer is bounded by scene size; GITestBox at 8 SPP completes in seconds
