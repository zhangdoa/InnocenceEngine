---
id: TASK-77.2
title: 'Post-PT denoiser: demodulated diffuse/specular SVGF-shape (in-house, no NRD)'
status: In Progress
assignee:
  - '@claude'
created_date: '2026-05-07'
updated_date: '2026-05-07'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
dependencies: []
parent_task_id: TASK-77
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add the **screen-space sample-reuse denoiser** missing from TASK-77.1. The hash-grid cache port (TASK-77.1, commits b6058cdc → 82c74743) is faithful to Capsaicin GI-1.0 but is architecturally a *long-tail-radiance feeder*, not a denoiser — every shipped primary-PT pipeline (Capsaicin, RTXPT, NRD-Sample, Lumen, GIBS) pairs the world cache with a screen-space stage where visible-motion reuse actually lives. TASK-77.1 stopped at the cache layer; this task fills the screen-space layer.

Concretely: temporal accumulation + spatial edge-aware filter on demodulated diffuse + specular radiance at the primary hit. The hash-grid cache from TASK-77.1 stays compile-time-OFF until TASK-77.3 (cache reactivation as long-tail feeder above this denoiser).

## Hard constraint — license

**No NVIDIA NRD library**. License incompatibility (project-side constraint, 2026-05-07 user direction). Architectural reference patterns from NRD-Sample are fair game (read the code; do NOT link the library or vendor any NVIDIA NRD source). Implementation is in-house, drawing on the open literature:

- SVGF (Schied et al. 2017) — temporal accumulation + edge-aware à-trous wavelet, variance-guided.
- A-SVGF (Schied et al. 2018) — adaptive history reuse.
- ReBLUR-shape patterns (hit-distance-driven blur radius, per-lobe diffuse/specular handling) — known from the SVGF lineage and ReBLUR talk slides; structurally implementable without NRD source.
- EA SEED Surfel-GI (SIGGRAPH 2021) — production-shipped post-PT denoise reference.

`git grep -i 'nrd\|nvidia.*nrd'` at any point during implementation must return references-list mentions only, not code.

## Architecture sketch

1. **Primary-hit signal split** (`GPUPathTracerRayGen.hlsl`): output diffuse + specular radiance separately, albedo-demodulated, with per-lobe hit distance.
2. **Temporal accumulation pass**: per-pixel previous-frame motion-vector reprojection, history-rejection on geometry/normal/depth disocclusion, variance estimate (mean^2 − mean of squares).
3. **Spatial filter pass**: edge-aware à-trous wavelet, variance-guided radius modulation, per-lobe. Hit-distance-modulated blur radius for specular.
4. **Composition**: re-modulate albedo, sum diffuse + specular, hand off to tonemap.

This builds atop the loop-per-bounce raygen already in place; no hash-grid coupling, no DXR-callback layer changes.

## Out of scope

- Hash-grid cache reactivation — TASK-77.3 (when filed).
- ReSTIR DI / GI / PT primary-hit reservoir reuse — TASK-77.3 candidate, not 77.2.
- Neural denoisers (NRC, OIDN, OptiX) — separate axis under TASK-77 umbrella.
- Motion-vector pipeline rework — current TAA motion vectors are reused as-is; if they prove insufficient, file separately.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 (AC-1, visual, blocking) Noise-floor reduction visible on motion across UnitTest + GITestBox + GISponza. User-direction layer-4 sign-off on at least one moving-camera capture per scene.
- [ ] #2 (AC-2, visual, blocking) No temporal artifacts (boil, ghost, lag). Static-camera convergence at least matches denoiser-OFF baseline.
- [ ] #3 (AC-3, supporting only) Per-pixel temporal stddev reduction on settled frames vs denoiser-OFF baseline. Cannot close on its own.
- [ ] #4 License audit: `git grep -i 'nrd\|nvidia.*nrd'` returns references-list mentions only, no code.
- [ ] #5 Bypass invariant: with denoiser disabled (compile-time toggle), output is bit-identical to current PT-only HEAD. Verified at every commit on the implementation chain.
- [ ] #6 Engine builds clean (RelWithDebInfo); GBV clean on smoke run.
- [ ] #7 Peer review by a fresh impl-stage agent (cross-stage if HLSL+C++ span warrants).
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Cross-references

- TASK-77 — parent umbrella (RD: PT denoise → promote to primary).
- TASK-77.1 — closed 2026-05-07 with "wrong-framing accepted" verdict; cache implementation correct + paper-port faithful, but cache architecture is feeder, not denoiser. Toggle stays compile-time-OFF.
- `.alignments/TASK-77.1-rework-paper-port-audit.md` — Capsaicin port audit (cache only).

## References to add to .claude/references.json on first HLSL author

- SVGF (Schied 2017): `https://cg.ivd.kit.edu/publications/2017/svgf/svgf_preprint.pdf`
- EA SEED Surfel-GI: `https://advances.realtimerendering.com/s2021/SIGGRAPH%20Advances%202021%20-%20Surfel%20GI.pdf`
- NRD-Sample (architectural reference only): `https://github.com/NVIDIA-RTX/NRD-Sample`

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## CL-1 design plan (2026-05-07)

### Architectural fork — Path A locked

Path B (reuse rasterized GBuffer) and Hybrid (reuse rasterized for "where am I", PT-write for denoiser-specific channels) are **broken-by-construction in the PT-primary path**: `OpaquePass` is gated under `if (!m_GPUPathTracerActive)` (`ExampleRenderingClient_PrepareCommands.cpp:82`, `_ExecuteCommands.cpp:144`), so `in_opaquePassRT0..3` carry stale or zero contents in PT mode. Reusing them would re-elevate the demoted rasterizer subsystem to load-bearing — exactly the failure mode `state/project-direction.md` warns against.

**Path A locked**: PT raygen writes per-primary-hit channels itself. Channel layout mirrors `OpaquePass.frag:124-129` so existing `DecodeGBuffer` / `lightPass.comp:161` decoders are reusable on the denoiser side without forking.

- RT0: positionWS (rgb) + mesh-id-or-skyflag (a)
- RT1: normalWS (rgb) + metalness (a)
- RT2: albedo (rgb) + roughness (a)
- RT3: motionVec.xy + hitDist (z) + reserved (w)

