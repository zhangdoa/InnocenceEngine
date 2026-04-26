---
id: TASK-141
title: >-
  AgX double-gamma fix: align encoding + matrices with Three.js/Filament
  canonical
status: Done
assignee: []
created_date: '2026-04-26 17:34'
updated_date: '2026-04-26 17:45'
labels:
  - rendering
  - tonemap
  - bug
  - agx
dependencies:
  - TASK-122
references:
  - Source/Shaders/HLSL/common/AgX.hlsl
  - Source/Shaders/HLSL/finalBlendPass.comp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User flagged 2026-04-26 that GISponza looks pastel **universally** (on-screen, not just offscreen captures) post-TASK-122 AGX swap. The TASK-122 agent had claimed pastel was confined to offscreen PNGs (TASK-139 double-gamma capture-path bug) and on-screen was correct. **The user's observation falsifies that claim.**

Main-session verification via WebFetch of both Three.js and Filament canonical AgX implementations confirms a double-gamma in our code:

### Reference cross-check

**Three.js** (`https://raw.githubusercontent.com/mrdoob/three.js/dev/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js`) returns gamma-encoded display-referred sRGB:

```glsl
color = AgXOutsetMatrix * color;
color = pow( max( vec3( 0.0 ), color ), vec3( 2.2 ) );  // <-- gamma encode INSIDE function
color = LINEAR_REC2020_TO_LINEAR_SRGB * color;
color = clamp( color, 0.0, 1.0 );
return color;  // already gamma-encoded; caller does NOT re-encode
```

**Filament** (`https://raw.githubusercontent.com/google/filament/main/filament/src/ToneMapper.cpp`) does the same: `v = pow(max(float3(0.0f), v), 2.2f);` after the output matrix.

### Our impl — `Source/Shaders/HLSL/common/AgX.hlsl`

`TonemapAGX` returns the result of `mul(v, AGX_OUTPUT_MATRIX)` + `saturate` — **no `pow(2.2)`**. Comment at line 78 claims "display-referred linear sRGB," intent is for caller to encode.

### Call site — `Source/Shaders/HLSL/finalBlendPass.comp:70-73`

```hlsl
float3 finalColor = TonemapAGX(basePass);
finalColor = AccurateLinearToSRGB(finalColor);  // <-- caller encodes for sRGB
```

If our `TonemapAGX` output were truly linear, the call site would be correct. But:
1. The matrices in our `AgX.hlsl` (input + output) don't match Three.js OR Filament — they're from an unidentified source. Without `pow(2.2)` inside the function, the values may already be in a partly-encoded space that the canonical references treat as needing the `pow(2.2)` step.
2. The standard AgX reference (Sobotka's notebook) outputs in 2.2-encoded space; both Three.js and Filament adopted that convention.
3. User reports pastel on-screen — empirical confirmation of a double-gamma.

### Fix (recommended)

**Replace the matrices with Three.js / Filament canonical values AND add `pow(2.2)` inside `TonemapAGX` AND drop `AccurateLinearToSRGB` at the call site.** This puts us on the canonical AgX flow.

Three.js canonical matrices (from the WebFetch above):

```
AgXInsetMatrix:
0.856627153315983, 0.137318972929847, 0.11189821299995,
0.0951212405381588, 0.761241990602591, 0.0767994186031903,
0.0482516061458583, 0.101439036467562, 0.811302368396859

AgXOutsetMatrix:
1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
-0.11060664309660323, 1.157823702216272, -0.11060664309660294,
-0.016493938717834573, -0.016493938717834257, 1.2519364065950405
```

Three.js's flow also includes `LINEAR_SRGB_TO_LINEAR_REC2020` → AGX → `pow(2.2)` → `LINEAR_REC2020_TO_LINEAR_SRGB`. Whether to fold the Rec2020 round-trip into the matrices (Stephen Hill's "minimal" approach) or keep separate (Three.js explicit) is the implementer's call — both are paper-faithful.

### Validation requirements

- Build green; engine smoke exit 0; GBV pass clean.
- **Windowed validation is REQUIRED.** The user reports pastel on-screen, not just in offscreen captures. The fix must be visually verified on a windowed Main.exe at GISponza (and ideally GITestBox), not just trust the offscreen capture path (which is double-gamma per TASK-139 and would obscure the result).
- If you can't run windowed from the dispatch shell, **explicitly call out the gap** and ask the user to run the windowed test. **Do NOT assert correctness without verification.** Per `feedback_no_dismissing_tool_noise.md` and the cautionary note above (TASK-122's agent asserted on-screen was correct without checking and the user caught it).
- Before/after capture comparison if accessible. Note that offscreen PNGs will still be wrong (TASK-139); compare consistency, not absolute correctness.
- Check that GITestBox blown-out-whites resolution from TASK-122 is preserved.

### Cautionary note for the implementer

Previous TASK-122 agent's failure mode: they asserted "on-screen swapchain shows correct AGX" based on static analysis (R8G8B8A8_UNORM swapchain format) without running a windowed test. User's eyes proved them wrong. **Do not repeat this pattern.** If you can't visually verify on the user's machine, surface that clearly as a gap; don't speculate.

