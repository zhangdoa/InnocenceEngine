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
- `FinalBlendPass` has an existing CPU readback path (C-key) that writes a PNG — but it is not triggered automatically.

## Architecture

```
TestGIScene.ps1
  └─ Main.exe -frames N
       ├─ [Fix]  LoadScene: MeshComponent GPU buffer views copied from template
       ├─        WorldSystem renders N frames of GITestBox
       ├─ [New]  Frame N-1: GPU readback → Bin/RelWithDebInfo/gpu_output.png
       ├─ [New]  Shutdown hook: CPU path tracer → Bin/RelWithDebInfo/cpu_reference.png
       └─ exit
  └─ magick compare gpu_output.png cpu_reference.png
       ├─ mean absolute error (MAE) logged
       ├─ NaN/Inf pixel check
       └─ PASS if MAE < threshold, else FAIL
```

## Components

### 1. Mesh GPU Buffer Fix

**Problem:** `AssetService::Load(path, MeshComponent&)` deserializes JSON fields (MeshShape, InstanceName) but does not copy `VertexBufferView` / `IndexBufferView` from the template asset. All GITestBox mesh entities have zero vertex stride after scene load.

**Fix:** In `JSONWrapper::LoadScene`, after `AssetService::Load(l_FilePath.c_str(), l_Mesh)`, look up the template by `l_CompName` in `TemplateAssetService` and copy the buffer views.

**Acceptance:** GITestBox entities render visible geometry (no vertex stride zero errors in log).

### 2. GPU Frame Capture

**Mechanism:** Reuse the existing `FinalBlendPass` CPU readback path.

**Trigger:** In `WorldSystem::Update()`, set a capture flag one frame before auto-terminate (i.e., when `m_AutoFrameCount == l_maxFrames - 1`). The flag is read by `FinalBlendPass` to perform the readback and write `Bin/RelWithDebInfo/gpu_output.png`.

**Error contract:** If readback fails, write a 1x1 black PNG so the downstream comparison fails loudly.

**Acceptance:** `gpu_output.png` exists in `Bin/RelWithDebInfo/` after test run and contains a non-trivial image.

### 3. CPU Path Tracer Scene Integration

Extend `RayTracer.cpp`, preserving its existing Monte Carlo structure.

**Changes:**

- **Scene population:** Replace hardcoded spheres with a loop over `EntityRegistry` — collect entities that have both `MeshComponent` (with valid buffer views) and `TransformComponent`. Build `HitableBox` primitives from the entity's local-space AABB scaled by `m_LocalScale`, positioned by `m_LocalPos`, oriented by `m_LocalRot`.
- **Material mapping:** Map `MaterialComponent` roughness/albedo to Lambertian (rough) or Metal (smooth). Emissive materials (lights) map to a new `Emissive` hitable type.
- **Camera:** Read `CameraComponent` FOV and near/far from the camera entity; read position/rotation from its `TransformComponent`.
- **BVH:** Build a flat AABB list from all hitables, pass to `BVHService::Build()`, use for ray traversal.
- **Output:** Write result to `Bin/RelWithDebInfo/cpu_reference.png` via the existing `IOService` / stb_image_write path.
- **Execution:** Triggered at engine shutdown (before window/device teardown), runs on a dedicated task thread, blocks until complete before returning.
- **SPP:** 8 (preserving existing value) — tunable via `InitConfig` later.

**Error contract:** If no mesh entities are found or BVH is empty, write a 1x1 black PNG and log an error.

**Acceptance:** `cpu_reference.png` exists, is non-black, and shows visible indirect bounce lighting (colored walls casting color into the scene interior).

### 4. TestGIScene.ps1 Comparison Logic

**Tool:** ImageMagick `magick compare` (assumed present; script checks and exits 1 with clear message if absent).

**Metrics:**
- Mean Absolute Error (MAE) across all pixels, luminance channel.
- Presence check: both PNGs must exist and be larger than 100 bytes.
- NaN/Inf check: `magick identify -verbose` on `gpu_output.png` reports min/max; fail if max is infinity or undefined.

**Threshold:** Set empirically on the first passing run; committed as a constant in the script. Initial value: 0.20 (loose — tighten as GPU quality improves).

**Output:** Script prints MAE value on every run for trend tracking, regardless of pass/fail.

**Acceptance:** Script exits 0 on a clean run, exits 1 with a descriptive message on any failure mode (missing file, MAE exceeded, NaN, ImageMagick absent).

## Implementation Order

Each step unblocks the next; no step may be skipped.

1. **Mesh GPU buffer fix** — prerequisite for all rendering work
2. **GPU frame capture** — verify `gpu_output.png` appears
3. **CPU path tracer scene integration** — verify `cpu_reference.png` appears and is non-black
4. **TestGIScene.ps1 comparison logic** — tune MAE threshold on first real run

## Error Handling Summary

| Failure Mode | Behavior |
|---|---|
| Template mesh not in TemplateAssetService | Log warning, leave buffer views null (no worse than before) |
| GPU readback fails | Write 1x1 black PNG, log error |
| CPU path tracer finds no mesh entities | Write 1x1 black PNG, log error |
| ImageMagick not found | Script exits 1 with install instructions |
| Either PNG missing | Script exits 1 |
| MAE > threshold | Script exits 1, prints actual MAE |
| NaN/Inf in GPU output | Script exits 1 |

## Success Criteria

- `TestGIScene.ps1` exits 0 with both PNGs present and MAE logged
- CPU reference shows visible indirect bounce lighting (colored walls casting color)
- MAE threshold committed as regression baseline after first passing run
- No D3D12 validation errors (existing criterion preserved)
- All four build/runtime/shader/review steps in CLAUDE.md workflow pass

## Out of Scope

- Triangle-level BVH (box primitives are sufficient for GITestBox's cube geometry)
- Dielectric/glass materials (not present in GITestBox)
- Importance sampling (8 SPP Lambertian is sufficient for comparison purposes)
- Golden reference image checked into git (threshold-based MAE is the regression mechanism)
- Perceptual metrics (SSIM, etc.) — MAE on luminance is sufficient for now