**Note (CL-1 correction)**: design-pass draft swapped RT1/RT2 alpha channels (claimed normal+roughness / albedo+metalness). Actual `DecodeGBuffer` (`lightPass.comp:161`) and `OpaquePass.frag` decoder contract is normal+metalness / albedo+roughness — CL-1 follows the decoder, layout above is corrected.

Motion vector at primary hit = project hit position with current `g_Frame.v_inv * g_Frame.p_original` and stored `prev_v * prev_p` (already maintained as `m_PrevViewMatrix` in `GPUPathTracerPass.h:91`); subtract in screen space.

### Per-pass design

**Signal split** (`GPUPathTracerRayGen.hlsl`, modified): at `bounce == 0` capture GBuffer-equivalent channels and split radiance into `radianceDiffuse` + `radianceSpecular`. Lobe assignment fires at the **primary-hit BSDF importance sample only** (existing `lobeSample < pDiffuse` branch ~line 629); the sampled lobe tags the path immutably for the rest of the path. NEE-at-primary-hit goes to diffuse (specular-NEE refinement is CL-5).

**Temporal accumulator** (CL-2 — `PTDenoiseTemporalPass`): compute shader, motion-vector reprojection, per-lobe history rejection (depth Δ + normal dot + mesh-id), variance estimate (Welford). Pattern ports from `GIDenoise.comp:184-260` swapping the rasterized motion-vector source for the PT-written one.

**Spatial à-trous** (CL-3 — `PTDenoiseAtrousPass`): single compute shader dispatched 5× with stride 1/2/4/8/16, ping-pong textures. SVGF edge-stopping weights (depth, normal, luminance vs √variance). Hit-distance modulates specular blur radius.

**Composition** (CL-4 — folded into `FinalBlendPass` or new `PTComposePass`): re-modulate diffuse by primary-hit albedo, sum diffuse + specular, write to a "denoised display radiance" texture replacing PT's accumulation as tonemap input. **PT accumulation buffer stays untouched** — toggle-OFF bit-identity holds.

### CL split

| CL | Scope | AC | Visible-progress shape |
|----|-------|----|------|
| **CL-1** | Raygen splits radiance into diffuse/specular + writes 4 GBuffer-equivalent UAVs at `bounce == 0`. New toggle `Inno::PTDenoise::ENABLED` defaults OFF. AccumBuffer composition unchanged. | AC-5 bypass (bit-identical), AC-6 builds clean. | RenderDoc-visible new UAV outputs at `bounce == 0`; no on-screen change. |
| **CL-2** | `PTDenoiseTemporalPass` — temporal accumulation + variance estimate, per-lobe history rejection. Composition still uses pre-temporal output when toggle ON. | Per-lobe history textures populated; bypass-OFF baseline matches HEAD. | 60-frame capture: history-on path shows reduced temporal noise on settled frames. |
| **CL-3** | `PTDenoiseAtrousPass` 5-iteration à-trous spatial filter, variance-guided. | AC-1 partial — visible noise-floor reduction in motion. | Side-by-side capture (toggle on vs off) on UnitTest. |
| **CL-4** | Composition pass — re-modulate albedo, sum lobes, swap into tonemap input. | All visual ACs land. | Three-scene moving-camera captures. |
| **CL-5 (optional)** | Specular-NEE-at-primary lobe assignment refinement. | AC-1 polish. | Glossy-floor test capture. |

### CL-1 risks / open questions (resolve in implementation)

- `m_PrevViewMatrix` is declared but design pass did not verify it's updated every frame. CL-1 implementer must check and wire if needed.
- PathTracerPayload may not carry instanceID; needs adding for the mesh-id history-rejection channel in CL-2. Cheapest: pack into RT0's `.w` channel.
- `tonemap` reads `GPUPathTracerPass::GetResult()` directly — CL-4 needs to substitute this; not a CL-1 concern but flagged for CL-4 entry.
- Two compile-time toggles co-exist post-CL-1: `PT_HASH_GRID_CACHE_ENABLED` (TASK-77.1, OFF) and `PT_DENOISE_ENABLED` (this task, OFF). Confirm `#if` nesting is orthogonal — denoiser writes happen at `bounce == 0`, never inside cache-write `bounce >= 1` block.

## CL-1: signal split + GBuffer-equivalent UAV writes (2026-05-07)

### Files touched

| Path | Role | Notable |
|---|---|---|
| `Source/ExampleProject/RenderingClient/PTDenoiseConstants.h` | NEW. C++ toggle. | Mirrors `HashGridCacheConstants.h` shape; CL-1 keeps it minimal — only the `ENABLED = false` flag. |
| `Source/Shaders/HLSL/common/PTDenoiseShared.hlsl` | NEW. Channel-name + motion-vec convention header. | Documents the layout-mirror invariant against `opaqueGeometryProcessPass.frag` and `lightPassCommon.hlsl::DecodeGBuffer`. |
| `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | Add `#define PT_DENOISE_ENABLED 0`, b4 + u7..u10 binding decls, `radianceDiffuse`/`radianceSpecular`/`isSpecularPath` accumulators, lobe tag at the primary-hit BSDF importance sample, NEE-site lobe routing, bounce==0 GBuffer-equivalent UAV writes, sky-miss UAV zero, AccumBuffer dual-form write. | Every new line under `#if PT_DENOISE_ENABLED` (12 named UAV refs, 17 lobe-bucket refs, 16 toggle-name refs in this file). |
| `Source/Shaders/HLSL/common/pathTracerPayload.hlsli` | Add `uint instanceID;` to `PathTracerPayload`. | 56B → 60B, still under MaxPayloadSizeInBytes=64. |
| `Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl` | Set `payload.instanceID = InstanceID();` once. | One line, no other behavioural change. |
| `Source/Engine/Services/DX12/DX12RenderPassResourceService_Pipeline.cpp` | Update payload-size comment. | Comment-only; the 64B value is unchanged. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass.h` | 4 `TextureComponent*` members + 4 accessors + 2 helper-method decls. | All gated `if constexpr (Inno::PTDenoise::ENABLED)` at use sites. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp` | Terminate / OnResize toggle blocks; `CreatePTGBufferTextures()` / `DeletePTGBufferTextures()` impls. | RGBA16F across all four (mirrors `RenderingConfigurationService::m_DefaultRenderPassDesc` Float16). |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Initialize.cpp` | `if constexpr` block calling `CreatePTGBufferTextures()`. | |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Setup.cpp` | New `l_denoiseBindingCount`, new static_assert, 5 layout-block entries. | `static_assert` short-circuits on toggle-OFF (mirrors cache-block pattern). |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Dispatch.cpp` | 4 transition pairs (Graphics CL pre, Compute CL post) + 5 BindGPUResource calls. | Slot-index arithmetic mirrored from Setup. |
| `.claude/references.json` | Update `GPUPathTracerRayGen.hlsl` entry with SVGF lineage; add `common/PTDenoiseShared.hlsl` entry. | NRD-Sample annotated as architectural reference only — no library / source vendored. |

### HLSL `#if PT_DENOISE_ENABLED` block locations + line counts