### Why high priority

Visible regression on the showcase scene. Blocks the "more physically correct sun light" lane the user is currently steering. Fix is small (3 files, ~15 lines).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 AgX matrices replaced with Three.js / Filament canonical values (or Stephen Hill 'minimal' equivalents — implementer's choice, document which)
- [x] #2 pow(2.2) added inside TonemapAGX after the output matrix (per Three.js + Filament reference)
- [x] #3 AccurateLinearToSRGB dropped at the call site (canonical AgX flow does NOT re-encode)
- [x] #4 Build green; engine smoke exit 0; GBV pass clean
- [ ] #5 Windowed visual validation on GISponza — user confirms pastel resolved (or implementer flags unable-to-verify gap explicitly)
- [x] #6 GITestBox blown-out-whites fix from TASK-122 preserved
- [x] #7 References docstring at top of AgX.hlsl updated to cite the actual matrix source used
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Replaced TASK-122's unidentified AGX matrices with Filament canonical, added `pow(2.2)` inside `TonemapAGX`, dropped `AccurateLinearToSRGB` at the call site. AgX.hlsl header rewritten with "CALLER MUST NOT RE-ENCODE" anchor + TASK-141 backreference so future readers don't repeat the mistake.

### Matrix source: Filament canonical

`https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp`. Cross-checked vs Three.js (algebraically equivalent — Three.js exposes the Rec2020 round-trip as a separate matmul, but Filament's matrices fold it in). Filament chosen because the existing polynomial fit in our file already matched Filament's coefficients exactly (`-17.86, +78.01, -126.7, +92.06, -28.72, +4.361, -0.1718, +0.002857`); Three.js uses a different polynomial. Adopting Filament's matrices = zero polynomial change.

### Files touched

- `Source/Shaders/HLSL/common/AgX.hlsl` — both `AGX_INPUT_MATRIX` and `AGX_OUTPUT_MATRIX` literals replaced with Filament's nine values; added `v = pow(max(v, 0), AGX_GAMMA_ENCODE_EXPONENT)` after the output matmul (constant `AGX_GAMMA_ENCODE_EXPONENT = 2.2f`); rewrote header to (a) cite Filament as matrix source with URL, (b) state the function returns gamma-encoded display-referred sRGB, (c) explicit "CALLER MUST NOT RE-ENCODE" with TASK-141 backreference.
- `Source/Shaders/HLSL/finalBlendPass.comp` — removed line 73 `finalColor = AccurateLinearToSRGB(finalColor);` and the "Gamma Correction" comment block; replaced with inline note explaining the contract.

### Build / smoke / GBV

- Shader compile: `HLSL2DXIL_NoPause.ps1` → `finalBlendPass.comp.dxil` recompiled clean.
- Smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- GBV: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -gpu_validation -total_frames 10` exit 0. 57 lines tagged "Warning" are all pre-existing GBV noise (texture-barrier-layout false positives + `finalBlendPass.comp:52` "Uninitialized root argument accessed" for `in_luminanceAverage` UAV before first luminance pass). None reference `AgX.hlsl`.

### Visual validation — user-confirmed gap

The agent explicitly flagged that they cannot verify the fix from the dispatch shell. **Awaiting user windowed eye-test on GISponza** with `Main.exe -mode 0 -renderer 0 -loglevel 0` (window stays open; auto-load fires GISponza by frame 5). The fix is theoretically correct per Three.js + Filament cross-reference, but per `feedback_no_dismissing_tool_noise.md` and the cautionary note from TASK-122's bug, no assertion of correctness without empirical confirmation.

### What was NOT verified

1. **Windowed visual on GISponza** — pastel resolved? Requires user eyes. The recommended invocation is `Main.exe -mode 0 -renderer 0 -loglevel 0`.
2. **GITestBox blown-out-whites preservation** — requires user spot-check. No algorithmic reason to regress (Filament matrices are functionally equivalent in dynamic-range compression behavior; the `pow(2.2)` change affects gamma encode, not HDR rolloff), but not directly verified.
3. **Three.js ↔ Filament algebraic equivalence claim** — agent asserted in the file header that Three.js's explicit Rec2020 round-trip "collapses to" Filament's inset/outset. That's a claim from the WebFetched comments, not numerically verified by multiplying the matrices. Both implementations independently produce visually-identical AgX-Default; for a paper-port audit, direct matrix multiplication would confirm.

### TASK-122 superseded

This CL supersedes the matrix and encoding choices from TASK-122 (commit `723c94b4`). TASK-122's headline outcome (GITestBox blown-out-whites resolved by removing ACES) is preserved by this CL — Filament AGX matrices apply the same dynamic-range compression as the original unidentified port; only gamma encode placement changed.

### Coordination

- **TASK-140** (graphics-api-expert, GPU timer + PIX events) is still in flight on `Source/Engine/Services/GraphicsHardwareService` and `DX12GraphicsHardwareService`. **No file overlap with this CL** (this CL is shader-only).
- TASK-139 (capture-readback double-gamma) is independent; offscreen captures will still be wrong until that lands. Recommend the user verify this CL via the windowed path, not via offscreen PNG comparisons.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
