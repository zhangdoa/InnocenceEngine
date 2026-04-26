# Alignment audit — TASK-6.3 LightPass fallback when 4-corner ring search exhausts

- Paper: Boissé et al., "GI-1.0: A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination", AMD Tech. Report 22-10-9831, October 2022 (`Build/GI1_0.pdf`). Sections audited: §2.1.5, §2.2 (in full), §2.4.1, §2.4.3.
- Reference impl: AMD Capsaicin v1.3 at commit `914b91596cd119eda85fbc1d3c7ee6ac391b1452` (2025-11-17), under `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/`.
- Audit date: 2026-04-26.
- Counted summary: **0 faithful / 1 divergent / 1 N/A (proposed task descriptor mismatched)**.

The audit is small because TASK-6.3's premise — "fall through to the world cache at the LightPass site when the 4-corner interpolation fails" — does **not** match what the paper or Capsaicin do. The paper-faithful structural fix is different from what the task description proposes.

---

## Headline finding

GI-1.0 does not consult the world cache at the LightPass / per-pixel interpolation site. The world cache (§2.2) is exclusively the lighting source for **secondary path vertices** — what the screen-probe rays hit when they bounce. When per-pixel interpolation fails (all 4 neighbour probes get a null edge-aware weight), the prescribed fallback is:

1. **Relaxed interpolation** — re-blend the same 4 probes with equal weights (no edge-aware gate). Paper §2.4.1, last paragraph; Capsaicin `gi1.comp:1604–1613`.
2. **Denoiser hint** — write `0.0` into the alpha channel of the resolved irradiance texture. Paper §2.4.1: *"We flag pixels calculated using relaxed interpolation in the alpha channel of the resolved texture; this information will be used later as a hint for the denoiser to try and discard the evaluated sample."* Capsaicin `gi1.comp:1636–1638`.
3. **Denoiser absorbs** — the temporal accumulator (`ReprojectGI`) sees `color.w == 0.0`, marks the sample with `lighting.w = -1.0` (Capsaicin `gi1.comp:4087`), the blur-mask emission becomes `MAX_BLUR_MASK - lighting.w` ≈ max radius (Capsaicin `gi1.comp:4083`, clamped at 8 in the filter), and the variable-radius bilateral blur (Figure 19) reaches into surrounding well-sampled pixels. Paper §2.4.3: *"When interpolation fails and no history is available, we use the relaxed irradiance interpolation instead of outputting no irradiance."*

There is **no fall-through to `HashGridCache_FilteredRadianceDirect/Indirect` at the LightPass / interpolation kernel** in Capsaicin. Every site that calls those world-cache readers is in the secondary-ray / multi-bounce / glossy-reflection path (`gi1.comp:1962, 2377, 2381, 2891, 2895`), not at the on-screen per-pixel resolve.

---

## Alignment table

| # | Decision | Paper spec | Reference impl | Our impl | Status |
|---|----------|------------|----------------|----------|--------|
| 1 | Behaviour when all 4 corner probes get zero edge-aware weight in the LightPass / per-pixel interpolation site | Relaxed interpolation: re-blend the 4 probes with equal weights, mark pixel with denoiser hint in alpha channel; denoiser then uses the hint to widen the spatial blur radius and absorb the under-sampled pixel. (§2.4.1, §2.4.3) | `gi1.comp:1604–1613` (relaxed-weights backup) → `gi1.comp:1636–1638` (denoiser_hint = 0 written to ColorBuffer.w) → `gi1.comp:4083, 4087, 4099` (denoiser interprets and emits max blur mask). | `RadianceCacheCommon.hlsl:286–301` performs the relaxed-equal-weight blend correctly inside the `else` branch of `SampleRadianceCache`. **However:** `GIDenoise.comp:162–168` then hard-codes `color.w = 1.0` for every SH-evaluated tap, *erasing* the relaxed-interpolation flag before it reaches the temporal accumulator. The blur-mask widening on relaxed pixels never fires; under-sampled pixels enter the temporal history at full weight and survive the denoise. `lightPassIndirectCompose.hlsl:38` then reads from this and clamps with `LIGHTPASS_AMBIENT_FLOOR` to mask the resulting darkness. | **DIVERGENT** |
| 2 | Sample the world cache (`in_WorldTileGrid`) with `(-viewDir, isShortRay)` at the LightPass call site as the structural fallback for failed 4-corner interpolation. | **Not in paper.** The world cache is for secondary path vertices only (§2.2.1, §2.2.4). | **No corresponding code in Capsaicin.** No on-screen LightPass reads from `g_HashGridCache_*Buffer`. | N/A — task description's proposed approach. The shape exists in `RadianceCacheClosestHit.hlsl:71–94` but only at the off-screen ray-miss site, not at LightPass. | **N/A — proposal not paper-faithful** |

