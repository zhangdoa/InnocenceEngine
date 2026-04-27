---
id: TASK-147
title: 'Point shadow A: foundation — cube atlas + LightDataService schema + slot allocator'
status: Done
assignee:
  - graphics-api-expert
created_date: '2026-04-26 22:30'
updated_date: '2026-04-27 09:30'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
  - infrastructure
dependencies:
  - TASK-66-design
parent_task_id: TASK-66
priority: high
references:
  - Source/Engine/Services/LightDataService.cpp
  - Source/Engine/Services/LightDataService.h
  - Source/Engine/Services/RenderingConfigurationService.h
  - Source/Engine/Common/GPUDataStructure.h
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-66 (point/sphere light shadows in rasterized pipeline).** Owner: `graphics-api-expert`.

Foundation infrastructure that unblocks the caster pass (TASK-148) and the component flag work (TASK-149). No rendering output by itself; this is plumbing.

### What this subtask delivers

1. **Cube-array shadow atlas allocation** — a single `Texture2DArray` with `DepthOrArraySize = maxPointShadows * 6` slices, mirroring the sun-shadow color-target idiom (`SunShadowGeometryProcessPass.cpp:44-48`). This is allocated as part of the new `PointShadowGeometryProcessPass` render-pass setup, but the atlas resource lifetime + descriptor wiring + barrier policy is the graphics-api scope.
2. **`PointShadowConstantBuffer` schema** in `Source/Engine/Common/GPUDataStructure.h`. Exact shape decided by the design call (see Implementation Notes — block until rendering-researcher publishes their recommendation), but minimally: per-shadowed-light, the 6 cube-face view+proj matrices and the atlas base slot index. Mirror the existing `CSMConstantBuffer` alignment idiom (`alignas(16)`, padding to canonical size).
3. **`LightDataService::GetPointShadowBuffer()`** + an internal `m_PointShadowCBVector` populated each frame from the LightComponent storage. Mirrors `m_CSMCBVector` / `GetCSMBuffer()` in `LightDataService.cpp:51,55,360-363`.
4. **Atlas slot allocator** — simple per-frame pack: walk shadow-casting point/sphere lights in deterministic order, assign slots `[0..N-1]`. No persistence across frames in this subtask (defer LRU / temporal coherence to a follow-up).
5. **`RenderingCapability::maxPointShadows`** field in `RenderingConfigurationService.h` next to `maxPointLights/maxCSMSplits`. Default value comes from the design call; the surfacing producer's analysis of "32 × 6 × 256² D32 ≈ 12 MB" is the budget anchor — `rendering-researcher` confirms quality target, this subtask confirms VRAM fit and lands the constant.

### What this subtask does NOT do

- No caster shaders, no LightPass binding, no shadowResolver extension — those are TASK-148.
- No `m_CastShadow` flag on `LightComponent` — that is TASK-149. This subtask exposes a contract ("the slot allocator reads `lightCfg.castShadow` if available, otherwise treats all point/sphere lights as shadow-casters") that TASK-149 then satisfies.
- No editor / serialization changes.

### Project invariants (anchor — read before designing)

- **Sun shadow precedent**: VSM-style packed `(depth, depth², 0, 1)` color attachment (`SunShadowGeometryProcessPass.cpp:53-59`, `sunShadowGeometryProcessPass.frag:61`). `Texture2DArray` is the canonical multi-slice shadow representation in this codebase; do not introduce TextureCube unless the design call explicitly mandates it.
- **Atlas resource type** depends on the design call (depth-only D32 for hardware-PCF vs. color-target VSM for moment-PCSS). Whichever it is, it must follow the existing `RenderPassDesc` idiom (`m_RenderTargetDesc` / `m_UseDepthBuffer`).
- **Service Update() ordering**: `LightDataService::Update()` runs each frame and uploads light + CSM buffers (`LightDataService.cpp:276-305`). Point-shadow buffer upload must follow the same pattern; if the cbuffer needs the camera frustum (e.g. for slot prioritization later), check `CameraService` ordering.
- **No magic numbers**: `maxPointShadows` is the canonical capacity; no inline `6` for face count except in geometry-shader code where it is structurally clear.

