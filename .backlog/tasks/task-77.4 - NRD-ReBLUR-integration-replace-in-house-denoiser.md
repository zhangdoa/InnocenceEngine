---
id: TASK-77.4
title: 'NRD ReBLUR integration — replace in-house denoiser stack (post-relicense)'
status: To Do
assignee: []
created_date: '2026-05-09'
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
Pivot from the in-house SVGF-shape denoiser (TASK-77.2 in-flight work) to NVIDIA NRD ReBLUR. TASK-77.2's bilateral + temporal accumulator + Option D AccumBuffer ping-pong each shipped with regressions (contour artifacts, fireflies, intense ghosting on motion). Three sessions of in-house tuning hit the same architectural traps the project's history has documented (TASK-6 SVGF walkback, TASK-125 Capsaicin walkback, TASK-77.1 wrong-framing closure).

Post-relicense to MIT (commit `18b6ece3`), NRD's NVIDIA-SDK license is now compatible with the engine's terms (MIT does not propagate restrictions onto linked dependencies). NRD ReBLUR is the production-shipped shape we have been trying to badly reimplement: demodulated diffuse/specular inputs, hit-distance-driven blur radius, built-in temporal-history clamping with proper variance estimation, antilag, firefly suppression. ~1300 LoC integration vs the multi-session in-house chain.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 (AC-1, visual, blocking) NRD ReBLUR delivers clean denoised output on motion across UnitTest + GITestBox + GISponza. User-direction layer-4 sign-off on at least one moving-camera capture per scene.
- [ ] #2 (AC-2, visual, blocking) No new artifacts vs raw 1-spp PT (no contour lines, no over-blur, no ghosting). Static convergence equal-or-better than current frame-accumulator baseline.
- [ ] #3 Compile-time toggle `Inno::NRD::ENABLED` (CMake `BUILD_WITH_NRD`) elides all NRD code when OFF; binary identical to pre-CL-1 baseline.
- [ ] #4 Runtime DevToggleRegistry `NRDDenoise` allows live A/B comparison; force-off on AMD/Intel via DXGI vendor check.
- [ ] #5 License-bundling correct: `LICENSES.md` aggregator at root, NV attribution line, README addendum. Engine `LICENSE` stays MIT.
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Plan (2026-05-09)

### CL split

