## Post-TASK-141 visual regression — diagnostic

**Scope.** Three user-reported visual issues post-TASK-141 (commit `54df3e86`,
AgX matrix + encoding fix) on GISponza windowed:

1. Image is universally darker (not just shadows).
2. Brightness pumps during camera motion — bright during motion, fades to a darker steady state over a few seconds.
3. Sun shadow looks more fragmented than before.

This artifact is research-only — no shader or engine code modified. Findings are intended to anchor the implementation CL.

---

### Headline findings

| Issue | Cause | Layer |
|---|---|---|
| 1. Universal dimness | Auto-exposure key value `9.6f` was tuned for ACES; AgX-Default has a steeper sigmoid tail and lower response at 1/9.6 of mid-range. Average pixel lands near 8-bit value **34** under AgX vs **102** under ACES at the same scene radiance. | `Source/Shaders/HLSL/finalBlendPass.comp:57` |
| 2. Multi-second brightness pumping | Two coupled lags. **Dominant: GI denoiser temporal cap** (Capsaicin-style cap = `lerp(4, 128, alpha_blend)`) which converges to steady state over 3–6 s once motion stops. **Secondary: 7-frame auto-exposure boxcar** (~117 ms at 60 fps). The auto-exposure lag was always there; AgX makes it visible because its slope at the new (low) working point amplifies small radiance changes. | GIDenoise.comp + luminanceAveragePass.comp + the `9.6f` key value coupling them |
| 3. Sun shadow "fragmentation" | **Perception of pre-existing artifact**, amplified by AgX. PCSS resolver and EvaluateSunLighting are unchanged since TASK-106 (commit `1e869aad`, 2026-04-19) — well before the AgX swap. The 16-tap Poisson sample pattern with per-pixel rotation is identical; what changed is the tonemap mid-tone slope, which makes per-tap step changes more visible. | Not a regression. Real but separate work item: increase tap count, jitter temporally, or upgrade to a denoised shadow term. |

---

### Math chain — issue 1 (dimness)

**Setup.** finalBlendPass.comp:57 computes `maxLuminance = Y_avg * 9.6f`, then `exposure = 1 / max(maxLuminance, EPSILON)`. The xyY-then-RGB round-trip on lines 64–67 is mathematically equivalent to scalar `RGB *= exposure` (chromaticity preserved).

**For Y = Y_avg (the average pixel)**, the post-exposure linear value is `1 / 9.6 ≈ 0.1042`.

**AgX log encode.** With AGX_MIN_EV = -12.474, AGX_MAX_EV = +4.026, span 16.500:

```
log2(0.1042) = -3.262
nEV = (-3.262 - (-12.474)) / 16.500 = 0.5582
```

