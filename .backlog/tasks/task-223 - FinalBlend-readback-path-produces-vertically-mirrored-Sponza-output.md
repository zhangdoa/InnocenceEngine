---
id: TASK-223
title: FinalBlend / readback path produces vertically-mirrored Sponza output
status: Done
assignee: []
created_date: '2026-05-13 22:28'
updated_date: '2026-05-17'
labels:
  - rendering
  - bug
  - readback
  - finalblend
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

`Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30` (with or without `-offscreen` / `-gpu_validation`) auto-captures a `gpu_output.png` where the rendered Sponza scene is **structurally corrupted**:

- **Vertical mirror-seam** down the image centerline; left and right halves do not cleanly join. The right half appears to be a flipped duplicate of the left, not the actual right side of the scene.
- **Large triangular geometry blob** in the lower-center (white at pre-existing baseline, green-tinted post-TASK-222 fix when the readback samples correct content). Probably an untextured / NaN-shaded mesh.
- **Duplicated curtain / arch geometry** flanking both sides.

Stone arches and brick masonry are readable; the path tracer is producing coherent surfaces. The corruption is structural (geometry layout / viewport / blit), not a denoiser or shading artifact.

## Pre-existing scope

Surfaced during TASK-222 visual validation (2026-05-14). Bisect-via-stash confirmed: present at pre-CL HEAD in **both** presentation and offscreen modes. Pre-TASK-222 captures appeared dimmer/grey because the broken state-tracker made the readback sample mismatched data; with TASK-222 landed, the readback content is brighter and accurate, making the structural corruption clearly visible.

This bug was masked until now because:

- Under `-gpu_validation`, the D3D12 validation layer killed the engine before the PNG flush (TASK-222 fixed that).
- Without `-gpu_validation`, the readback silently produced dim/desaturated garbage that nobody scrutinised closely — early visual-validation agents read the captures as "clean Sponza" because the dominant content (recognizable curtains/arches) was present, and the mirror-seam wasn't called out.

## Likely root-cause space (unverified)

1. **Viewport / scissor halving** — the path tracer or FinalBlend pass renders into only half the width, then the readback or composition pass mirrors / duplicates it. Possible since `DispatchRays` width arguments or `RSSetViewports` setup is sometimes copy-pasted.
2. **Stereo / multi-view path mistakenly active** in non-VR runs — engine might have a leftover VR / split-screen branch that fires even when only one eye / view is configured.
3. **`ReadTextureBackToCPU` row-pitch / format handling** — DXGI `D3D12_RESOURCE_DESC` row pitch padding vs the readback buffer's tightly-packed write could halve-stride the image. Channels-swapped tint observed post-TASK-222 hints at a related DXGI_FORMAT vs UAV-format mismatch in the readback CPU-side copy.

## Reproduction

```
Scripts/BuildWin.ps1 -SkipShaderCompile
Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30
# Inspect: Bin/RelWithDebInfo/gpu_output.png
```

Compare visually to `C:/GitRepo/InnocenceEngine/Build/baseline-presentation.png` (pre-TASK-222 HEAD) or `current-CL-presentation.png` (post-TASK-222).

## Deliverables

- Identify which stage of the pipeline introduces the mirror-seam.
- Confirm whether the green/white triangular blob is shared root-cause or a separate issue.
- Fix and re-capture: PNG shows a single coherent Sponza render (no vertical mirror, no untextured blob).

## Cross-references

- TASK-222 (FinalBlend readback transition under `-gpu_validation`) — the fix that exposed this bug; do not roll back. Reference PNGs:
  - `C:/GitRepo/InnocenceEngine/Build/baseline-presentation.png` (pre-CL, dim corrupt)
  - `C:/GitRepo/InnocenceEngine/Build/baseline-offscreen.png` (pre-CL, dim corrupt)
  - `C:/GitRepo/InnocenceEngine/Build/current-CL-presentation.png` (post-CL, bright corrupt with channel tint)
- Skill `visual-validation` — discipline that surfaced this (numeric metrics + dominant-content description never substitute for full visual inspection).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 `Main.exe -total_frames 30` capture shows single coherent Sponza render — no vertical mirror-seam, no duplicated geometry, no untextured triangular blob
- [ ] #2 #2 Result holds in both presentation and `-offscreen` modes
- [ ] #3 #3 Pre-existing integration tests covering rendering remain green
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

### 2026-05-14 — Audit (code-impl)

**Verdict:** the ticket framing is wrong. There is no rendering bug producing a vertical mirror; the visible "mirror" is the geometric symmetry of the GISponza atrium photographed straight down its bilateral symmetry axis with a 90° FOV. The "triangular blob" is the Stanford Dragon entity (`GISponza.Dragon`) at world origin, scaled 10×, textured by `Ground037_Color` (moss / ground albedo) per `Data/ExampleProject/Components/GISponza.Dragon.MaterialComponent.json` — that is a content/asset-pairing issue, orthogonal to readback / blit / viewport plumbing.

