---
id: TASK-136
title: >-
  Systemic: audit graphics subsystems for per-consumer-duplicated constants that
  mirror HLSL canonical values
status: To Do
assignee: []
created_date: '2026-04-25 22:25'
labels:
  - rendering
  - refactor
  - systemic-hygiene
  - constants
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/RadianceCacheConstants.h
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from the TASK-127 retrospective (2026-04-25). The 2/3 GI cutoff bug was caused by a stale `SH_TILE_SIZE = 2` constant duplicated per-pass in `Source/ExampleProject/RenderingClient/RadianceCache*Pass.h` headers, drifting from the HLSL canonical `SH_TILE_SIZE = 3` in `Source/Shaders/HLSL/RayTracingTypes.hlsl`. The C++ side allocated the SH atlas at `probeGrid * 2`; the HLSL wrote/read at `probeIndex * 3`; result was OOB writes (UAV returned zero) for probes past the 2/3 boundary.

The fix consolidated the constants into a single `RadianceCacheConstants.h` namespace shared across all GI passes. Per `feedback_systemic_not_local.md`, this drift class is almost certainly not unique to the GI subsystem.

### What to audit

Search `Source/ExampleProject/RenderingClient/*.h` (and any other graphics-pass headers) for `const uint32_t` / `static constexpr` member constants that mirror values defined canonically in `Source/Shaders/HLSL/` (especially `RayTracingTypes.hlsl`, `BRDF.hlsl`, `common/*.hlsl`).

### Likely candidates to investigate (not exhaustive)

- **Shadow CSM split count** — split count likely defined in the shadow pass class AND in the shadow shaders
- **Light culling tile size** — per-pass tile constant likely duplicated between `lightCulling.comp` and `LightCullingPass.h`
- **BRDF LUT extent** — LUT dimensions likely duplicated between the LUT generator pass class and the consumer shader
- **TAA history extent / jitter sample count** — TAA pass class constants vs `TAAPass.cpp` shader-side
- **Probe / volume dimensions for any other voxel/volume pass** (irradiance volumes, fog, etc.)

### Deliverable

For each subsystem found with the drift pattern: consolidate the constants into a single `<Subsystem>Constants.h` header analogous to `RadianceCacheConstants.h`, with comments anchoring the contract to the HLSL canonical file. Validate that the C++ value matches the HLSL value pre-consolidation (i.e. no live drift) — if any drift IS found, file a task per finding with a captured-symptom screenshot before fixing.

### Why medium priority, not low

Drift bugs from this class are silent until they manifest as visible artifacts (TASK-127 was visible only because the result was a large rectangular cutoff; smaller drifts can hide for years). Consolidating now prevents the next instance.

### Constraint