---

## Detail entry — Row 1 (DIVERGENT)

**Paper quotes**

§2.4.1 (last paragraph before §2.4.2): *"If all probes get assigned a null weight, then interpolation fails, which can lead to light-leaking artifacts as highlighted in Figure 18. In such cases, we fall back to setting equal weights for all neighbor probes, which we refer to as 'relaxed interpolation'. We flag pixels calculated using relaxed interpolation in the alpha channel of the resolved texture; this information will be used later as a hint for the denoiser to try and discard the evaluated sample."*

§2.4.3: *"When interpolation fails and no history is available, we use the relaxed irradiance interpolation instead of outputting no irradiance."*

**Reference impl — `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/gi1.comp`**

```
1604:    bool use_backup = false;
1605:
1606:    if (dot(w, w) == 0.0f)
1607:    {
1608:        w = float4(1.0f, probes.y != kGI1_InvalidId ? 1.0f : 0.0f
1609:                       , probes.z != kGI1_InvalidId ? 1.0f : 0.0f
1610:                       , probes.w != kGI1_InvalidId ? 1.0f : 0.0f);
1611:
1612:        use_backup = true;  // for 'relaxed' interpolation in failure cases
1613:    }
...
1636:    float denoiser_hint = (use_backup ? 0.0f : 1.0f);
...
1638:    g_GIDenoiser_ColorBuffer[did] = float4(irradiance, denoiser_hint);
```

Then, in `ReprojectGI`:

```
4083:    float blur_mask = (!is_sky_pixel ? max(kGIDenoiser_MaxBlurMask - lighting.w, 0.0f) : -1.0f);
...
4087:        lighting += float4(color.xyz, color.w > 0.0f ? 1.0f : -1.0f);
```

A relaxed-interpolation pixel arrives with `color.w == 0.0`. The branch on line 4087 appends it with weight `-1.0`, pushing `lighting.w` *below* zero. The blur_mask emission `MAX_BLUR_MASK - lighting.w` then becomes large, the variable-radius blur in `FilterGI` widens, and the relaxed pixel is overwritten by neighbours.

**Our impl**

`Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl:282–304` (`SampleRadianceCache`):

```
float3 result;
if (totalWeight > 0.0)
{
    ...weighted average...
}
else
{
    float3 ITL = LoadIrradiance(tl * SH_TILE_SIZE, pixelNormal);
    ...
    result = wTL * ITL + wTR * ITR + wBL * IBL + wBR * IBR;
}
return max(result, float3(0.0, 0.0, 0.0));
```

The relaxed-equal-weight blend is structurally present in the `else` branch — this part matches. **But the function returns only `float3` and provides no signalling channel** for "this pixel was a relaxed-interpolation backup".

`Source/Shaders/HLSL/GIDenoise.comp:162–168` then drops the signal on the floor:

```
// Current sample. `color.w == 1` follows the design decision in CL1's
// task body — every SH-evaluated tap is treated as a valid sample
// (no Capsaicin-style denoiser_hint sentinel).
float3 l_IrradianceFromCache = SampleRadianceCache(l_ScreenCoord, l_Position_WorldSpace, N);
if (any(isnan(l_IrradianceFromCache)))
    l_IrradianceFromCache = float3(0.0, 0.0, 0.0);
float4 color = float4(l_IrradianceFromCache, 1.0);
```

