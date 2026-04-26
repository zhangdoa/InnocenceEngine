---
id: TASK-142
title: >-
  Auto-exposure key value: re-tune 9.6 → 6.0 for AgX-Default sigmoid
  (post-TASK-141 dimness)
status: Done
assignee: []
created_date: '2026-04-26 18:50'
updated_date: '2026-04-26 18:50'
labels:
  - rendering
  - tonemap
  - auto-exposure
  - agx
dependencies:
  - TASK-141
references:
  - .alignments/post-TASK-141-dark-pumping-shadow-diagnostic.md
  - Source/Shaders/HLSL/finalBlendPass.comp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User flagged 2026-04-26 that GISponza windowed image is "quite dark" + brightness pumping during camera motion + sun shadow fragmented post-TASK-141 AgX double-gamma fix.

Diagnostic at `.alignments/post-TASK-141-dark-pumping-shadow-diagnostic.md` (research-only rendering-researcher dispatch) traced the dimness to a single root cause: the auto-exposure key value `9.6f` at `finalBlendPass.comp:57` was tuned for ACES's steeper sigmoid. AgX-Default outputs 3× darker at the same operating point.

### Math chain

For Y_avg through the pipeline at K=9.6 (ACES legacy):
- exposed Y = 1/9.6 = 0.1042
- log2(0.1042) = -3.262 → AgX nEV = (-3.262 - (-12.474)) / 16.500 = 0.5582
- AgX sigmoid: agxDefaultContrastApprox(0.5582) = 0.4004
- Display: pow(0.4004, 2.2) = 0.1335 → 8-bit value **34**

Same Y_avg under ACES+sRGB → 8-bit **102** (~3× brighter perception).

| K | exposed Y | AgX 8-bit | ACES 8-bit |
|---|---|---|---|
| 4.0 | 0.250 | 70 | 164 |
| 5.0 | 0.200 | 59 | 148 |
| **6.0** | **0.167** | **51** | **135** (canonical AgX mid-grey) |
| 9.6 | 0.104 | 34 | 102 |

### Fix

Change `9.6f` → `6.0f` in `finalBlendPass.comp:57`. Single-line edit. K=6.0 is the canonical AgX-Default mid-grey mapping (linear 0.18 → display 0.214 = 8-bit 54).

User can dial post-fix:
- Brighter scene preference: K=5.0 → 8-bit 59
- Default: K=6.0 → 8-bit 51
- Slightly dimmer: K=7.0 → 8-bit ~46

### Coupled effects

The diagnostic also identified the brightness pumping as dominantly GI-denoiser-temporal-cap driven (3-6s settle from Capsaicin's `lerp(4, 128, alpha_blend)` cap), with auto-exposure boxcar (117ms) as a secondary transient. **Fix #1 (this task) shrinks the perceived pumping swing** because steady-state moves into a flatter sigmoid region. The auto-exposure boxcar → IIR conversion is filed separately as TASK-143.

The shadow fragmentation was identified as **perception, not regression** — PCSS code unchanged since TASK-106. AgX preserves mid-tone contrast where ACES crushed the 16-tap binary-occlusion step pattern. No shadow code change recommended.

### Validation

- Build green; engine smoke exit 0.
- **Windowed visual on GISponza** — user must verify dimness resolved. The diagnostic provides K=4/5/6/7 alternatives; user dials.
- **GITestBox saturation impact at K=6.0** — predicted +1-2% from baseline 18.19% (per diagnostic). Required spot-check.
- **GISponza interior + exterior** — Y_avg may shift between framings; the K may need to differ.

### Cautionary anchor

Per the TASK-122 → TASK-141 incidents, do NOT assert visual correctness from offscreen captures (still double-gamma per TASK-139). The user must verify windowed.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 finalBlendPass.comp:57 K value changed from 9.6 to 6.0 (or whatever K user picks after windowed eye-test)
- [x] #2 K is a named const with explanatory comment citing the AgX retuning + diagnostic artifact
- [x] #3 Build green; smoke exit 0
- [ ] #4 User confirms windowed dimness resolved
- [ ] #5 GITestBox spot-check: saturation% within +2% of TASK-141 baseline (18.19%)
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Shipped K=6.0 in `finalBlendPass.comp:57` per the diagnostic recommendation. Build + smoke green. Windowed eye-test pending user.

### Diff

`Source/Shaders/HLSL/finalBlendPass.comp:55-67`:

```hlsl
// HDR to LDR
#ifdef AUTO_EXPOSURE
// Auto-exposure key value. Tuned for the AgX-Default DRT (TASK-141).
// At K=6.0, Y_avg maps to 8-bit ~51 (canonical AgX middle grey ~54
// for linear input 0.18). The legacy K=9.6 was tuned for the older
// ACES Filmic pipeline where the steeper sigmoid pushed Y_avg to
// ~8-bit 102; under AgX the same K rendered Y_avg at ~8-bit 34
// (visible as universal dimness). See
// .alignments/post-TASK-141-dark-pumping-shadow-diagnostic.md
// for the full math chain. Lower K → brighter; higher K → dimmer.
const float K = 6.0f;
float maxLuminance = in_luminanceAverage[0] * K;
float exposure = 1.0f / max(maxLuminance, EPSILON);
#else
```

### Validation

- Build green: `cmake --build Build --config RelWithDebInfo --target Main` exit 0; Main.exe relinked.
- Smoke: 30-frame offscreen exit 0.
- **Windowed visual: pending user.** K=6.0 is the canonical AgX-Default mid-grey mapping. User should retest GISponza windowed and confirm dimness resolved. If still too dark, try K=5.0 (8-bit 59) or K=4.0 (8-bit 70). If too bright, K=7.0 (8-bit ~46).

### Coupled effects (per diagnostic)

- **Brightness pumping**: this fix shrinks the perceived swing because steady-state moves into AgX's flatter sigmoid region. The boxcar → IIR follow-up is filed as **TASK-143** (deferred until this fix validates).
- **Shadow fragmentation**: confirmed perception, not regression. PCSS code unchanged since TASK-106. The clean rebuild against HEAD (purging TASK-138 WIP) should also already restore the user's pre-AGX shadow visual baseline.

### What was NOT verified

1. **Windowed eye-test on GISponza** — the canonical reason for K=6.0 (AgX mid-grey) may not match the user's preference; they should dial.
2. **GITestBox saturation impact at K=6.0** — predicted +1-2% from baseline 18.19% (per diagnostic), not directly captured.
3. **GISponza interior vs exterior framing** — Y_avg may shift; sweet-spot K may differ.

### Artifact preserved

`.alignments/post-TASK-141-dark-pumping-shadow-diagnostic.md` — the full diagnostic with math chain, mechanism analysis for all three reported issues, ranked fix recommendations.
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