The corruption hypotheses in the ticket (viewport halving, stereo, row-pitch / format) are each refuted below by file:line citation.

#### Pixel-level confirmation

Both `Build/baseline-presentation.png` and `Build/current-CL-presentation.png` are 1280×720 RGBA — full screen resolution, no halving. Quantitative symmetry checks (Python on the PNGs):

- Adjacent-column discontinuity at x=640 (alleged seam): mean L1 delta = 1.1; column-wise median = 3.1. **No discontinuity at the centerline.**
- Column-pair `(x, W-1-x)` (true horizontal-mirror predicate) over nontrivial-luminance pixels: identical-pct = 0.08 / 0.10 / 0.09 across baseline-presentation / baseline-offscreen / current-CL. **The right half is NOT a pixel-mirror of the left half.**
- Adjacent-pair column duplication (would indicate half-width upscale): only 66/640 column pairs >90% identical — not a 2× upscale.
- Quadrant means in current-CL: Q1 [136,153,154], Q2 [53,54,59], Q3 [133,136,128], Q4 [100,100,104] — asymmetric, ruling out true mirror.

What looks like a "mirror seam" to the human eye is the **architectural bilateral symmetry of the Sponza atrium** viewed straight down its long axis from `(0, 2, 0)` with the long axis aligned to the world Z and the columns mirrored across the X=0 plane.

#### Camera root cause

`Data/ExampleProject/Components/GISponza.Camera.TransformComponent.json:3-13`:

```
Position: (0, 2, 0)
Rotation: identity (W=1)
```

`Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json:3-6`:

```
FOVX: 90.0
WidthScale: 16, HeightScale: 9
```

Identity rotation means the view direction is `Direction::Backward` rotated by identity (`Source/Engine/Services/CameraService.cpp:57`), i.e. along ±Z. The Sponza & Curtains entities both sit at origin with identity transforms (`Data/ExampleProject/Components/GISponza.Sponza.TransformComponent.json`, `…Curtains.TransformComponent.json`). The atrium model is built bilaterally symmetric about its X=0 plane, so a camera at `(0,2,0)` with identity rotation and 90° FOV is, by construction, photographing a near-mirror-symmetric view of the atrium — by scene design, not pipeline bug.

For comparison, `Build/captures/three_scene_20260514_211007/gisponza/gpu_output_0026.png` (same engine, same scene, same renderer) shows a clean Sponza render — driven by `TestPathTracerThreeScenes.ps1` with `-camera_orbit 20,8,120`, which overrides the scene-file camera with an off-axis orbiting viewpoint (`Source/ExampleProject/LogicClient/World.inl:326-329`). The bilateral symmetry of the atrium is invisible from that angle.

#### Per-hypothesis findings

**H1 (viewport / dispatch halving)** — **refuted.**

- FinalBlend dispatch: `Source/ExampleProject/RenderingClient/FinalBlendPass.cpp:173` — `Dispatch(l_viewportSize.x / 8.0f, l_viewportSize.y / 8.0f, 1)` = 160×90 groups × 8×8 threads = 1280×720. No `/2` or `>>1`.
- All other compute passes (`SkyPass`, `PreTAAPass`, `TAAPass`, `LightPass`, `SSAOPass`, `MotionBlurPass`, GI filter passes, etc.) dispatch the same `/ 8.0f` shape — full screen, no halving.
- Path tracer `DispatchRays(l_resolution.x, l_resolution.y, 1)` at `GPUPathTracerPass_Dispatch.cpp:125`. Also full screen. (The PT is not active in this run anyway — see §"Active pipeline path" below.)
- The single engine `RSSetViewports` call at `Source/Engine/Services/DX12/DX12FrameManagementService_RenderTargets.cpp:159` passes `count=1, &PSO->m_Viewport`. Width/Height populated from `viewportDesc.m_Width/Height` at `Source/Engine/Services/DX12/DX12Helper_Pipeline_Viewport.cpp:7-8`, which are seeded by `RenderingConfigurationService.cpp:38-39` directly from `m_screenResolution`. No `/2` or `*0.5` anywhere.

**H2 (stereo / multi-view)** — **refuted definitively.**

- Engine-source grep for `stereo|multiview|leftEye|rightEye|numViews|StereoEye` (case-insensitive, excluding `External/` submodules): **zero matches.** All hits are inside `Source/External/GitSubmodules/{renderdoc, imgui, assimp, …}` — third-party code never invoked by the engine.
- Only `RSSetViewports` callers in tracked engine code: ImGui DX12 backend (one viewport, full window) and `DX12FrameManagementService_RenderTargets.cpp:159` (one viewport from PSO). No code path sets `RSSetViewports(2, …)`.