The deliberate choice to hardcode `color.w = 1.0` (acknowledged in the in-source comment) is the **direct cause** of the AMBIENT_FLOOR symptom: relaxed-interpolation pixels enter the temporal history at full weight, the spatial blur never widens for them, the resulting irradiance estimate is whatever the relaxed blend produced (potentially zero if all 4 corners' SH coefficients themselves were zero, e.g. unspawned probe tiles), and `lightPassIndirectCompose.hlsl:38` masks the resulting black with the constant.

Then `Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl:22, 38`:

```
static const float3 LIGHTPASS_AMBIENT_FLOOR = float3(0.02, 0.025, 0.03);
...
l_IrradianceFromCache = max(l_IrradianceFromCache, LIGHTPASS_AMBIENT_FLOOR);
```

**Nature of divergence**

Two-step divergence:
1. The denoiser_hint signalling channel doesn't exist in the engine. `SampleRadianceCache` returns `float3` rather than `float4`; the relaxed-vs-converged distinction is lost between `RadianceCacheCommon.hlsl:286` and `GIDenoise.comp:165`.
2. Without the denoiser_hint, the spatial blur radius can't widen for relaxed pixels, so the bilateral filter doesn't absorb them, so they survive into LightPass as zero or near-zero irradiance, and `LIGHTPASS_AMBIENT_FLOOR` was added to paper over the black pixels.

**Impact**

The constant scene-tinted floor renders shadowed corners with a fixed cool-blue cast regardless of scene content (warm interiors look wrong, the actual paper-described mechanism — the spatial blur reaching into well-sampled neighbours — is bypassed entirely).

**Resolution needed**

Restore the denoiser_hint signalling end-to-end:

1. **`SampleRadianceCache` (RadianceCacheCommon.hlsl:237)** — change the return type to `float4`, packing `(irradiance, hint)` where `hint = (totalWeight > 0.0) ? 1.0 : 0.0`.
2. **`GIDenoise.comp:165–168`** — propagate the hint into `color.w` instead of hardwiring `1.0`. Update the temporal-accumulation logic at line 287–290 (the `lighting += float4(color.xyz, color.w > 0.0 ? 1.0 : -1.0);` shape from Capsaicin) so relaxed pixels enter history with the negative weight.
3. **`lightPassIndirectCompose.hlsl:22, 38`** — remove `LIGHTPASS_AMBIENT_FLOOR` and the `max(...)` clamp. The denoised irradiance, once the spatial blur is correctly widened on relaxed pixels by step 2, is the terminal value.
4. **GIDenoise blur-mask path** — `GIDenoise.comp:284` already computes `blur_mask = max(MAX_BLUR_MASK - lighting.w, 0.0)`. Once `lighting.w` can go negative for relaxed pixels (per step 2), this expression fires correctly without further change.

The deliverable is then "remove `LIGHTPASS_AMBIENT_FLOOR`" but the supporting work is in `SampleRadianceCache` and `GIDenoise.comp`, **not** in `lightPass.comp` or `LightPass.cpp`.

---

## Engine-side checkpoints (where each invariant should be enforced)

Pointers only; no audit work. The rendering-researcher's parallel investigation should cross-reference these:

| Invariant | Engine site |
|-----------|-------------|
| Relaxed-interpolation backup is structurally present | `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl:294–301` (the `else` branch of `SampleRadianceCache`). Already correct, but loses its "I was relaxed" signal at the return boundary. |
| Denoiser-hint signal threading from interpolation → denoiser | Boundary lost between `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl:303` (return type `float3`) and `Source/Shaders/HLSL/GIDenoise.comp:165, 168` (hardcoded `color.w = 1.0`). |
| Temporal accumulator interprets denoiser-hint | `Source/Shaders/HLSL/GIDenoise.comp:287–290`. Currently `color.w` is always 1.0 from upstream so the `color.w > 0.0 ? 1.0 : -1.0` branch is dead — the `-1.0` arm never fires. |
| Spatial filter widens for relaxed pixels via blur mask | `Source/Shaders/HLSL/GIDenoise.comp:284`. Expression is paper-correct, but needs `lighting.w` to actually go negative (currently can't, per the row above). The downstream filter is `Source/Shaders/HLSL/GIFilterHorizontal.comp` / `GIFilterVertical.comp` / `common/GIFilterCommon.hlsl`. |
| LightPass terminal compose | `Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl:22, 32–41`. Removing `LIGHTPASS_AMBIENT_FLOOR` is the *deliverable*, but only safe to do once steps 1–3 above are in place. |
| LightPass bindings — `in_WorldTileGrid` is **NOT** wired | `Source/ExampleProject/RenderingClient/LightPass.cpp:46` (sizes `m_ResourceBindingLayoutDescs` to 22, ending at sampler `s1` index 21). No structured-buffer binding for the world-tile grid exists in the LightPass slot table or in the `BindGPUResource` calls at `LightPass.cpp:294–318`. **This is correct under the paper-faithful resolution above** — the world cache should not be added to LightPass. The task description's request to wire it would itself be a divergence. |
| Existing world-cache read pattern for off-screen rays (reference shape only — should not be ported to LightPass) | `Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl:71–94`. Faithful to Capsaicin's secondary-ray world-cache lookup; this is where the world cache *does* belong. |
| LightPass kernel — no work needed at this site | `Source/Shaders/HLSL/lightPass.comp:139` calls `ComposeIndirectLighting`; that header is where the constant lives. The 152-line kernel itself is unaffected by the resolution. |

---

## Open questions

1. **Why was `color.w = 1.0` hardcoded?** The comment at `GIDenoise.comp:163–164` references "the design decision in CL1's task body". If that decision was based on a different understanding of how Capsaicin's denoiser_hint interacts with the temporal blender (e.g. believing it would discard samples rather than widen the blur), the rationale should be re-examined as part of TASK-6.3 closure. The paper-faithful behaviour is that relaxed pixels are *kept but down-weighted via spatial blur radius widening*, not discarded.
2. **Do we have a per-pixel SH evaluation noise level low enough that the spatial blur widening alone covers all the cases the AMBIENT_FLOOR was masking?** Capsaicin's denoiser is tuned for per-ray noise; our SH-projected probes are smoother. If the widened bilateral blur produces an unsatisfactory result on `GITestBox` after the fix, the answer is paper-faithful (more aggressive blur) rather than reintroducing a constant — the task should re-validate against `GPUPathTracerRayGen.hlsl` reference, not reintroduce the floor.
3. **`isShortRay` semantics** (per the prompt's question 3): The engine's `IsShortRay(RayTCurrent())` at `RadianceCacheClosestHit.hlsl:72` is a function of *ray distance* and is meaningful only inside an RT closest-hit shader where `RayTCurrent()` is defined. There is no equivalent at the LightPass site (no ray was traced). The task description's `(-viewDir, isShortRay)` descriptor at the LightPass site has no defined semantics — confirming that the proposed approach is not just unsupported by the paper but also has no natural value for the descriptor inputs.

---

## Not audited

- §2.2.3 Prefiltering Radiance — out of scope; the world-cache prefiltering is unrelated to LightPass.
- §3 Screen-Space Global Illumination (near-field) — out of scope; this is a separate hybrid component not implicated in the AMBIENT_FLOOR symptom.
- The `GIFilterHorizontal.comp` / `GIFilterVertical.comp` blur radius interpretation of the blur-mask texture — the audit confirms `GIDenoise.comp:284` writes the mask in the paper-faithful form, but the filter's consumption side was not opened. If step 4 of the resolution (`blur_mask` interpretation) turns out to differ from Capsaicin's `FilterGI` shape, that would be a separate row.