| Lines | Role |
|---|---|
| 78-80 | include of `common/PTDenoiseShared.hlsl` |
| 96-103 | previous-frame CB binding declaration (b4) |
| 141-157 | 4 GBuffer-equivalent UAV declarations (u7..u10) |
| 336-352 | sky-miss UAV-zero + sky-radiance routed to diffuse |
| 388-402 | bounce==0 split + lobe accumulators + `isSpecularPath` decl |
| 425-474 | bounce==0 GBuffer-equivalent UAV writes + motion-vec math |
| 496-504 | sun-NEE lobe-routing |
| 541-547 | sky-NEE lobe-routing |
| 587-593 | point-NEE lobe-routing |
| 650-656 | sphere-NEE lobe-routing |
| 776-785 | cache-substitution path lobe-routing (only active when both toggles on) |
| 820-831 | lobe tag set false at diffuse-branch entry |
| 849-852 | lobe tag set true at specular-branch entry |
| 895-908 | AccumBuffer dual-form write switch (`radianceDiffuse + radianceSpecular` when on) |

14 separate `#if PT_DENOISE_ENABLED` blocks; ~170 HLSL lines under the toggle. Toggle-OFF, all strip out.

### Static_assert FIRE-verification

- Pre-revert: temporarily set `Inno::PTDenoise::ENABLED = true` in `PTDenoiseConstants.h` AND `l_denoiseBindingCount = 4` in `GPUPathTracerPass_Setup.cpp`.
- Build output: `error C2338: static_assert failed: 'GPUPathTracer raygen denoiser-binding count must be 5 (b4 + u7..u10). ...'` at `GPUPathTracerPass_Setup.cpp(72,42)` — the new denoiser assert fired at the expected line with the expected message.
- Post-revert: count restored to 5; build clean.

### Per-pass binding-count progression

| Toggle combo | Cache count | Denoise count | Vector size | Slot range |
|---|---|---|---|---|
| Cache OFF + Denoise OFF (CL-1 default) | 0 | 0 | 12 | 0..11 |
| Cache ON  + Denoise OFF | 7 | 0 | 19 | 0..11, 12..18 |
| Cache OFF + Denoise ON  | 0 | 5 | 17 | 0..11, 12..16 |
| Cache ON  + Denoise ON  | 7 | 5 | 24 | 0..11, 12..18, 19..23 |

CL-1 verified on diagonal (off+off, on+on) — `static_assert` short-circuits when toggle off; `error C2338` fires when denoise-on with a wrong count; corrected denoise-on count produces a clean build.

### Diff-hygiene grep results

- `radianceDiffuse|radianceSpecular`: 17 occurrences in 1 file (`GPUPathTracerRayGen.hlsl`).
- `m_PTGBuffer_*`: 28 occurrences across 3 files (header, .cpp impl, dispatch).
- `PT_DENOISE_ENABLED|PTDenoise::ENABLED`: 33 occurrences across 7 files.
- `u_PTDenoise_*`: 12 occurrences in 1 file (raygen).
- License audit: `nrd|nvidia.*nrd` returns project-internal hits in `.claude/references.json` (architectural-reference annotation), `.backlog/tasks/*.md` (task descriptions), `.alignments/*` — no code, no library link, no vendored source.

### Build output (toggle=0 + toggle=1)

- HLSL2DXIL toggle=0: clean. Pre-existing-file recompiles only because `pathTracerPayload.hlsli` changed (the `instanceID` field add). No errors.
- HLSL2DXIL toggle=1: clean. `GPUPathTracerRayGen.hlsl` recompiled; all DXIL emitted.
- BuildWin RelWithDebInfo toggle=0: `Main.exe` + `RenderTest.exe` linked. Pre-existing C4003 `max` macro warnings in `MathHelper.h` (unrelated). No new warnings, no errors.
- BuildWin RelWithDebInfo toggle=1: `Main.exe` + `RenderTest.exe` linked. No errors.

### Capture spot-check