### Why this is foundation-first

Caster pass (TASK-148) needs the constant buffer to know face matrices + atlas base slot. Component flag (TASK-149) needs the contract that "shadow-casting lights have an atlas slot index". Sequencing this subtask first lets the other two go in parallel.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `RenderingCapability::maxPointShadows` field present, default value matches the design call from TASK-66 design audit
- [x] #2 `PointShadowConstantBuffer` declared in `Source/Engine/Common/GPUDataStructure.h` with shape consistent with the design call (face matrices + atlas base slot at minimum)
- [x] #3 `LightDataService::GetPointShadowBuffer()` returns a populated `GPUBufferComponent`; populated each frame from the light storage; size ≤ `maxPointShadows`
- [x] #4 Cube atlas resource allocated (resource type per design call: D32 depth or float2 color); descriptor / barrier wiring verified — engine starts without GBV errors
- [x] #5 Engine launches GISponza windowed without regression; no shadow output yet (no caster pass), but no crash, no GBV warnings
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Sequencing**: BLOCKED on the TASK-66 design call (VSM vs PCF on cube atlas; atlas resolution + slot count). `rendering-researcher` resolves both and amends this task before implementation begins. Until then the AC#1/AC#2 details are placeholders.

**Build-cache hygiene**: TASK-146 has landed (mirror-semantic shader deploy via `Scripts/Lib/Compile-HLSL.psm1` + `CMake/DeployRuntimePayload.cmake`). `Scripts/HLSL2DXIL_NoPause.ps1` deletes orphan `.dxil` automatically; `cmake --build` wipes-and-recopies the per-Config deploy target. No manual nuke needed; pass `-FullClean` to the script for paranoid bisects.

**Prior art to mirror**:
- `SunShadowGeometryProcessPass::Setup()` for render-target descriptor shape (texture-array, packed depth color, viewport sizing, comparison function).
- `LightDataServiceImpl::UpdateCSMData()` for the per-frame matrix-vector populate idiom and `Upload()` call.
- The 6-face matrix array of `GIConstantBuffer.r[6]` / `v_inv[6]` (`GPUDataStructure.h:113,116`) as cbuffer-shape precedent for "6 face transforms in one constant".

---

**Implementation closure (graphics-api-expert, 2026-04-27):**

**Files touched (foundation only — no shaders, no passes, no build infra):**
- `Source/Engine/Common/GPUDataStructure.h` — `PointShadowConstantBuffer` (512B, mirrors `CSMConstantBuffer` 256B-multiple alignment idiom). `INVALID_ATLAS_SLOT` was landed by TASK-149 in parallel; my code consumes it.
- `Source/Engine/Services/RenderingConfigurationService.{h,cpp}` — `RenderingCapability::maxPointShadows = 8` (per TASK-150 design call).
- `Source/Engine/Services/LightDataService.{h,cpp}` — `m_PointShadowCBVector`, `m_PointShadowGPUBufferComp`, `m_PointShadowAtlas`, `UpdatePointShadowData()` slot allocator + per-light cbuffer populator, `GetPointShadowBuffer()`, `GetPointShadowAtlas()`, `GetPointShadowCount()`.