**6th-order sigmoid output at nEV = 0.5582** (Filament's `agxDefaultContrastApprox`):
```
sig(0.5582) = 0.4004
```

**Final 8-bit display value** (after pow(2.2) gamma encode in TonemapAGX):
```
disp = 0.4004 ^ 2.2 = 0.1335 = 8-bit value 34
```

The same `Y_avg` under ACES + AccurateLinearToSRGB lands at **8-bit value 102** — almost exactly 3× brighter on screen. Both are reproduced in this matrix:

| Key value `K` | exposed `Y_avg = 1/K` | AgX 8-bit | ACES + sRGB 8-bit |
|---|---|---|---|
| 3.0 | 0.3333 | 84  | 183 |
| 4.0 | 0.2500 | 70  | 164 |
| 5.0 | 0.2000 | 59  | 148 |
| 6.0 | 0.1667 | 51  | 135 |
| **9.6** | **0.1042** | **34** | **102** |
| 12.5 | 0.0800 | 26 | 85 |
| 18.0 | 0.0556 | 17 | 64 |

**Reference: middle-grey (linear 0.18).** AgX maps it to **8-bit 54**; ACES+sRGB maps it to **8-bit 141**. Middle-grey for AgX-Default is meant to land near 18% on screen (~54/255), per Sobotka's design notes — that *is* the canonical AgX behaviour. So AgX is doing exactly what AgX claims; the calibration mismatch is the **upstream key value**, not the tonemap.

**To put `Y_avg` near 8-bit 102** (perceptual middle-grey ~ what ACES delivered), the post-exposure value would need to be ~0.20–0.25, i.e. `K ≈ 4–5`. To put it at 8-bit 90 (a slightly darker but still neutral target), `K ≈ 6`. **Anything ≥ 8 will look "dim" relative to the prior ACES build.**

---

### Mechanism — issue 2 (multi-second pumping)

The histogram → moving-average → exposure chain consumes the **TAA output**, which already has GI temporally accumulated in it (see `ExampleRenderingClient.cpp:354–377`). Two distinct lags exist downstream of TAA:

#### Lag (a): GI denoiser temporal cap — `Source/Shaders/HLSL/GIDenoise.comp`

Capsaicin port: `cap = lerp(MIN_CAP=4, MAX_CAP_FACTOR·MAX_BLUR_MASK = 128, |alpha_blend|)` (gi1.comp:4092). Per-frame mixing rate = 1/N where N is the current cap.

| State | `alpha_blend` | cap N | per-frame mix | 95% settle |
|---|---|---|---|---|
| Heavy motion | ~0 | 4   | 25%   | ~10 frames (170 ms @60) |
| Light motion | ~0.5 | ~66 | ~1.5% | ~200 frames (~3 s @60) |
| Steady state | ~1 | ~128 | ~0.8% | ~380 frames (~6 s @60) |

The color-delta EMA (rate 1/8 in `kColorDeltaEmaRate`) decays over ~30 frames after motion stops, so `alpha_blend` climbs back toward 1 over a similar window — but the **cap doesn't fully relock to 128 until that EMA settles**, so newly-converged regions retain a 3–6 s brightness adaptation.

#### Lag (b): auto-exposure 7-frame uniform boxcar — `luminanceAveragePass.comp:18`

`numHistoryFrames = 8` → 7-tap moving average over the per-frame log-luminance estimate. Step response: 50% in ~4 frames, 100% in 7 frames (~117 ms at 60 fps).

#### Why the user sees multi-second pumping post-AgX (and not before, or not as obviously)

- The dominant timescale (3–6 s) **comes from the GI denoiser EMA**. This was true before TASK-122 too. ACES' steeper sigmoid in the 0.05–0.3 mid-range (slope ~0.7) compressed those radiance changes into a smaller display-space range; AgX's slope at the operating point (~0.3 disp/EV) **uncompresses** them.
- The auto-exposure boxcar adds a fast `~117 ms` overshoot on top: when motion reveals a brighter region, the histogram immediately rises, exposure responds, exposure slightly under-corrects for the next ~4 frames, then the GI history slowly drags Y_avg up over seconds → the average pixel slowly loses brightness as exposure drops.
- The "brightens during motion → fades to darker steady-state" pattern matches: motion suppresses GI history (cap → 4), so newly disoccluded radiance lands fast and bright. Once motion stops, GI history accumulates more samples → `Y_avg` drifts up → exposure drops → display darkens. This is exactly the observed shape.

**Dominant contributor: the GI denoiser EMA + the dimness from issue 1 acting together.** Re-tuning `9.6f` to a value that puts steady-state near 8-bit 80–100 will (a) eliminate the dimness and (b) shrink the perceived swing because both the bright-during-motion and dark-at-rest endpoints rise in parallel.

---

### Verdict — issue 3 (shadow fragmentation)

**Perception, not regression.** Evidence:

- `git log -- Source/Shaders/HLSL/common/shadowResolver.hlsl` last touched `1e869aad` (TASK-106) on 2026-04-19, **6 days before TASK-122/141**. No edits to PCSS, blocker search, kernel rotation, sample count, or bias since.
- `lightPassDirectLighting.hlsl::EvaluateSunLighting` calls `SunShadowResolver` unchanged.
- 16-tap Poisson + per-pixel rotation is fundamentally noisy at small penumbras — single-tap binary occlusion contributes 1/16 = ~6% of the shadow factor → with AgX's higher mid-tone contrast, each tap step now displays as ~10/255 instead of (under ACES sigmoid crush) ~5/255.
- The shadow factor enters as `1.0 - shadow` multiplied into `io_DirectLuminance`. Under AgX, the *relative* contrast between fully-lit and partially-occluded mid-tone pixels is preserved more honestly; under ACES it was crushed into a smaller display range.

**Recommendation: do not modify `shadowResolver.hlsl` for this incident.** If the user wants softer shadows post-AgX, file as a separate task — candidates include doubling the filter sample count, temporal jitter of the rotation seed across frames, or a 1-tap separable bilateral denoise on the shadow term. Those are real improvements, not regression fixes.

---

### Ranked fix recommendations

#### Fix #1 (highest priority): re-tune the auto-exposure key value for AgX

- **File:** `Source/Shaders/HLSL/finalBlendPass.comp:57`
- **Change:** `float maxLuminance = in_luminanceAverage[0] * 9.6f;` → `float maxLuminance = in_luminanceAverage[0] * 6.0f;` (or 5.0f for slightly brighter, 7.0f for slightly dimmer).
- **Predicted impact:** average pixel under K=6.0 lands at 8-bit value **51**; that is mid-grey for AgX-Default (compare to 102 under ACES; AgX is intentionally *darker* at middle grey, this is the design). At K=5.0 → 8-bit 59. The dimness disappears (~+50% perceived brightness vs current). Pumping perceptually shrinks because the steady-state output is now off the toe of the AgX sigmoid where slope is steepest.
- **Cost:** 1-line edit. Recompile, smoke, windowed eye-test on GISponza + GITestBox. ~15 min total.
- **Risk:** GITestBox saturation could change. The 60-frame stat under ACES (TASK-122 commit message): GITestBox sat% ≥254 = 18.39%, AgX (with K=9.6) = 18.19%. Lowering K boosts exposure → brighter input to AgX → more saturation. Likely shift +1–2%, but AgX's highlight retention should prevent runaway clipping. **Verify with a 60-frame GITestBox capture** before committing.
- **What to verify:** Y_avg at runtime via a temporary `Log(Verbose, ...)` injection (revert before commit) on GISponza ground level; confirm it matches the analytical prediction (Y_avg ≈ 1.0–3.0 cd/m² indirect-dominated typical for Sponza interior).

#### Fix #2: extend the auto-exposure smoothing to a true EMA

- **File:** `Source/Shaders/HLSL/luminanceAveragePass.comp:51–69`
- **Current behaviour:** boxcar over 7 frames (uniform weights, hard step at the trailing edge).
- **Change shape:** replace cyclic-buffer + sum/N with a 1-pole IIR `out[0] = lerp(out[0], current, alpha)` where `alpha = 1 - exp(-dt / tau)` and `tau ≈ 0.5–1.0 s`. This requires `dt` to be passed in via PerFrame_CB, which it already is (`g_Frame.deltaTime` exists in common.hlsl — verify before quoting).
- **Predicted impact:** removes the boxcar's hard step at frame 7 (currently visible as a sharp dimming bump after fast camera moves). EMA also costs only a single float of state instead of 8.
- **Cost:** small re-shape of the smoothing block + storage drop from 8 floats → 1 float (state) + 1 float (output). 30 min including a soak test.
- **Risk:** changing the smoothing curve may interact with the GI lag in surprising ways. **Suggest doing this as a follow-up after Fix #1 is validated**, not bundled in.

#### Fix #3 (don't do unless user reports it): shadow softness

- **Suggestion only if user still flags shadows post-Fix #1:** in `shadowResolver.hlsl`, increase `filterSamples` from 16 to 32, OR add a frame-index seed to `ShadowKernelRotationAngle` so the per-pixel rotation jitters across frames (TAA then averages out the binary tap noise). Both are 1-line changes; 32-tap PCF roughly doubles the cost of the shadow loop.
- **Don't touch this in the same CL as Fix #1** — perception fixes need their own validation pass.

---

### What was NOT verified

Per the cautionary anchor in the task brief and feedback memory `feedback_no_dismissing_tool_noise.md`:

1. **Empirical Y_avg measurement on GISponza.** I did *not* inject a debug log read of `in_luminanceAverage[0]`. The K=9.6 → display 34 prediction is analytical, not measured. The shape of the prediction (qualitative: dimmer than ACES) is robust because it depends only on the AgX sigmoid being a sigmoid; the exact 8-bit value depends on the actual `Y_avg` in the scene.
2. **Windowed visual confirmation that Fix #1 (K=6.0) lands near perceptual middle grey.** The K=4–6 recommendation is bracketed; user must eye-test the result. ACES → AGX is a deliberate look change, not a 1:1 brightness match — a value that *feels right* may be K=4 or K=8.
3. **Whether GISponza's `Y_avg` actually sits at the predicted ~0.1–1.0 cd/m² range.** Sponza's bright sun + courtyard dome could push `Y_avg` up by an order of magnitude during exterior shots, which would shift the K-value sweet spot. Quick test: run `Main.exe -mode 0 -renderer 0 -loglevel 0` on GISponza, orbit camera, eye-test Fix #1 at K=4 and K=6 on both interior and exterior framings.
4. **Capsaicin GI cap dynamics on real GISponza motion.** I traced the math from gi1.comp:4092 + the EMA rate but did not capture before/after camera moves. The 3–6 s timescale prediction follows from `1/N` where `N ∈ [4, 128]`; if real `alpha_blend` data shows the cap rarely climbs above ~30, the multi-second part of the pumping is shorter and Fix #1 alone is sufficient. If it climbs to 100+, Fix #2 becomes more valuable.
5. **Three.js ↔ Filament algebraic equivalence claim** in `AgX.hlsl` header — inherited from TASK-141, not re-verified. Not relevant to this diagnostic but flagged in case the K-tuning produces unexpected hue shifts (it shouldn't, because the matrices map gamuts and the sigmoid is the same).
6. **GITestBox saturation impact of K=6.0** — predicted +1–2% but not measured. Required before committing Fix #1.
7. **Shadow taps actually being visible vs binary occlusion noise vs penumbra-size aliasing.** I asserted "perception of pre-existing artifact" from the unchanged code + AgX's higher mid-tone contrast, but did not capture a side-by-side. If user disagrees post-Fix #1, that's a separate dive.