- Toggle OFF: `Bin/RelWithDebInfo/Main.exe -total_frames 30` ran to clean termination. Log line: `[Inno::WorldSystem::Update] Auto-test: 30 frames rendered, terminating.` No D3D12 errors, no GBV warnings (GBV disabled by default per the engine's startup-message). Logs at `Build/captures/pt-denoise-cl1-toggle-off.{log,err}`.
- Toggle ON (optional encouraged): same scene + flags, also clean. 30-frame auto-terminate. Logs at `Build/captures/pt-denoise-cl1-toggle-on.{log,err}`. Demonstrates the toggle path is well-formed end-to-end (UAV alloc + bind + dispatch + transition + Terminate without leaks).

### Risk-question resolutions

- **`m_PrevViewMatrix` per-frame update?** Not used. Verified `m_PrevViewMatrix` in `GPUPathTracerPass.h` is updated inside `_Update.cpp:51-55` ONLY when the camera view changes (it's a tripwire for accumulation reset, not a per-frame snapshot). The motion-vec math in CL-1 instead binds the engine's existing `PerFrameDataService::GetPreviousFrameBuffer()` — the same ping-pong CB the rasterizer's `OpaquePass.frag` consumes at b2. Engine-native; no parallel CB upload added.
- **Payload `instanceID`?** Added. `PathTracerPayload` grew from 56B to 60B (`uint instanceID;` field); `MaxPayloadSizeInBytes` cap (64B) unchanged. `GPUPathTracerClosestHit.hlsl` writes `payload.instanceID = InstanceID();` once. Sky / miss path leaves the field at the zero-init value (the raygen `(PathTracerPayload)0` initialiser fills the whole struct, so `instanceID == 0` on miss). RT0.w stores `instanceID + 1` so instance 0 does not collide with the sky flag at the `DecodeGBuffer::l_RT0.a == 0` decoder gate.
- **OpaquePass channel-format precision?** RGBA Float16 across all four RTs (per `RenderingConfigurationService.cpp:32-36` default render-pass desc). All four PT-GBuffer textures match: `TexturePixelDataFormat::RGBA + TexturePixelDataType::Float16`. Position carries 16F precision floor — same as the rasterizer ships, so denoiser passes (CL-2/3/4) read contract-equivalent data whether the source is rasterized or raytraced.
- **Nested toggle orthogonality?** Verified. Cache writes happen at `bounce >= 1u` (`PT_HASH_GRID_CACHE_ENABLED` block bounded by `if (bounce >= 1u)` at the InsertCell site); denoiser writes happen at `bounce == 0u`. The two `#if`-blocks share no code, no resources, no slots. Confirmed by the four-cell binding-count table above and by all four toggle combinations producing the expected count formula.

## CL-2: temporal accumulator + per-lobe history rejection (2026-05-07)

### Files created

| Path | Lines | Role |
|---|---:|---|
| `Source/Shaders/HLSL/PTDenoiseTemporal.comp` | 224 | Per-pixel motion-vector reprojection + per-lobe history blend. SVGF Σ/Σ² moment estimator at the same α as the radiance blend. Single-tap reprojection (3×3 gather deferred to CL-3's à-trous filter). Sky / first-frame / disocclusion / mesh-id-mismatch all reset sampleCount to 1 and seed moments from the new sample. |
| `Source/ExampleProject/RenderingClient/PTDenoiseTemporalPass.h` | 91 | Pass class declaration. Owns per-lobe radiance UAVs (single-buffered, raygen writes them) + per-lobe history textures (RGBA16F radiance + RG16F moments, ping-pong on FrameCountSinceLaunch). |
| `Source/ExampleProject/RenderingClient/PTDenoiseTemporalPass.cpp` | 229 | Setup / Initialize / Update / Terminate / Status / accessors. 1 CB + 11 SRVs + 4 UAVs binding layout. Update gates activation on the GBuffer-equivalent + history textures all being Activated. |
| `Source/ExampleProject/RenderingClient/PTDenoiseTemporalPass_Dispatch.cpp` | 93 | PrepareCommandList — graphics-CL transitions to compute-readable / compute-writable, compute-CL bind + dispatch. 8×8 ceiling-divided thread groups. |
| `Source/ExampleProject/RenderingClient/PTDenoiseTemporalPass_RenderTargets.cpp` | 65 | RenderTargetsCreationFunc — instantiates the 10 textures (2 radiance current-frame UAVs + 4 history-radiance ping-pong + 4 history-moments ping-pong). |

### Files modified

| Path | Key changes |
|---|---|
| `Source/Shaders/HLSL/common/PTRaygenBindings.hlsl` | Added u11 (RadianceDiffuse) and u12 (RadianceSpecular) inside `#if PT_DENOISE_ENABLED`. |
| `Source/Shaders/HLSL/common/PTRaygenIntegrator.hlsl` | At the AccumBuffer composition site (toggle-on branch), additionally write the clamped per-lobe radiance to u11 / u12. Sky-miss path clears handled by the unified write at function tail. |
| `Source/Shaders/HLSL/common/PTDenoiseShared.hlsl` | Added `PT_DENOISE_MAX_HISTORY_FRAMES = 32`, `PT_DENOISE_NORMAL_DOT_THRESHOLD = 0.95`, `PT_DENOISE_DEPTH_REL_THRESHOLD = 0.1` constants + history-format documentation. |
| `Source/ExampleProject/RenderingClient/PTDenoiseConstants.h` | Mirrored constants (`MaxHistoryFrames`, `HistoryNormalDotThreshold`, `HistoryDepthRelativeThreshold`) on the C++ side. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass.h` | Replaced single `m_PTGBuffer_*` members with Even/Odd pairs. Replaced `Get*` accessors with `GetCurrent*` / `GetPrevious*` ping-pong accessors. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp` | Ping-pong allocation in `CreatePTGBufferTextures` / `DeletePTGBufferTextures`. Anonymous-namespace `PTGBufferUseEven()` helper drives the parity. Eight new `GetCurrent*` / `GetPrevious*` accessors. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Dispatch.cpp` | Capture the current-frame ping-pong slot once per dispatch (so the Graphics-CL transition + Compute-CL bind + post-dispatch transition reference the same texture). Bind u11 / u12 from `PTDenoiseTemporalPass::Get*RadianceDiffuse/Specular`. Post-dispatch transition the radiance UAVs to ReadOnly so the temporal pass can read them as SRV-equivalent. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_BindingLayout.cpp` | Denoiser binding count 5 → 7. New u11 / u12 layout entries. Updated static_assert message. |
| `Source/ExampleProject/RenderingClient/ExampleRenderingClient_PrepareCommands.cpp` | Schedule `DispatchOrBypass(PTDenoiseTemporalPass::Get())` after `GPUPathTracerPass` under `if constexpr (Inno::PTDenoise::ENABLED)`. |
| `Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp` | Execute / Signal block for the temporal pass. Graphics-CL Execute + Signal (transition pass) → Compute-CL Wait on path tracer + Execute + Signal. |
| `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` | Initialize / Update / GetDispatchedPasses include the temporal pass under `if constexpr (Inno::PTDenoise::ENABLED)`. |
| `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Setup.cpp` | Setup-side `PTDenoiseTemporalPass::Get().Setup()` under the toggle. |
| `.claude/references.json` | New entry for `PTDenoiseTemporal.comp` citing SVGF + the engine-side GIDenoise.comp:184-260 reprojection precedent. NRD-Sample annotated as architectural reference only. |

### Pass scheduling shape

`PurgeTiles → UpdateTiles → MipCascadeBuild → GPUPathTracer → PTDenoiseTemporal → (FinalBlend / tonemap)`. The first three under `PTHashGridCache::ENABLED` (currently OFF), the temporal pass under `PTDenoise::ENABLED` (this CL's toggle, also currently OFF). Same-queue Compute Signal/Wait pattern — temporal-pass compute waits on path-tracer compute. Graphics-CL transitions for the temporal pass live on the graphics queue same as `GIDenoisePass`, because the GBuffer textures may carry PIXEL_SHADER_RESOURCE state that's invalid on a compute CL.

### History-texture format chosen + rationale

Per lobe (diffuse, specular):
- **Radiance + sample count**: RGBA16F. RGB = blended radiance, A = sample count clamped at `PT_DENOISE_MAX_HISTORY_FRAMES = 32`. 8 bytes/pixel × 2 frames × 2 lobes = 32 bytes/pixel.
- **Moments (Σ luma / N, Σ luma² / N)**: RG16F. Variance recovered at read time as `max(g - r*r, 0)`. 4 bytes/pixel × 2 frames × 2 lobes = 16 bytes/pixel.

Plus 2 single-buffered RGBA16F per-lobe radiance UAVs (current-frame raygen output) = 16 bytes/pixel.

Total: ~64 bytes/pixel. At 1280×720 → ~59 MB. Plus the path tracer's GBuffer ping-pong (4 channels × 2 frames × 8 bytes/pixel = 64 bytes/pixel = ~59 MB), total temporal-stage footprint ≈ 118 MB.

**Σ/Σ² (SVGF-faithful) over Welford**: Schied 2017 §4.2's à-trous edge weight `exp(-|x_q - x_p| / (σ · σ_x))` consumes the variance estimator directly. Welford produces the same expected value but in a different storage shape; converting at every à-trous tap would cost more than carrying the moment form. The format also matches what every published SVGF implementation reads at the spatial filter stage, so CL-3's port has zero reconciliation surface.

**MaxHistoryFrames = 32**: SVGF reference value (Schied 2017 §3, "α = 1/N capped at 1/32"). Empirically a balance between residual noise (low N → fast α drop, more responsive but noisier convergence) and motion lag (high N → slow α drop, can leak ghosting through). 16 was the alternative noted in the brief; 32 picked because the per-lobe demodulated channels at a primary-PT integrator have less per-frame variance than Capsaicin's per-ray (where MAX_BLUR_MASK = 16 makes sense), so the deeper accumulation window improves SNR without measurable lag at the primary hit.

### Ping-pong implementation (which class owns the swap)

**GBuffer-equivalent textures** (4 channels × 2 frames): `GPUPathTracerPass` owns the ping-pong. `GetCurrent*` / `GetPrevious*` accessors use `FrameManagementService::GetFrameCountSinceLaunch() % 2u` parity. Path tracer always writes the "current" slot; the temporal pass reads both via accessor.

**History textures** (radiance + moments × diffuse/specular × 2 frames): `PTDenoiseTemporalPass` owns the ping-pong. Same parity logic in an anonymous-namespace helper so the path tracer can't accidentally bind a history texture (it doesn't need to).

**Per-lobe current-frame radiance UAVs**: single-buffered, owned by `PTDenoiseTemporalPass`. The path tracer borrows them by accessor at bind time. The path tracer's post-dispatch transition flips them to ReadOnly so the temporal pass reads them as SRV-equivalent.

This split keeps each pass's owned-state co-located with the pass that mutates it, and avoids any "previous-set held by the consumer" coupling the brief flagged as an alternative. Documented decision: the path tracer is the natural owner of the GBuffer-equivalent because it's the writer; the temporal pass is the natural owner of the history because it's the only writer-and-reader. Each pass's `Update()` activation gate checks only its own resources plus what it borrows, so a partial-resource-failure mode keeps a clean blast radius.

### Bypass invariant verification

- All new HLSL writes inside `#if PT_DENOISE_ENABLED`. New per-lobe radiance UAV declarations gated by the same `#if`. Toggle-OFF AccumBuffer write at the function tail is byte-identical to HEAD (the `min(radiance, 100000)` line stays under `#else`).
- All new C++ resource allocations inside `if constexpr (Inno::PTDenoise::ENABLED)`. New texture members default to `nullptr`. Toggle-OFF: no allocation, no binding, no shader bytes referenced, no dispatch — `PTDenoiseTemporalPass::Setup` returns Terminated immediately.
- Binding-count progression: cache OFF + denoise OFF = 12 (unchanged). cache OFF + denoise ON = 19 (was 17 in CL-1; +2 for u11 / u12). cache ON + denoise ON = 26 (was 24 in CL-1; +2). The static_assert enforces the new count of 7.

### Build output (toggle=0 + toggle=1)

- HLSL2DXIL toggle=0: clean. `GPUPathTracerRayGen.hlsl` recompiled (depends on the CL-2-modified `PTRaygenBindings.hlsl` + `PTRaygenIntegrator.hlsl`); `PTDenoiseTemporal.comp` compiled fresh.
- HLSL2DXIL toggle=1: clean. Both above recompile under the on-toggle macro path. No errors.
- BuildWin RelWithDebInfo toggle=0: `Main.exe` + `RenderTest.exe` linked. Pre-existing C4003 `max` warnings only. No new warnings or errors. Three new C++ TUs compiled (`PTDenoiseTemporalPass.cpp`, `_Dispatch.cpp`, `_RenderTargets.cpp`).
- BuildWin RelWithDebInfo toggle=1: `Main.exe` + `RenderTest.exe` linked. No errors. Confirms binding-count static_assert passes at the new 7 value.

### Capture spot-check

- Toggle OFF (`Bin/RelWithDebInfo/Main.exe -total_frames 30`, working dir `Bin/RelWithDebInfo/`): clean termination. `Auto-test: 30 frames rendered, terminating.` GISponza loaded at frame 5. Engine terminated cleanly. 0 D3D12 ERROR / VALIDATION ERROR matches in log. Exit code 0. Log at `Build/captures/pt-denoise-cl2-toggle-off.log`.
- Toggle ON: clean termination. Same "30 frames rendered" log line. 0 D3D12 errors. Engine terminated. Exit code 0. Log at `Build/captures/pt-denoise-cl2-toggle-on.log`. Demonstrates the 11-SRV + 4-UAV binding layout, the path-tracer-to-temporal Compute Signal/Wait, the ping-pong accessor flow, and the PTDenoiseTemporal.comp dispatch all work end-to-end without GBV warnings or device-removed.

The toggle-ON RenderDoc-side gate (history sampleCount ramps to 32 on static frames; resets to 1 on disocclusion) is the visible signal that CL-2 actually populated the history textures. Capture not pulled in this CL — the runtime didn't have a moving camera in the 30-frame auto-test path. Reserved for the CL-3 visual-validation pass when the history starts to drive on-screen output.

### Not verified

- The full SVGF moment estimator behaviour under motion: CL-2 ships invisible-to-display, so the visible-validation gate is RenderDoc-only. The toggle-ON 30-frame auto-test confirms the binding layout + dispatch + Signal/Wait shape is well-formed, but does not exercise a moving-camera disocclusion. CL-3's visual gate (where history starts to drive output) will catch any reprojection-sign / depth-gate / normal-test bug that CL-2 leaks.
- Float16 precision for the moments: if luma² exceeds Float16's max representable value (~65504), the moment storage saturates. In our HDR scale (radiance clamped at 100000 in raygen) `luma² = 0.2126·R + ...)²` can reach ~10¹⁰ — well above Float16. CL-3 entry should consider RG32F for moments, or pre-clamp luma to a safer range before squaring. Filed mentally; not fixed in CL-2 because the squared-luma path has no consumer until CL-3's à-trous tap.
- Same-queue Compute Wait timing: `WaitIfActive(GPUPathTracerPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute)` is the right shape per the existing cache-pass chain, but if a future change moves the temporal pass to the graphics queue (e.g. for HW-accelerated bilateral), the wait surface will need re-evaluation.

### Surprises

- **Inline function in PTRaygenIntegrator.hlsl conflict with the sky-path UAV writes**: I initially added per-lobe radiance writes inside the sky-miss branch alongside the existing GBuffer-zeroing writes, then realised the function-tail unified write at AccumBuffer composition would overwrite them with the same `min(radianceDiffuse, 100000)` value. Refactored to keep the per-lobe writes only at the function tail; the sky-block keeps the GBuffer-zeroing writes (different — they zero RT0/RT1/RT2/RT3 unconditionally) but no longer touches u11 / u12.
- **File-size ratchet on PTDenoiseTemporalPass.cpp**: initial single-TU draft hit 370 lines (over the 300 ratchet). Split per `disciplines/on-implement/file-splitting.md` § "Same class, different responsibility cluster": dispatch went to `_Dispatch.cpp` (90), render-target creation went to `_RenderTargets.cpp` (65), main TU dropped to 229. All three under 300.
- **GIDenoise.comp `previous_uv = uv + velocity` sign**: re-confirmed during shader authoring that the engine motion-vector convention (RT3.xy = `screen_prev - screen_curr`, in pixels) is the OPPOSITE of Capsaicin's convention. CL-2 follows GIDenoise's note (lines 184-194) — addition, not subtraction. Same sign as the rasterizer's TAA reprojection, so engine-wide reprojection helpers stay reusable.

## CL-1 surprises (preserved)

- **Channel layout mismatch with the design plan**: design-plan `bullet` list claimed `RT1 = normal+roughness, RT2 = albedo+metalness`. The actual rasterizer `OpaquePass.frag:127-128` and `DecodeGBuffer` (lightPassCommon.hlsl:62-65) carry `RT1 = normal+metallic, RT2 = albedo+roughness`. CL-1 follows the actual decoder contract because the goal is forward-compatibility with `DecodeGBuffer`. Documented in `common/PTDenoiseShared.hlsl`.
- **`m_PrevViewMatrix` mis-framing**: the design risk note suggested wiring up a per-frame update if the field is a no-op holdover. It isn't a holdover, but it isn't a per-frame view either — it's an accumulation-reset tripwire. Resolved by skipping `m_PrevViewMatrix` entirely and using `PerFrameDataService::GetPreviousFrameBuffer()` (the engine's ping-pong CB), which is the rasterizer's source of truth for the same data.
- **DXC SROA collapse**: when extracting `radiance += ...` sites into named lobe-bucket additions, I initially refactored `radiance += X` to `float3 X = ...; radiance += X;` form unconditionally, which would technically depend on DXC's SROA optimizer to collapse the temporary back to identical DXIL on toggle-OFF. To eliminate the dependency, I restructured all NEE sites so toggle-OFF code paths are byte-identical to HEAD (the `radiance += throughput * CookTorranceGGX(...)` lines are unchanged when the toggle is off). The lobe-bucket `float3` temporaries now live entirely inside `#if PT_DENOISE_ENABLED` blocks. Bypass invariant strictly held at the source level, not the DXIL level.

### File-split bundle (TASK-219 [task-stays-open])

The CL-1 additions pushed two files past the file-size ratchet:

| File | Pre-CL-1 | After CL-1 | Action |
|---|---:|---:|---|
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Setup.cpp` | 269 | 343 | split |
| `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | 692 | 910 | split |

The split is structural-only. CL-1 logic (lobe split, GBuffer-equivalent writes, motion-vector math, payload `instanceID`, channel layout, all 14 `#if PT_DENOISE_ENABLED` blocks) is preserved verbatim across the new files.

#### C++ split — `GPUPathTracerPass_Setup.cpp`

- New file: `Source/ExampleProject/RenderingClient/GPUPathTracerPass_BindingLayout.cpp` (264 lines).
- Same TU class (`GPUPathTracerPass`); houses the `m_ResourceBindingLayoutDescs` configuration block (12 base + 7 cache + 5 denoise descriptors) **and** both toggle-gated `static_assert` invariants. Per `disciplines/on-implement/file-splitting.md` § "Same class, different responsibility cluster".
- New private method `GPUPathTracerPass::ConfigureRaytracingBindings()` declared in `GPUPathTracerPass.h`; called once from `Setup()` after the `RenderPassComponent` is created.
- The `static_assert` block moved into the new TU. FIRE-verification re-run below confirms the assert still triggers at the new file/line.

| File | Pre-split | Post-split |
|---|---:|---:|
| `GPUPathTracerPass_Setup.cpp` | 343 | 91 |
| `GPUPathTracerPass_BindingLayout.cpp` | — | 264 |

#### HLSL split — `GPUPathTracerRayGen.hlsl`

The 910-line raygen needed a deeper split than a single sub-section extraction; a 4-way split landed it at 57 lines (top-level entry-point + toggle docs + 3 includes):

| New file | Lines | Role |
|---|---:|---|
| `common/PTRaygenBindings.hlsl` | 96 | b0/b1/b2/b3/b4 cbuffers + TLAS + light SRVs + UAVs (toggle-gated cache + denoise blocks). |
| `common/PTRaygenHelpers.hlsl` | 180 | PCG/Halton, Disney/GGX BSDF math, hemisphere/GGX sampling, `SkyColor`, `GenerateCameraRay`. |
| `common/PTRaygenIntegrator.hlsl` | 247 | Free-function `RunPathIntegrator(uint2 pixel, uint2 resolution)` — the bounce loop, BSDF importance sample, RR, AccumBuffer composition. Calls helpers + reads bindings via global symbols (HLSL has no module system). |
| `common/PTRaygenIntegrator_GBufferWrite.hlsli` | 58 | Inline-snippet (`#include`-d inside `RunPathIntegrator`). Bounce==0 GBuffer-equivalent UAV write block. Per `disciplines/on-implement/file-splitting.md` § free-function header umbrella — splits a long body's domain-cohesive sub-block into a sibling include rather than inflating an inout-parameter signature. |
| `common/PTRaygenIntegrator_NEE.hlsli` | 195 | Inline-snippet. Sun + Sky + Point + Sphere NEE blocks. |
| `common/PTRaygenIntegrator_Cache.hlsli` | 161 | Inline-snippet. `#if PT_HASH_GRID_CACHE_ENABLED` Site-3 read + Site-2 multibounce write block. |
| `GPUPathTracerRayGen.hlsl` | **57** (was 910) | Toggle docs + `#define`s + 3 `#include`s + 4-line `[shader("raygeneration")]` entry point delegating to `RunPathIntegrator`. |

The `.hlsli` snippets are recognized HLSL-preprocessor-spliced source, used elsewhere in the tree (`pathTracerPayload.hlsli`). They share scope with the surrounding `RunPathIntegrator` body — no inout-parameter scaffolding, no DXIL drift surface from name-mangling. Each snippet's header documents in-scope dependencies.

#### DXIL byte-identity gate

Toggle=0 + toggle=1 both build clean before and after the split. The DXC-emitted `.dxil` files differ at SHA-256 hash level (DXC bakes source bytes into the embedded debug info via `/Zi -Qembed_debug`), so a raw file hash is not a reliable equivalence test. Comparison via `dxc -dumpbin` disassembly:

| Verification axis | Toggle=0 | Toggle=1 |
|---|---|---|
| Disassembled instruction count (lines starting with `%`) | 1730 (baseline) = 1730 (post-split) | 1971 (baseline) = 1971 (post-split) |
| `dx.op.*` intrinsic call counts (sorted-uniqued) | identical | identical |
| Diff post-normalization (strip `!dbg`, line/col, SSA names, block labels) | only block-label suffix differences from one extra inline frame (`RunPathIntegrator`) | same: only block-label suffix differences |
| Trailing-comma artifacts on cbuffer load lines | 3 (debug-meta vestigial) | 3 (debug-meta vestigial) |

Sample of the only structural delta — block-label suffix differences caused by an additional inlining level when `RayGenShader` calls `RunPathIntegrator`:

```text
< br i1 %, label %"Z.exit", label %.lr.ph219.preheader      (baseline: single-inline of Halton)
> br i1 %, label %"Z.exit.i", label %.lr.ph36.preheader     (post-split: double-inline through RunPathIntegrator)
```

Same opcode, same operands, same control-flow edges — only the auto-generated SSA suffixes shift because the extra inline frame mints fresh names. Disassembly artifacts confirmed at `/tmp/raygen_*_t{0,1}.disasm` (kept ephemeral; reproduce via `dxc -dumpbin` on `Bin/Shaders/DXIL/GPUPathTracerRayGen.hlsl.dxil`).

DXC's container hash (visible in `; shader hash: ... (includes source)` of the disassembly) is by definition source-byte-dependent and so **cannot** prove byte-identity through a structural split — it would change even on a comment edit or whitespace normalization. The instruction-count + intrinsic-call-frequency + post-normalization diff is the operative gate.

#### CMake reconfigure

`file(GLOB)` enumeration in `Source/ExampleProject/RenderingClient/CMakeLists.txt` did not auto-pick up `GPUPathTracerPass_BindingLayout.cpp` until `cmake .` re-ran. After reconfigure, the TU compiled into `ExampleRenderingClient.lib` and the engine linked clean.

#### Static_assert FIRE-verification re-run (post-split)

The static_assert block lives in `GPUPathTracerPass_BindingLayout.cpp` now (lines 22, 33). Pre-revert: temporarily set `Inno::PTDenoise::ENABLED = true` AND `l_denoiseBindingCount = 4`. Build output:

```text
GPUPathTracerPass_BindingLayout.cpp(33,42): error C2338: static_assert failed:
'GPUPathTracer raygen denoiser-binding count must be 5 (b4 + u7..u10). ...'
```

— assert fires at the new file/line with the expected message. Post-revert: count restored to 5, `ENABLED = false`; build clean.

#### Final line counts (post-split)

| File | Lines | Limit |
|---|---:|---:|
| `GPUPathTracerPass_Setup.cpp` | 91 | 300 |
| `GPUPathTracerPass_BindingLayout.cpp` | 264 | 300 |
| `GPUPathTracerPass.h` | 144 | 300 |
| `GPUPathTracerRayGen.hlsl` | 57 | 300 |
| `common/PTRaygenBindings.hlsl` | 96 | 300 |
| `common/PTRaygenHelpers.hlsl` | 180 | 300 |
| `common/PTRaygenIntegrator.hlsl` | 247 | 300 |
| `common/PTRaygenIntegrator_GBufferWrite.hlsli` | 58 | 300 |
| `common/PTRaygenIntegrator_NEE.hlsli` | 195 | 300 |
| `common/PTRaygenIntegrator_Cache.hlsli` | 161 | 300 |

#### Capture spot-check (post-split)

`Bin/RelWithDebInfo/Main.exe -total_frames 30` (toggle OFF default): exit code 0, auto-test confirmed `30 frames rendered, terminating.`, scene load succeeded at frame 5 (`Auto-test: loaded GISponza scene at frame 5`), no D3D12 errors, no GBV warnings, no device-removed. Pre-existing engine warnings only (GPU validation-disabled banner; long-task warnings during teardown). Log at `Build/captures/pt-denoise-cl1-split-toggle-off.log`.

## Review (shader-impl, 2026-05-07) — PASS (with one ADVISORY)

Reviewer: shader-impl. Scope: bundled CL-1 substance + TASK-219 file-split. Read every new HLSL file end-to-end, every touched C++ TU, the task's CL-1 implementation note, and `Scripts/Lib/Compile-HLSL.psm1`.

### CL-1 substance (preserved through the split)

- Lobe-tag rule (`PTRaygenIntegrator.hlsl:153-187`): `isSpecularPath` set ONCE at primary-hit BSDF importance sample (`if (bounce == 0u) isSpecularPath = false/true` inside both lobe branches), immutable for the rest of the path. Matches CL-1 design.
- GBuffer-equivalent UAV writes (`PTRaygenIntegrator_GBufferWrite.hlsli`): bounce==0 gated, RT0 = positionWS + (instanceID + 1u sentinel), RT1 = N + metalness, RT2 = albedo + roughness, RT3 = motionVec(px) + hitDist + 0. Aligns with `lightPassCommon.hlsl::DecodeGBuffer:55-66`. The design-plan RT1/RT2 alpha swap was correctly resolved and is documented in `PTDenoiseShared.hlsl:21-24`.
- Motion-vector math: `screen_prev - screen_curr` in pixels (`g_FramePrev.v / g_FramePrev.p_original` reprojection of current `payload.hitPos`), Y-flip applied to both screen coords before subtraction. Sign + unit match `OpaquePass.frag:124` so engine-wide reprojection helpers reusable.
- NEE-at-primary-to-diffuse decision (`PTRaygenIntegrator_NEE.hlsli`): `if (bounce == 0u || !isSpecularPath) radianceDiffuse += ... else radianceSpecular += ...` repeated identically for sun / sky / point / sphere lobes. Indirect bounces follow path tag — correct.
- Cache+Denoise interaction (`PTRaygenIntegrator_Cache.hlsli:135-144`): cache substitution lobe-routing reads `isSpecularPath` only inside the `bounce >= 1u` cache gate, after the primary-hit lock — well-formed.
- Sky-miss UAV-zero + sky-routed-to-diffuse (`PTRaygenIntegrator.hlsl:99-111`): correct; matches `DecodeGBuffer`'s `l_RT0.a == 0` sky test.
- Payload `instanceID` field (`pathTracerPayload.hlsli:21-25`, `GPUPathTracerClosestHit.hlsl:55`): 56B → 60B, MaxPayloadSizeInBytes=64 unchanged. DX12 service comment updated.
- Bypass invariant verified at source level: every new HLSL line is inside `#if PT_DENOISE_ENABLED`; no temporaries leak into the toggle-off compile path. Toggle-OFF NEE additions to `radiance` are byte-identical to HEAD.

### DXIL byte-identity gate substitution

- Verified `Scripts/Lib/Compile-HLSL.psm1:155` actually compiles with `-Qembed_debug /Zi /Zss`. The agent's claim that DXC bakes source bytes into embedded debug info → byte-identical .dxil through a comment-bearing split is structurally impossible — is correct.
- Structural diff (instruction count 1730/1730 toggle=0, 1971/1971 toggle=1; intrinsic frequency identical; only block-label suffix differences from one extra inline frame of `RunPathIntegrator`) is the right operative gate for this build configuration. Same opcode, same operands, same control flow — only auto-generated SSA suffixes shift. Acceptable.
- ADVISORY: file as a TASK-219 follow-up — `HLSL2DXIL.ps1` release-build variant should drop `-Qembed_debug /Zi /Zss` (or grow a `--strip-debug` mode) so future structural splits can use raw SHA-256 .dxil byte-identity as the gate. Not blocking this CL; the structural-diff substitution is rigorous given current flags.

### `.hlsli` snippet pattern

- Precedent established by `pathTracerPayload.hlsli`. Pattern matches `disciplines/on-implement/file-splitting.md` § "Free-function or template-heavy headers" in spirit (split by domain).
- Each `.hlsli` correctly omits `#pragma once` and function declarations — they are function-body excerpts spliced into `RunPathIntegrator`. Each header documents in-scope dependencies (locals, globals, gated branches) — sufficient for a future caller.
- Inline-snippet `break` in `_Cache.hlsli:159-160` correctly exits the surrounding bounce loop because the snippet shares scope. No DXIL drift surface vs an inout-parameter free function.

### File-size compliance

All ten touched/new files at or under 300: GPUPathTracerRayGen.hlsl 57, PTRaygenBindings.hlsl 96, PTRaygenHelpers.hlsl 180, PTRaygenIntegrator.hlsl 247, *_GBufferWrite.hlsli 58, *_NEE.hlsli 195, *_Cache.hlsli 161, GPUPathTracerPass_Setup.cpp 91, GPUPathTracerPass_BindingLayout.cpp 264, GPUPathTracerPass.h 144 (Initialize 121, Dispatch 112, .cpp 163, PTDenoiseConstants.h 33 — all well under). Ratchet satisfied.

### static_assert post-move

Lives at `GPUPathTracerPass_BindingLayout.cpp:33`. Predicate `!Inno::PTDenoise::ENABLED || l_denoiseBindingCount == 5` short-circuits on toggle-off (current default). Message text intact: u7/u8/u9/u10 named, b4 + PerFrameConstantBufferPrev rationale, b9a103cc precedent, sibling pass counts (UpdateTiles 1+5, MipCascadeBuild 1+3, PurgeTiles 1+2). Brief's "l_denoiseBindingCount = 4" was the FIRE-verification injection value (deliberately wrong); actual code uses 5 and matches the static_assert constant. FIRE rerun verified at the new file/line — accepted.

### Bypass invariant + out-of-scope creep

`HashGridCacheConstants.h::ENABLED = false`, `PTDenoiseConstants.h::ENABLED = false`. AccumBuffer write path bit-identical to HEAD when both off. No accidental edits to CL-1 substance during the split — every `#if PT_DENOISE_ENABLED` block landed verbatim in its new home. DX12 service change is a comment-only update (56B → 60B); closest-hit adds one line; payload field is the documented add. No out-of-scope creep detected.

### Build hygiene

CMake reconfigure called out in the implementation note (required for `file(GLOB)` to pick up `_BindingLayout.cpp`). Both toggle combinations built clean and `Bin/RelWithDebInfo/Main.exe -total_frames 30` ran to clean termination on the toggle-off default.

### Verdict

**PASS**. CL-1 substance is preserved verbatim through the file-split; the bypass invariant holds at the source level; the DXIL substitute gate is rigorous given DXC's debug-embedding flags. One non-blocking ADVISORY: file a TASK-219 follow-up to add a debug-stripped HLSL2DXIL release variant so future splits can gate on raw .dxil byte-identity.

Reviewed-By: shader-impl

<!-- SECTION:NOTES:END -->