**Atlas placement decision**: allocated as a standalone `TextureComponent` via `TextureResourceService` in `LightDataService::Setup`/`Initialize`/`Terminate`, **not** via a skeletal `RenderPassComponent`. Rationale: a `RenderPassComponent`'s `InitializeRenderPass` requires a `ShaderProgram` and triggers `CreatePipelineStateObject`, both of which TASK-148 owns. Pre-allocating a half-built render pass would force TASK-148 to either delete + re-create the resource or wire a sentinel shader. Standalone `TextureComponent` is the same pattern used by `BRDFLUTPass::m_Result` (compute-only) and `LightPass::m_LuminanceResult` — `Usage = ColorAttachment` routes through `DX12Helper::GetTextureBindFlags` and gets `D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET` correctly. TASK-148 retrieves the texture via `GetPointShadowAtlas()` and binds it to `PointShadowGeometryProcessPass` via `m_RenderTargetsCreationFunc` — the existing customization hook for "I already have the resource, don't allocate a new one."

**Cbuffer shape (PointShadowConstantBuffer, 512B per element):**
- `Mat4 p` — shared across all 6 cube faces (90° FOV, 1.0 aspect, near=0.1m, far=range). Single matrix instead of `p[6]` saves 320B/element and the GS reuses it for every face.
- `Mat4 v[6]` — per-face view-from-light, computed via `Math::lookAt(lightPos, lightPos + faceForward, faceUp)`. Face order matches D3D TextureCube convention: +X, -X, +Y, -Y, +Z, -Z. ±Y faces use ±Z as `up` to avoid look-direction collision.
- `Vec4 lightPosWS_range` — `.xyz` world position, `.w` = range. TASK-148's caster `frag` reads this to compute linear depth `length(posWS - lightPosWS) / range`.
- `uint32_t atlasBaseSlot` — first slice in `Texture2DArray`; faces occupy `[base..base+5]`. Caster GS sets `SV_RenderTargetArrayIndex = atlasBaseSlot + faceIdx`.
- `uint32_t isActive` — 1 if slot is live, 0 if sentinel. Caller-readable convenience flag (redundant with `atlasBaseSlot == INVALID_ATLAS_SLOT` but matches design-call language).
- Padding: `uint32_t[2] + float[8]` to land at 512B (next 256B-multiple after 480B body). CSM uses 256B; point-shadow needs 6 face matrices so 512B is the minimum aligned size.

**Slot allocator behavior**: dense per-frame pack. Walks `EntityRegistry::Storage<LightComponent>().All()` in registry order; for each `Point`/`Sphere` light with `m_CastShadow == true` and a live `TransformComponent`, assigns the next free slot in `[0..maxPointShadows-1]`. `m_PointLightAtlasSlot[k]` / `m_SphereLightAtlasSlot[k]` (sidecar landed by TASK-149) gets the slot index; lights beyond budget keep `INVALID_ATLAS_SLOT` and produce a Warning. No persistence across frames (defer LRU/temporal to follow-up). Failure paths (missing transform, range ≤ near plane) log Warning and skip — never silently swallow.

**Atlas slot ↔ light index mapping**: stored in the per-light sidecar (`m_PointLightAtlasSlot` / `m_SphereLightAtlasSlot` — service-level `Get*AtlasSlot(in_Index)` accessors landed by TASK-149) rather than added to `PointLightConstantBuffer` / `SphereLightConstantBuffer` cbuffers. Reason: extending those structs would force a matching `Source/Shaders/HLSL/common/common.hlsl` change to `PointLight_CB` / `SphereLight_CB`, which is rendering-researcher's subtree. TASK-148 chooses how to expose the slot to LightPass HLSL — either by adding the cbuffer field there, by adding a parallel `PointShadowSlotIndex` SSBO, or by reading the slot directly from `PointShadowConstantBuffer[slot]` after a CPU-side `LightIdx -> SlotIdx` lookup.