| CL | Scope | Est. LoC | Visible-progress |
|---|---|---|---|
| **CL-0** (manual, no commit) | `git checkout -- Source/` discard the in-house Option D mess + drop stash@{0} CL-3 spatial bilateral *after* CL-3 lands. | n/a | Working tree clean |
| **CL-1** | NRD submodule (pin `v4.17.4`) at `Source/External/GitSubmodules/NRD` + CMake `BUILD_WITH_NRD` toggle + `LICENSES.md` aggregator + `Inno::NRD::ENABLED` constexpr skeleton in new `NRDConstants.h`. **No engine code wires to NRD yet**. | ~80 | Build-system-only; binary identical to pre-CL on `=OFF` |
| **CL-2** | Format-conversion compute pass (pack engine inputs to ReBLUR's expected layouts: `IN_VIEWZ`, `IN_NORMAL_ROUGHNESS`, `IN_MV`, `IN_DIFF_RADIANCE_HITDIST`, `IN_SPEC_RADIANCE_HITDIST`) + integrator albedo-demodulation at the diffuse-radiance write site + delete `PTDenoiseTemporalPass` (4 files) + drop the GBuffer-equivalent ping-pong (`_Even`/`_Odd`) since NRD reconstructs prev-frame from MVs internally. | ~400 | Display reverts to baseline 1-spp PT (NRD not dispatched yet) |
| **CL-3** | `NRDIntegration.hpp` adapter under `Source/ExampleProject/RenderingClient/NRDIntegrationAdapter.{h,cpp}` (translates engine `TextureComponent*` → NRI `Resource`) + new `PTNRDDenoisePass` calling `Integration::SetCommonSettings` + `Denoise` + new `PTNRDCompositionPass` un-packing ReBLUR outputs and combining `albedo*outDiff + outSpec` → tonemap input. Tonemap binding swaps from `GPUPathTracerPass::GetResult()` to composition output. | ~600 | First user-visible improvement |
| **CL-4 (optional)** | `IN_DIFF_CONFIDENCE` / `IN_SPEC_CONFIDENCE` from disocclusion/sample-count signal + `DevToggleRegistry` tuning hooks (`HitDistParams`, `accumulationFrameNum`, `enableAntiFirefly`) + `Inno::NRD::FORCE_OFF_ON_NON_NV_GPU` runtime check via DXGI adapter description. | ~200 | Tuning + portability |

**Total**: ~1300 LoC net, 5-7 days single-author wall-clock.

### Reuse from existing TASK-77.2 work

**Keep** (NRD wants exactly this data):
- CL-1 GBuffer-equivalent UAV write sites in `PTRaygenIntegrator_GBufferWrite.hlsli` and `PTRaygenIntegrator.hlsl`. NRD wants worldPos / normal / roughness / albedo / motion / mesh-id / hit distance — all already produced.
- Per-lobe radiance UAVs (`u_PTDenoise_RadianceDiffuse` / `u_PTDenoise_RadianceSpecular`) — these become NRD's `IN_DIFF_RADIANCE` / `IN_SPEC_RADIANCE` once packed with hit-dist via `REBLUR_FrontEnd_PackRadianceAndNormHitDist`.
- Lobe-tagging logic (`isSpecularPath`) at primary-hit BSDF importance sample.
- AccumBuffer (single-buffered) as composition output target.

**Throw away** (CL-2 deletions):
- `PTDenoiseTemporalPass.{h,cpp}` + `_Dispatch.cpp` + `_RenderTargets.cpp` — NRD owns temporal accumulation.
- AccumBuffer ping-pong (Option D corpse).
- History textures (`m_HistoryRadiance*`, `m_HistoryMoments*`).
- SVGF moment shape constants in `PTDenoiseShared.hlsl`.
- GBuffer-equivalent ping-pong `_Even`/`_Odd` — NRD reconstructs prev-frame internally from motion vectors.

**Discard already-stashed** (post-CL-3):
- `stash@{0}` CL-3 spatial bilateral. Drop with `git stash drop stash@{0}` once CL-3 NRD lands.

### Input format conversions

| ReBLUR slot | Format | Current engine source | Conversion |
|---|---|---|---|
| `IN_VIEWZ` | R32F or R16F (linear view-Z) | `m_PTGBuffer_Position` RT0.rgb worldPos | Pack pass: transform worldPos by current view, take Z |
| `IN_NORMAL_ROUGHNESS` | RGBA8 packed (oct-encoded) | RT1.rgb worldNormal + RT2.a roughness | `NRD_FrontEnd_PackNormalAndRoughness(N, roughness)` |
| `IN_MV` | RGBA16F (2.5D or 2D-pixel) | RT3.xy pixel-space motion | 2D-pixel mode + `motionVectorScale = {1, 1, 0}` to keep engine's `prev - curr` convention |
| `IN_DIFF_RADIANCE_HITDIST` | RGBA16F | per-lobe diffuse UAV + RT3.z hit dist | `REBLUR_FrontEnd_PackRadianceAndNormHitDist(rad, normHitDist, true)` |
| `IN_SPEC_RADIANCE_HITDIST` | RGBA16F | per-lobe specular UAV + RT3.z hit dist | Same |
| `OUT_DIFF/SPEC_RADIANCE_HITDIST` | RGBA16F (NRD-allocated) | n/a | `REBLUR_BackEnd_UnpackRadiance` then `albedo * outDiff + outSpec` |

**Albedo demod at write site**: at the `radianceDiffuse` write in the integrator, multiply by `1 / max(albedo, 0.001)`. Composition (CL-3) re-multiplies. Bounce ≥ 1 contributions are already albedo-modulated by the path's earlier vertex (matches ReBLUR reference integrators).

**Hit-distance normalization**: ReBLUR's `hitDistParams = {A=3, B=0.1, C=20, D=-25}` are NV's defaults. Default these for CL-3 first frame; expose via `Inno::NRD::HitDistParams` constexpr struct in `NRDConstants.h` for tunability.

### Compile-time + runtime toggles

```
Inno::NRD::ENABLED  (constexpr in NRDConstants.h)
  = true  iff BUILD_WITH_NRD CMake option is ON
  = false otherwise → all NRD code elides via if constexpr (Inno::NRD::ENABLED)

g_DevToggle_NRDDenoise (runtime bool in DevToggleRegistry)
  = always present (only meaningful when ::ENABLED)
  = forced false when running on non-NV GPU (CL-4 vendor check)
  = user-toggleable for A/B comparison
  = when false, skip Format-Convert + NRD-Denoise + Composition passes;
                tonemap reads raw AccumBuffer (1/N mean over frames).
```

Two-level shape mirrors `PTHashGridCache::ENABLED` + `g_DevToggle_HashGridCache` already in the engine.

### Vendor lock-in fallback

**Recommend**: compile-in always, runtime force-off on AMD/Intel via DXGI adapter description check. AMD users see raw 1-spp PT (matches current pre-denoise baseline). Single binary ships everywhere.

Reject `#ifdef NV_GPU_ONLY` because it splits the build matrix.

### License-bundling

- New `LICENSES.md` at repo root. Aggregates bundled-third-party notices. NRD section reproduces full text of `Source/External/GitSubmodules/NRD/LICENSE.txt` plus required attribution: *"This software contains source code provided by NVIDIA Corporation."*
- `README.md` addendum: *"This engine optionally links the NVIDIA NRD SDK (proprietary). See LICENSES.md."*
- Root `LICENSE` (MIT, post-`18b6ece3`) stays unchanged.

NV's redistribution clause (§1.c) satisfied: (i) engine has material additional functionality, (ii) NRD incorporated only as object code, (iii) NRD not redistributed standalone. Hardware-interop clause (§1.b) satisfied via runtime fallback on non-NV.

### Risks

| Risk | Mitigation |
|---|---|
| NRD pack-helper signature drift across releases | Tag-pin `v4.17.4`. CL-2 entry validates signatures against checked-in submodule (`Source/External/GitSubmodules/NRD/Shaders/Include/NRD.hlsli`) — raw URL 404'd in planning context. |
| `NRDIntegration.hpp` API drift | Tag pin. Adapter class isolates dependency to one TU. |
| NRD CMake project conflicts with engine CMake | NRD has self-contained CMake; build via `build_third_party()` helper that already absorbs assimp's similar standalone build. Pass `-DNRD_DISABLE_INTERPROCEDURAL_OPTIMIZATION=ON` if LTO clashes. |
| Queue-ordering: NRD on graphics queue vs PT on compute | Insert `WaitOnGPU` between PT compute completion and FormatConvert + NRD dispatches. Existing pattern. CL-3 acceptance verifies no race. |
| Albedo-demod numerical instability on near-black albedos | `max(albedo, 0.001)` floor at demod site; composition re-mod uses raw albedo. |
| `motionVectorScale` sign vs engine's `prev - curr` convention | Resolve at CL-3 by inspection (read `NRDDescs.h::CommonSettings`). Set ±1 to align. Visual-validation gate catches mis-sign as smearing. |

### CL-1 detailed brief (for next session entry)

**Files to add**:
- `Source/External/GitSubmodules/NRD/` (submodule import, pin `v4.17.4`)
- `Source/ExampleProject/RenderingClient/NRDConstants.h` (constexpr `Inno::NRD::ENABLED`, hit-dist defaults, no NRD include yet)
- `LICENSES.md` (root)

**Files to modify**:
- `CMakeLists.txt` (root): add `option(BUILD_WITH_NRD "Build NVIDIA NRD denoiser integration" ON)`
- `CMake/BuildThirdPartyLibs.cmake`: append `build_third_party(NRD ...)` call gated on `BUILD_WITH_NRD`
- `.gitmodules`: submodule entry
- `README.md`: one-line addendum

**Build verification**:
- `Scripts/BuildWin.ps1` succeeds with both `-BuildWithNRD ON` and `=OFF`
- Binary identical between OFF-builds before and after CL-1 (zero engine code wired)

### Deferred to CL-2 entry

- `NRD.hlsli` raw URL 404'd in planning. CL-2 entry's first task: read `Source/External/GitSubmodules/NRD/Shaders/Include/NRD.hlsli` directly to confirm `REBLUR_FrontEnd_PackRadianceAndNormHitDist` / `NRD_FrontEnd_PackNormalAndRoughness` / `NRD_FrontEnd_GetNormalizedHitDist` signatures against `v4.17.4`.

### Cross-references

- TASK-77 — parent umbrella.
- TASK-77.2 — superseded by this task. In-house denoiser (CL-1 GBuffer-equivalent + CL-2 lobe-split + CL-3 spatial bilateral + Option D AccumBuffer ping-pong) shipped with regressions across three iteration sessions; the CL-1+CL-2 input-shape work survives as feed-in to NRD.
- TASK-77.1 — closed wrong-framing (cache is feeder, not denoiser); cache stays compile-time-OFF.
- Commit `18b6ece3` — relicense from GPL-3.0 to MIT, the unblocker.
- Stash@{0} — CL-3 spatial bilateral (abandoned), drop after CL-3 NRD lands.

<!-- SECTION:NOTES:END -->