Behavior-preserving consolidation only. Any value-change uncovered during the audit is a separate task, not folded into this one.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Source/ExampleProject/RenderingClient/*.h grepped for `const uint32_t` / `static constexpr` member constants — inventory in final summary
- [ ] #2 Each found constant cross-referenced against HLSL canonical (RayTracingTypes.hlsl / BRDF.hlsl / common/*.hlsl) — match or drift status quoted per constant
- [~] #3 For each subsystem with the drift pattern: a `<Subsystem>Constants.h` header consolidates the constants with HLSL-canonical anchor comments — screen-tile 8×8 landed (2026-05-15); 5 follow-up subsystems remain (TASK-225 SSAO drift, light-culling 16×16, point/sphere light arrays, MaxTextureSlotCount, luminance histogram 256 + reduction 16×16, BRDF LUT 512)
- [ ] #4 Any live drift discovered (i.e. C++ value != current HLSL value) filed as its own task with symptom screenshot, NOT silently fixed in this CL
- [ ] #5 Final summary lists subsystems audited AND subsystems explicitly skipped (with reason)
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

## Implementation Notes

### 2026-05-14 — Audit phase (inventory only)

AUDIT-ONLY dispatch — no source modified, no engine launched. Scope: AC #1, #2, #5 of this ticket. Consolidation (#3) and drift-fix-CL (#4) deferred to follow-up dispatches.

#### Constant inventory — C++ ↔ HLSL cross-reference

Drift status legend: `match` = same value, duplicated literally; `drift` = different values (live bug); `cpp-only` = no HLSL mirror; `hlsl-only` = no C++ mirror; `consolidated` = already factored into a `*Constants.h` header (TASK-127 / TASK-77.x prior work).

| Subsystem | C++ site | C++ value | HLSL site | HLSL value | Status |
|---|---|---|---|---|---|
| RadianceCache TILE_SIZE | `RadianceCacheConstants.h:22` | `TILE_SIZE = 8u` | `RayTracingTypes.hlsl:5`, `common/RadianceCacheCommon.hlsl:14` (named `RADIANCE_CACHE_TILE_SIZE`) | `8` | consolidated (TASK-127) |
| RadianceCache SH_TILE_SIZE | `RadianceCacheConstants.h:31` | `SH_TILE_SIZE = 3u` | `RayTracingTypes.hlsl:6` | `3` | consolidated (TASK-127) |
| RadianceCache UPSCALE_X/Y | `RadianceCacheConstants.h:37-38` | `2u/2u` | — | — | consolidated; HLSL anchor via comment in header |
| PTHashGridCache buckets/tiles/cells | `HashGridCacheConstants.h:45-67` | full ladder | `common/PTHashGridCache.hlsl` `#define` block | matches | consolidated (TASK-77.x) |
| PTDenoise ENABLED | `PTDenoiseConstants.h:30` | `true` | `GPUPathTracerRayGen.hlsl::PT_DENOISE_ENABLED` | `1` | consolidated (TASK-77.x) |
| NRD ENABLED | `NRDConstants.h:36-40` | CMake-gated | — (CMake compile-def `INNO_BUILD_WITH_NRD`) | — | consolidated (TASK-77.4) |
| Light culling tile size | `LightCullingPass.h:44` `m_tileSize=16`, `TiledFrustumGenerationPass.h:28-29` `m_tileSize=16, m_numThreadPerGroup=16` | `16` | `common/common.hlsl:69` `#define LIGHT_CULLING_BLOCK_SIZE 16`, used in `lightCulling.comp:62,103,133`, `tileFrustum.comp:27,35-44`, `lightPass.comp:137`, `common/lightPassDirectLighting.hlsl:92` | `16` | match (per-consumer duplication; 2 C++ headers, 1 HLSL canonical, ~10 HLSL use sites) |
| Point light array size | `Engine/Services/RenderingConfigurationService.cpp:20` `maxPointLights=1024` | `1024` | `common/common.hlsl:26` `static const int NR_POINT_LIGHTS = 1024` | `1024` | match (literal duplicated; HLSL canonical sizes cbuffer arrays in `lightCulling.comp:21`, `lightPass.comp:34`) |
| Sphere light array size | `Engine/Services/RenderingConfigurationService.cpp:21` `maxSphereLights=128` | `128` | `common/common.hlsl:27` `static const int NR_SPHERE_LIGHTS = 128` | `128` | match (same shape as point lights) |
| BRDF LUT extent | `BRDFLUTPass.cpp:23-24`, `BRDFLUTMSPass.cpp:25-26` (`Width=512, Height=512`) + dispatch `(32,32,1)` at lines 110/115 | `512` (and derived 32 = 512/16) | `BRDFLUTPass.comp:264-270` `[numthreads(16,16,1)]` + literal `512.0` divisor; `BRDFLUTMSPass.comp:48,55` `[numthreads(16,16,1)]` + `textureSize=512u` | `512` | match (5+ literal duplications, no anchor) |
| BRDF LUT sample count | — | — | `BRDFLUTPass.comp:170` `const uint SAMPLE_COUNT = 1024u` | `1024` | hlsl-only |
| TAA dispatch tile size | `TAAPass.cpp:154` `viewportSize / 8.0f` | `8` | `TAAPass.comp:40` `[numthreads(8,8,1)]` | `8` | match (literal duplicated; this is one of ~10 screen-space passes with the same 8x8 anchor) |
| TAA jitter source | `Engine/Services/PerFrameDataService.cpp:156-157` Halton(3) / Halton(4) | bases 3,4 | — | — | cpp-only (jitter generation is CPU-side; shader reads `p_jittered` from CB) |
| TAA tuning constants | — | — | `TAAPass.comp:29-38` (MOTION_VECTOR_CLAMP, ADAPTIVE_CLAMP_MIN/MAX, LUMA_DIFF_*, HISTORY_BLEND_*, etc.) | various | hlsl-only |
| SSAO kernel size | `SSAOPass.h:28` `m_kernelSize=64`, `SSAOPass.cpp:107,109,114,127` | `64` | `SSAONoisePass.comp:32` `sampleCount=32`, `:48` array `[64]`, `:125` loop bound `sampleCount`, `:169` divisor `sampleCount` | `32` (loop), `64` (array) | **DRIFT — filed as TASK-225** |
| SSAO noise tex extent | `SSAOPass.cpp:131,148,149` `l_textureSize=4` | `4` | `SSAONoisePass.comp:85` `readCoord / float2(4.0, 4.0)` | `4.0` | match (literal duplicated 3x in C++ + 1x in HLSL) |
| SSAO sample radius / bias | — | — | `SSAONoisePass.comp:33,34` `radius=0.5f`, `bias=0.05f` | — | hlsl-only |
| Luminance histogram bins | `LuminanceHistogramPass.cpp:64` `m_ElementCount=256` | `256` | `luminanceHistogramPass.comp:26` `sharedHistogram[256]`, `:35` `*254.0+1.0`, `:67` loop bound `256`, `:38` `[numthreads(16,16,1)]` | `256` | match (literal `256` duplicated 4x in HLSL + 1x in C++; `[numthreads(16,16,1)]` ↔ `viewportSize/16` at cpp:126-127) |
| Luminance history slots | `LuminanceAveragePass.h:26` `m_MaxResultToKeep=8` | `8` | `luminanceAveragePass.comp:18` `numHistoryFrames=8` | `8` | match (literal duplicated; shader uses `(frameIndex % 7)+1` for slot 1..7 plus slot 0 for adapted output — 8 total) |
| Luminance reduction block size | dispatch `(1,1,1)` at `LuminanceAveragePass.cpp:139` | implicit | `luminanceAveragePass.comp:27` `[numthreads(16,16,1)]` (single group reads 256 bins) | `16` | match (implicit pairing — block 16x16 = 256 threads = 256 histogram bins; the 256 dependency is the histogram one above) |
| Compute culling thread group | `ComputeCullingPass.cpp:134` `constexpr kThreadGroupSize=64` (with comment "Must match THREAD_GROUP_SIZE in the .comp shader") | `64` | `opaqueGPUCulling.comp:24` `#define THREAD_GROUP_SIZE 64`, `:74` `[numthreads(THREAD_GROUP_SIZE,1,1)]` | `64` | match (explicit "must match" comment is the smell flag) |
| Screen-tile size (8x8) | `LightPass.cpp:343`, `SkyPass.cpp:115`, `FinalBlendPass.cpp:173`, `GIFilterHorizontalPass.cpp:142`, `GIFilterVerticalPass.cpp:146`, `PreTAAPass.cpp:139`, `PostTAAPass.cpp:125`, `TAAPass.cpp:154`, `SSAOPass.cpp:236` — all `viewportSize.* / 8.0f` | `8` | `lightPass.comp:151`, `skyPass.comp:22`, `finalBlendPass.comp:57`, `preTAAPass.comp:19`, `postTAAPass.comp:17`, `TAAPass.comp:40`, `SSAONoisePass.comp:73`, `RadianceCacheFilterHorizontal.comp:46`, `RadianceCacheFilterVertical.comp:38`, `RadianceCacheReprojection.comp:112`, `GIDenoise.comp:136`, `GIFilterCommon.hlsl:94`, `PTNRDFormatConvert.comp:130`, `PTNRDComposition.comp:121`, `GPUPathTracerToneMap.hlsl:25`, `mipmapGenerator2D.comp:40` — all `[numthreads(8,8,1)]` | `8` | match (the most-duplicated literal in the audit: ~9 C++ sites × ~15 HLSL sites = ~24 occurrences with no anchor) |
| MaxTextureSlotCount | `Engine/Common/GPUDataStructure.h:113` `const uint32_t MaxTextureSlotCount = 7` | `7` | `common/common.hlsl:131` `static const uint MaxTextureSlotCount = 7` | `7` | match (literal duplicated; struct array sizes derive from it on both sides) |
| INVALID_TEXTURE_INDEX | `Engine/Common/GPUDataStructure.h:8` `0xFFFFFFFF` | sentinel | `common/common.hlsl:24` `0xFFFFFFFF` | sentinel | match (literal duplicated) |
| Sun angular radius | — | — | `common/common.hlsl:37` `SUN_ANGULAR_RADIUS=0.00465` | — | hlsl-only (consumed by `sunSampling.hlsl`, `SunShadowRTRayGen.hlsl`, `lightPassDirectLighting.hlsl`) |
| Path tracer max bounces | — | — | `common/PTRaygenIntegrator.hlsl:84` `const uint MAX_BOUNCES = 4` | `4` | hlsl-only |
| Debug-view enum | `Engine/Common/GPUDataStructure.h:30-37` `enum class DebugViewMode` | 5 values | `common/common.hlsl:82-86` `#define DEBUG_VIEW_*` | 5 values | match (enum mirrored across both sides; the comment in common.hlsl:71-81 acknowledges the duplication) |
| Tile-light heatmap ceiling | — | — | `common/common.hlsl:90` `#define DEBUG_VIEW_TILE_LIGHT_HEATMAP_MAX 16u` (tied to 16-entry `debugColors[16]` at :40) | `16` | hlsl-only (consumer is the debug-view branch in `lightPassDirectLighting.hlsl`) |
| GI Denoise MaxBlurMask | — | — | `GIDenoise.comp:101` AND `common/GIFilterCommon.hlsl:34` both define `kGIDenoiser_MaxBlurMask = 16.0` | `16.0` | match (HLSL-internal duplication across two shader files; same drift class, different domain — flag for the consolidation CL) |
| Debug mesh / material caps | `DebugPass.h:51-52` `m_maxDebugMeshes=65536, m_maxDebugMaterial=512` | — | — | — | cpp-only (CPU-side buffer cap; no shader-side enforcement) |
| BSDFTest sphere count | `BSDFTestPass.h:31` `m_shpereCount=10` | — | — | — | cpp-only |

#### Drift findings

**1 live drift found, 1 ticket filed:**

- **TASK-225** — SSAO kernel-count drift (C++ allocates 64 samples, shader iterates 32). Filed under `.backlog/tasks/task-225 - SSAO-kernel-count-drift-C++-allocates-64-shader-reads-32.md`. Screenshot deferred to consolidation CL per AC #4 (audit dispatch did not run the engine).

#### Subsystems skipped — with reason

- **VolumetricPass / voxel passes** — `ExampleRenderingClient_Setup.cpp:276` shows `// VolumetricPass::Setup();` commented out. `Source/Shaders/HLSL/WIP/volumetric*.comp` shaders reference undefined symbols (`dispatchParams`, `perFrameCBuffer`, `pointLights`) indicating they are inactive WIP. No live drift class because not active.
- **MotionBlurPass shader** — `WIP/motionBlurPass.comp:26` references undefined `perFrameCBuffer`; dispatch in `MotionBlurPass.cpp:119` is commented out. Inactive WIP.
- **GI bake passes** (`WIP/GIBake*.frag`, `WIP/GIResolve*.comp`, `WIP/voxel*`) — all under `Shaders/HLSL/WIP/`, no active C++ pass driving them.
- **CSM (Cascade Shadow Maps)** — explicitly removed per TASK-138 (`SunShadowRTPass.h:8-14` notes "sole sun-shadow path after the CSM+PCSS swap landed", `common.hlsl:43-45` notes "HLSL register b3 (was CSMCBuffer) is intentionally unused"). No CSM-side constants remain to audit.
- **Animation / Billboard / Debug / FinalBlend / TransparentBlend / TransparentGeometryProcess / OpaquePass / OpaqueCullingPass / GPUPathTracer / PTHashGrid passes** — examined; no per-consumer mirror constants of HLSL canonical values beyond the screen-tile-size (8x8) and light-culling (16x16) entries already tabled above. PTHashGrid and RadianceCache are consolidated already.

#### Recommendation — next dispatch's first consolidation target

**Screen-tile size 8×8** is the highest-value first consolidation target:

- **Largest surface area** (24 occurrences across 9 C++ files and 15 HLSL files — 4× the duplication of the next-largest item).
- **Most likely future drift site** — any agent who tweaks one pass's thread-group size will touch the literal in 1 of 24 places; nothing forces them to update the other 23. The TASK-127 precondition.
- **Behavior-preserving consolidation is safe** — all 24 occurrences are the same value (8), so introducing a single `RenderTileConstants::SCREEN_TILE_SIZE = 8u` is a pure refactor with no behavioural risk. No new ticket needed.
- **Lowest-risk first land** — establishes the consolidation pattern under a non-drift case before the next dispatch tackles the SSAO drift (TASK-225) where a behaviour choice is required.

After 8×8 consolidation, secondary targets in priority order:

1. **TASK-225 (SSAO)** — drift requires a value-pick decision; do it second so the pattern is already established.
2. **Light culling tile size 16×16** — second-most-duplicated literal (~6 occurrences), value-match.
3. **NR_POINT_LIGHTS / NR_SPHERE_LIGHTS / maxPointLights / maxSphereLights** — couple the runtime `RenderingCapability` cap to the HLSL cbuffer array size so changing one without the other becomes a compile error.
4. **MaxTextureSlotCount** — already declared identically on both sides; trivial single-source consolidation.
5. **Luminance histogram bins (256) + reduction block (16×16)** — pair of dependent constants; consolidate together.
6. **BRDF LUT extent (512)** — duplicated 5 times in two shader-class pairs.

The systemic ticket TASK-136 should reuse this priority list when scheduling per-subsystem consolidation CLs.

### 2026-05-15 — Consolidation #1 (screen-tile 8×8)

Behaviour-preserving consolidation of the 8×8 screen-tile dispatch literal across 9 RenderingClient passes.

**Header**: `Source/ExampleProject/RenderingClient/ScreenTileConstants.h` — `Inno::ScreenTile::SCREEN_TILE_SIZE = 8u`. Placement matches the `RadianceCacheConstants.h` precedent (every consumer is a `RenderingClient` pass; no Engine-side consumer exists today). Comment anchors the contract to the HLSL canonical literal `[numthreads(8,8,1)]` in `Source/Shaders/HLSL/lightPass.comp:151` and lists the 15 sibling shaders sharing the same group size.

**C++ sites migrated** (8, all `uint32_t(viewportSize.* / 8.0f)` → `uint32_t(viewportSize.* / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE))`):

- `SkyPass.cpp:116`
- `FinalBlendPass.cpp:174`
- `GIFilterHorizontalPass.cpp:143`
- `GIFilterVerticalPass.cpp:147`
- `PreTAAPass.cpp:140`
- `PostTAAPass.cpp:126`
- `TAAPass.cpp:155`
- `SSAOPass.cpp:237`

Each consumer also gained `#include "ScreenTileConstants.h"` immediately after its self-header include. `MotionBlurPass.cpp:119` left untouched — the dispatch is commented out (inactive WIP per the 2026-05-14 audit's subsystems-skipped list).

**Deferred at commit time — `LightPass.cpp:344`**: the file is 399 lines, over the 300-line ratchet; adding the `#include "ScreenTileConstants.h"` would push it to 400. Commit-gate blocks. Same shape as TASK-198 batch 1's LightPass deferral. Defer to TASK-135 (LightPass shader refactor) or a separate split CL.

**HLSL untouched** — the convention (matching `RadianceCacheConstants.h`) anchors C++ to HLSL via comment, not vice versa.

**Bit-identical replacement**: `static_cast<float>(8u) == 8.0f` exactly; every consolidated site computes the same `uint32_t` dispatch extent as before. `SCREEN_TILE_SIZE` is `constexpr`; `static_cast<float>` is a compile-time constant.

**Validation**:

- Build: `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` → ExitCode 0; `Main.exe` and `RenderTest.exe` linked.
- Smoke run: `Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 30 -offscreen` (CWD = `Bin/`) → ExitCode 0; log `Bin/[2026-5-15-18-38-17-26].Log` line 300 `Auto-test: 30 frames rendered, terminating.`; no `D3D12 ERROR` / `CORRUPTION` / `Validation Error` lines; tail line `Engine has been terminated.`

**File-size ratchet**: `LightPass.cpp` migration deferred (see above — gate blocks at 400 lines). All touched files in the landing set remain under 300.

**ACs**: only AC #3 ticked, and only for this subsystem. AC #1, #2, #5 remain open for the follow-up consolidations queued in the priority list above (TASK-225 SSAO drift fix → light-culling 16×16 → point/sphere light arrays → MaxTextureSlotCount → luminance histogram + reduction → BRDF LUT 512). Task stays `In Progress`.