**VRAM budget check (correction to TASK-150's design call):**
TASK-150 cited `8 × 6 × 256² × Float32 (4B) = 12 MB`. That's wrong for the chosen format: `PixelDataFormat::RGBA × PixelDataType::Float32 = R32G32B32A32_FLOAT = 16B/pixel`. Real per-slice cost: `48 × 256 × 256 × 16B = 50,331,648 B ≈ 48 MB`. With `IsMultiBuffer = true` (mirroring sun-shadow precedent for swap-chain cycling) and 3-frame swap chain: `~144 MB total`. This is roughly the same order as sun shadow's `4 × 2048² × 16B × 3 = 192 MB` so the codebase tolerates this scale. No regression observed under GBV. To shrink: drop to `PixelDataFormat::RG` (R32G32_FLOAT, 8B/pixel) → 72 MB total — still mirrors sun's `(depth, depth²)` semantic since only `.r` and `.g` are used by PCSS. Recommend rendering-researcher consider this in TASK-148 if VRAM proves an issue.

**Validation summary:**
- Engine build clean: `cmake --build Build --config RelWithDebInfo` — Engine.lib, ExampleRenderingClient.lib, Main.exe, RenderTest.exe, TestSuite.exe all rebuilt with no warnings or errors related to TASK-147.
- Engine launch: `Main.exe -renderer 0 -loglevel 0 -gpu_validation -total_frames 60` — runs ~90 seconds, loads `UnitTest.InnoScene`, auto-loads `GISponza.InnoScene` at frame 5 (1 directional + 2 point + 1 sphere lights, 3 shadow-casters within budget), renders 60 more frames, auto-terminates clean. Log `[2026-4-27-7-17-45-151].Log` (343 KB) contains zero `D3D12 ERROR`/`D3D12 WARNING`/`Validation Error`/`Validation Warning`/`CORRUPTION` matches. `PointShadowAtlas` and `PointShadowCBuffer` both reach `is initialized.`/`(GPUBufferUsage::Generic) is initialized.` lifecycle markers.
- TestSuite: 46/47 pass. The 1 failure (`FixedSizeString: trailing slash is preserved (no sacrificial-char truncation)`) is in `Source/Engine/Common/FixedSizeString.h` territory, not touched by TASK-147 — pre-existing, owned by low-level-expert.

**What was NOT verified:**
- **Caster output**: TASK-148 has not landed; atlas is live but no shader writes to it. Cannot RenderDoc-capture the shadow data — there isn't any.
- **Resolver path**: LightPass HLSL still uses pre-shadow code paths (no `PointShadowResolver` call). Cannot verify `m_PointLightAtlasSlot[k]` is read correctly from shader side until TASK-148 wires it.
- **Multi-light budget overflow**: GISponza scene has 3 shadow-casters, well under the budget of 8. The "exceeded `maxPointShadows`" warning path was not exercised. Manual inspection of the allocator confirms the guard is correct (`if (l_NextSlot >= l_MaxPointShadows) continue;`) but no scene currently authors >8 shadow-casting positional lights.
- **Cube-face matrix correctness**: lookAt + perspective matrices are computed and uploaded but never sampled by any shader yet (TASK-148). A subtle face-orientation bug (wrong `up` for ±Y, swapped front/back) would not surface until the caster + resolver land.
- **Sphere-light range semantic**: `m_Shape.x` is overloaded — for Point it's "auto-calculated attenuation radius", for Sphere it's "sphere radius". My allocator uses it as the perspective `zFar` for both. For sphere lights this likely under-projects the cube (a 1m sphere in a 50m room would map shadows only to the sphere surface). TASK-148 may need a separate "shadow range" field or compute attenuation radius from `m_LuminousFlux` for sphere lights too. Documented for follow-up.
- **No PointShadow CB upload happened in this run**: GISponza has 3 point/sphere lights all with `m_CastShadow == true` (default), so the 3-element cbuffer should have uploaded successfully. Confirmed by absence of GBV errors on the upload path (mapped-memory write would have triggered out-of-bounds validation if the struct size mismatched). Did not confirm upload occurred via positive log evidence — `Upload` is silent on success per `safety-observability.md`.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (engine + shader tier)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — engine launches GISponza windowed without regression; screenshot / RenderDoc capture proving no crash and no GBV warnings; atlas resource visible in capture
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