**H3 (`ReadTextureBackToCPU` row-pitch / format / y-flip)** — **refuted for the specific FinalBlend texture used.**

The FinalBlend Result texture inherits from `RenderingConfigurationService.cpp:29-36`:
- `Sampler = Sampler2D`
- `PixelDataFormat = RGBA`
- `PixelDataType = Float16`
- `Width = 1280, Height = 720`

Then `FinalBlendPass.cpp:203-204` overrides `Usage = ComputeOnly` and creates the texture. `Usage` is treated as compute-UAV by `GetTextureFormat` (`Source/Engine/Services/DX12/DX12Helper_Texture_Format.cpp:149-159`) — falls through to the Float16/RGBA branch returning `DXGI_FORMAT_R16G16B16A16_FLOAT`. 8 bytes per pixel.

Readback at `Source/Engine/Services/DX12/DX12TextureResourceService_Readback.cpp`:
- Line 41: `GetCopyableFootprints` writes the correct subresource footprint; `Footprint.Width = 1280`, `Footprint.Height = 720`, `Footprint.RowPitch` aligned up to `D3D12_TEXTURE_DATA_PITCH_ALIGNMENT` (256).
- For 1280-pixel Float16 RGBA rows: tight-pack row = `1280 × 8 = 10240` bytes. `10240 % 256 == 0`, so the row-pitch is exactly 10240 — no padding. The CPU loop's `l_PixelData = l_srcRow + col * l_pixelDataSize` (line 161) with `l_pixelDataSize = 8` strides correctly.
- Float16 channel decode at lines 197-208: `channels = l_pixelDataSize / 2 = 4` for RGBA Float16. Reads 4×2 bytes per pixel. Correct.
- `l_dstIndex = row * textureDesc.Width + col` (line 170): top-down ordering, matching D3D12's resource layout and PNG's row order. No flip.
- `WriteCaptureToFile` at `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp:207-216`: rebuilds an `uint8_t` RGBA8 buffer in the same top-down row order; passes `Width=1280, Height=720, Sampler=Sampler2D, PixelDataFormat=RGBA, PixelDataType=UByte` to `STBWrapper::Save`.
- `STBWrapper::Save` at `Source/Engine/ThirdParty/STBWrapper/STBWrapper.cpp:99`: `stbi_write_png(filename, 1280, 720, 4, data, 1280*sizeof(int32_t))` → `stride = 5120 bytes`. Correct for 4-byte-per-pixel RGBA8.

**Pre-existing latent bug noted, NOT folded in:**
`GetTextureFormat` collapses both `PixelDataFormat::RGB` and `PixelDataFormat::RGBA` to the same 4-channel DXGI format (e.g. `DXGI_FORMAT_R8G8B8A8_UNORM` at lines 45-46, `DXGI_FORMAT_R16G16B16A16_FLOAT` at lines 155-156) — i.e. there is no real 3-channel resource — while `GetTexturePixelDataSize` at lines 198-208 returns a different `l_channelSize` for RGB (3) vs RGBA (4). For any texture where someone sets `PixelDataFormat = RGB`, the readback CPU loop will under-stride pixels and produce a structurally corrupt image. FinalBlend Result uses RGBA so it is unaffected here. Worth filing as a separate task — surfaced, not folded in (per `surface-dont-chase`).

#### Active pipeline path under repro CLI

`Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30` does **not** activate the path tracer. The path tracer is gated on `testCase == "gpu_path_tracer"` at `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Setup.cpp:210-214` — the repro CLI sets neither `-test` nor `-scene`. So `m_GPUPathTracerActive == false`, the FinalBlend input is `TAAPass::Get().GetResult()` (`ExampleRenderingClient_PrepareCommands.cpp:167`), and the rasterizer GBuffer → LightPass → TAA chain produces the input image. The "path tracer produces good surfaces" claim in the ticket is misattributed — the renderer in this run IS the rasterizer.

Scene transition is driven by `World.inl:284-288`: at frame 5, the engine swaps from `UnitTest.InnoScene` to `GISponza.InnoScene` via `SceneService::Load("…/GISponza.InnoScene", true)`.

#### Triangular blob root cause

`Data/ExampleProject/Components/GISponza.Dragon.TransformComponent.json:14-18`: scale `(10, 10, 10)`, position `(0, 0, 0)` — Stanford Dragon at world origin, blown up 10× so it sits directly under the camera at `(0, 2, 0)`. With identity camera rotation and 90° FOV, the dragon's silhouette dominates the lower-center pixels (consistent with `Build/_audit_lower_center.png`).

`Data/ExampleProject/Components/GISponza.Dragon.MaterialComponent.json:14-30`: textures are `Ground037_NormalGL`, `Ground037_Color` (moss / ground albedo, hence the green tint), `BasicMetallicTexture`, `Ground037_Roughness`, `BasicAOTexture` — a ground material pulled onto a dragon mesh. Material-asset mismatch, not a rendering bug.

#### Best-fit root cause

The ticket symptoms are **not produced by FinalBlend, readback, viewport, stereo, or row-pitch.** They are produced by:

1. The GISponza scene's shipped Main Camera transform at `(0,2,0)`/identity placing the camera on the atrium's bilateral symmetry axis, with 90° FOV exposing the symmetry.
2. The Dragon entity at origin with scale 10 and a ground-texture material producing a large moss-green silhouette in the lower-center of the frame.

The post-TASK-222 visual change ("bright corrupt with channel tint" vs the prior "dim corrupt") is exactly what TASK-222 fixed: the readback now samples the in-flight FinalBlend Result rather than mismatched state-tracker data. The image is brighter because the data is now correct — not because anything new is corrupting it.

#### Proposed fix CL shape

There is no rendering-pipeline fix to write here. The closure paths are content fixes against the GISponza scene:

1. **Camera transform** — `Data/ExampleProject/Components/GISponza.Camera.TransformComponent.json` + `Bin/Data/…` mirror: relocate / rotate the Main Camera off the bilateral symmetry axis. Pattern: the orbit harness uses `(yaw, pitch=20°, radius=8, duration=120)` for an off-axis framing. Static scene-file camera could match by placing the camera at, e.g., `(6, 2, 0)` with a yaw quaternion looking back toward origin.
2. **Dragon material** — `Data/ExampleProject/Components/GISponza.Dragon.MaterialComponent.json` + `Bin/Data/…` mirror: replace `Ground037_*` textures with the appropriate dragon PBR set (or simply clear `TextureComponents` to fall back to the JSON Albedo / Metallic / Roughness scalars).
3. **Acceptance criteria reframe** — AC #1 and #2 are not actionable as written ("no vertical mirror-seam") because there isn't one to remove; rewrite them as "GISponza default camera does not photograph the atrium's bilateral symmetry axis" and "no entity uses a textures-array mismatched against its mesh class."

The DOD's "build compiles" / "integration tests" requirements collapse to "load `GISponza.InnoScene`, render N frames, compare visually" — same as TASK-127 / TASK-213 capture procedure. No engine-code or shader-code change needed; the diff is two JSON files in `Data/` (plus the mirror in `Bin/Data/`).

If the user wants to keep the camera at `(0,2,0)` for symmetric framing (some test rationale), then the only AC-actionable fix is the Dragon material. Either decision is the user's; the task description's framing ("FinalBlend/readback path") needs to be discarded.

#### Surfaced (NOT folded in)

- **`GetTextureFormat` RGB→4-channel collapse vs `GetTexturePixelDataSize` returning 3 for RGB**: latent corruption hazard for any future texture configured with `PixelDataFormat::RGB`. File as separate cleanup task before someone trips on it.
- **`current-CL-presentation.png` "channel tint"**: the cyan/teal cast (vs baseline's grey) is the auto-exposure being driven by the now-correct FinalBlend output rather than the post-state-drift garbage. Not a regression; consistent with TASK-222's fix.

#### What was NOT verified in this audit

- No engine launch (read-only audit per dispatch brief).
- No alternate camera transform tested in-engine — proposed fix is on paper only.
- No grep for `RowPitch` arithmetic in `VK`/`MT` graphics backends (DX12 is the active backend for this repro per `-renderer 0`).
- The "mirror" hypothesis cannot be 100% disproved by static analysis if a downstream consumer (display present, OS compositor) is doing something exotic, but the captured PNG comes straight from `STBWrapper::Save` over the readback buffer — no display compositor is in that path.

### 2026-05-17 — Superseded by TASK-228

The 2026-05-14 audit above checked left/right bilateral pixel symmetry and concluded the artifact was content (camera on the atrium's bilateral axis). That audit was right that the symptom is content/symmetry-shaped but wrong on which specific content — and missed that the visible "mirror" framing in the captured PNG was actually a contrast artifact between fully-lit non-metallic regions and zero-output metallic regions.

TASK-228's diagnostic chain re-localized the artifact to `Source/Engine/Common/BCCompression.cpp:78-80`: BC4 compression unconditionally extracted the source `.r` channel for every BC4 slot, so the packed glTF MetallicRoughness textures stored the wrong channel for both metallic (slot 2) and roughness (slot 3) `.innobin` files. With `metallic=1` on ~10% of Sponza pixels, `albedo * (1-metallic) * irradiance = 0` produced the central dark region the PNG showed.

The TASK-228 structural fix lands the per-slot channel-source hint through the importer chain plus a re-bake of the Sponza assets. Close TASK-223 as duplicate of TASK-228 once that commit lands. See TASK-228 Implementation Notes for the fix shape + visual A/B.
