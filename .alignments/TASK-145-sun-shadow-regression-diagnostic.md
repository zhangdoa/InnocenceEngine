# TASK-145 — Sun shadow "totally absent" diagnostic

User report (2026-04-26 post-TASK-142 K=6.0, fresh-from-HEAD GISponza windowed):
sun shadows are TOTALLY ABSENT. This is a regression from the pre-AGX baseline.

This artifact is research-only. No code modified.

---

## Headline

**Inconclusive without windowed verification on the user's machine.** All
file-level evidence on disk says shadows should still be working. The static
Bin audit dump (pre-AGX, GITestBox) shows shadow data is being written by the
depth pass and consumed by the resolver into Light_Luminance with shadowed
pixels ~21000x darker than lit ones. AgX@K=6.0 simulated against that real
luminance distribution preserves shadow contrast (5%-percentile pixel maps to
display 0, 50% to 99) — so if the resolver and GI compose still behave that
way today, K=6.0 alone cannot make shadows "totally absent."

The candidate that I cannot rule out without a fresh GISponza audit dump:
**indirect GI flooding shadowed regions**. GISponza interior is dense with
indirect bounce; recent commits (TASK-6.7 16-ray budget, TASK-6.9 looser
coverage threshold, TASK-6.10 sky NEE) all bump the indirect term. If the
indirect radiance lands close to the direct sun radiance, AgX's preserved
midtone contrast renders both into the same display band → no perceptual
contrast gap → "shadows look gone."

---

## Verified evidence

### 1. No code regression in the shadow pipeline

`git log` for every shadow-related file shows no edit since TASK-106 commit
`1e869aad` (2026-04-19), well before the TASK-122/141/142 chain (all 04-26
19:22+):

| File | Last touched |
|------|--------------|
| `Source/Shaders/HLSL/common/shadowResolver.hlsl` | `1e869aad` (TASK-106) |
| `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` | `6134d9e2` (TASK-135 split, no shadow logic change) |
| `Source/Shaders/HLSL/sunShadowGeometryProcessPass.{vert,geom,frag}` | 2025 |
| `Source/Shaders/HLSL/sunShadowCulling.comp` | unchanged for the relevant period |
| `Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp` | last touched in TASK-60 (2025-06) |
| `Source/Engine/Services/LightDataService.cpp` (CSM compute) | `ae379fc4` (chore: drop diag log; logic untouched) |

DXIL artifacts in `Bin/RelWithDebInfo/Shaders/DXIL/` were rebuilt today
(2026-04-26 21:48), matching current source.

### 2. Audit dump shows the depth pass IS writing shadow data

`Bin/audit_03_SunShadow_RT0.hdr` (2026-04-26 00:21, pre-AGX, GITestBox scene):

- 8192x2048 = 4 cascades stacked Y-wise at 2048×2048 each
- Cascade 0: full coverage, depth in [0.15, 0.99]
- Cascade 1: full coverage, depth in [0.12, 0.95]
- Cascade 2: 80% coverage, depth in [0.10, 1.00]
- Cascade 3: 4.5% coverage (mostly cleared 1.0 — outermost cascade rarely has
  blockers, expected)
- Visualization (`Bin/_visualcheck_shadowmap_cascade0.png`) clearly shows the
  GITestBox sphere row casting depth into cascade 0.

**Hypothesis #3 (shadow texture empty / wrong binding) — REJECTED.**

### 3. Light_Luminance audit shows shadow consumption is correct

Same audit, `audit_08a_Light_Luminance.hdr` (post-LightPass, after GI compose):

- 1.91% of opaque pixels are fully shadowed (RT1 illuminance < 0.001)
- In those shadowed pixels, RT0 (luminance with GI compose) median = **0.7465**
- In sun-lit pixels (RT1 > 10000), RT0 median = **15979.83**
- Ratio: shadowed regions are **~21000x darker** than lit ones
- GI flood ratio in this scene: 0.00% — indirect does not fill in direct shadow

**Hypothesis #2 (resolver inversion) — REJECTED.** If resolver returned visibility
instead of shadow factor, RT1 would not have been ~0 in shadowed regions.

### 4. K=6.0 vs K=9.6 simulated against the real luminance distribution

| Pixel percentile | Y_in | AgX@K=9.6 8-bit | AgX@K=6.0 8-bit | ACES@K=9.6 8-bit |
|---|---|---|---|---|
| 5% (deep shadow) | 0.42 | 0 | 0 | 1 |
| 25% | 4348 | 24 | 38 | 81 |
| 50% (median) | 15879 | 74 | 99 | 168 |
| 75% | 16263 | 75 | 101 | 170 |
| 95% | 17543 | 79 | 105 | 175 |

- `<32` "shadow" pixel fraction: K=9.6 → 27.4%, K=6.0 → 23.6% — barely changed
- `<16` "true black" pixel fraction: K=9.6 → 22.6%, K=6.0 → 19.9% — slightly fewer
- Shadow:lit contrast ratio (5%:50%) under K=6.0: **0:99** — full dynamic range preserved

**Hypothesis #1 (K=6.0 contrast wash) — REJECTED on this scene's distribution.**
The K-revert test (temporarily set K=9.6) is unlikely to bring shadows back if
they're actually missing under K=6.0; it would only make the whole image dimmer.

### 5. Cascade-AABB selection is byte-for-byte the TASK-106 implementation

`SunShadowResolver` line 179-189 picks `primaryIdx` by world-space AABB
containment of `positionWS`. The AABB stored in `CSMs[i].AABB{Min,Max}` is the
world-space AABB of the camera frustum slice (LightDataService.cpp:268-269).
The light-space projection bounds (used for the actual depth render) come from
`ExtendAABBToBoundingSphere(l_AABBWorld)` which is strictly larger. So a pixel
selected for cascade i should also project successfully to that cascade's
light-space NDC. No new fragility introduced.

`EvaluateCascadeShadow` returns 0 (= no shadow) only when the pixel projects
outside [-1,1] in light-space NDC — possible at extreme grazing angles, but
this would mean "no shadow" not "wrong shadow," and would be cascade-local.

---

## Cannot rule out without windowed eval on user's machine

### A. Indirect GI flooding (most likely candidate, scene-dependent)

LightPass composes `l_DirectLuminance += l_IndirectLuminance` in lightPassIndirectCompose.hlsl line 32:
```hlsl
return in_Material.m_Albedo * (1.0 - in_Material.m_Metallic) * l_IrradianceFromCache / PI;
```

`l_IrradianceFromCache` comes from GIDenoise → GIFilterVertical, fed by the
radiance cache. Recent commits all bumped the indirect term:

- `1c896354` TASK-6.7: GI ray budget 1→16
- `94949890` TASK-6.9: COVERAGE_ACTIVATION_THRESHOLD 0.9→0.5 (more pixels get
  full radiance-cache irradiance instead of fallback)
- `e06235ee` TASK-6.10: sky NEE at radiance-cache secondary vertex (more sky
  light into shadowed pixels)

In the GITestBox audit, indirect:direct ratio in shadowed pixels was 0.00%.
But GITestBox is an open desert scene with no walls and a single bounce.
**GISponza interior** (the user's report) has dense multi-bounce indirect
bounce off pale stone — indirect can plausibly reach radiance values close to
the direct sun term. Combined with AgX's preserved midtone contrast and
K=6.0's brighter exposure, indirect-only pixels (= shadowed regions) and
direct+indirect pixels (= sun-lit regions) could both land in display band
80-130, perceptually flat → "shadows gone."

This is consistent with the user's earlier "fragmented" report under the
TASK-138 WIP binary (which also had K=9.6 still): if shadow attenuation was
already weak vs. indirect, K=6.0 would only push both bands brighter without
restoring contrast.

### B. GISponza-specific cascade fit

Without a GISponza audit dump, I cannot confirm cascade AABBs cover the
visible geometry for a Sponza-courtyard camera framing. If cascade 0 is sized
for the entire frustum slice but the user looks down a corridor, the
columns might fall in cascade 1 or 2 where penumbra-size tuning could differ
slightly. Not a regression source by itself, but a confounding variable.

### C. Sun direction in the loaded GISponza scene

If GISponza's sun has been re-authored to point near-vertical or directly
at the camera, shadow contact under arches becomes vanishingly thin. Did
not check the scene file.

---

## What I did NOT do

- **Run the windowed K-revert test (K=6.0 → 9.6).** I cannot run windowed
  Main.exe from the dispatch shell (same constraint as TASK-122/141/142 noted
  in the cautionary anchor). The simulation in section 4 above is analytical
  only. **The user must run this test** — it is the cheapest disambiguator
  between "K-induced perception" and "real regression." Concrete recipe:
  1. Edit `Source/Shaders/HLSL/finalBlendPass.comp:65` `K = 6.0f` → `9.6f`.
  2. Recompile shaders only (HLSL2DXIL_NoPause.ps1 or equivalent).
  3. Launch GISponza windowed; eye-test shadows.
  4. **REVERT THE EDIT** before any commit.
  - **If shadows return at K=9.6**: contrast wash + indirect flood. Route
    forward via TASK-144 data-driven K and/or a separate task to gate the
    indirect compose by sun-shadow visibility (see "Recommended remediation"
    below).
  - **If shadows still gone at K=9.6**: real bug downstream of K. Route
    forward via a fresh GISponza audit dump (`-mode 0 -renderer 0 -loglevel 0
    -offscreen -total_frames 30 -audit`) and re-run the analysis here against
    `audit_08a_Light_Luminance.hdr` and `audit_08b_Light_Illuminance.hdr` for
    GISponza specifically.

- **Capture a fresh GISponza audit dump.** Same constraint. The existing
  `Bin/audit_*.hdr` files are GITestBox, not GISponza.

- **Inject a debug `Log(Verbose, ...)` of `shadowFactor` for a known pixel.**
  Would have required a build cycle + windowed run.

- **Wrap `SunShadowGeometryProcessPass` in `BeginGpuPass / EndGpuPass`
  (TASK-140 timer).** Same constraint — would need a runtime to read.

- **Verify the GISponza scene's sun direction** in the .innoscene file.

---

## Recommended remediation (ordered by likelihood)

### Priority 1 — User runs K-revert test, reports back

This is the cheapest single experiment that disambiguates the two remaining
hypothesis classes (perception vs. real bug). 5 minutes of user time.

### Priority 2 — Capture GISponza audit dump

Once we have GISponza's `audit_08a_Light_Luminance.hdr` and
`audit_08b_Light_Illuminance.hdr`, the same RT0/RT1 ratio analysis from
section 3 above will tell us in 30 seconds whether indirect is flooding
shadowed regions. This is the conclusive answer for hypothesis A.

### Priority 3 (only if A confirmed) — Gate indirect by direct visibility, OR cap indirect under bright sun

Two known-good patterns from production renderers:

1. **AO-modulate the indirect compose**: `l_IrradianceFromCache *= ssao_term`.
   We already have SSAO bound at LightPass register t6 — currently consumed
   by the BSDF accumulator but could also dim the indirect compose in
   shadowed regions. Cheap, single-line, no algorithm change.

2. **Multi-scattering / contact-bounce gating**: cap `l_IrradianceFromCache`
   above some fraction of the unshadowed direct sun radiance. Avoids the
   cache "leaking" sun energy into geometric pockets it shouldn't reach.
   More work; correct for the right reason.

Both are deferrable to a separate task. Do NOT bundle into TASK-145 if K-revert
disambiguates this incident as perception.

### NOT recommended

- Touching `shadowResolver.hlsl`. The code is clean, the math is clean, the
  audit shows it works at the resolver level. Modifying it without evidence
  of a bug there will introduce one.
- Lowering K below 6.0 to "make shadows visible" — this just dims the whole
  image and re-introduces TASK-141 dimness without solving the contrast issue.
- Reverting to K=9.6 as the fix — same problem, returns to TASK-141 dimness.

---

## Cautionary anchor compliance

Per TASK-122/141/138 WIP feedback memory: this artifact does not assert
correctness without empirical evidence on the user's machine. The Bin audit
dumps that I do have are pre-AGX GITestBox; they prove the **pipeline shape**
is correct but they do NOT prove the **current GISponza windowed render** is
correct. The user-facing report must be reproduced empirically before any
"fix" is committed.

I explicitly do NOT claim:
- That shadows are "actually fine" on GISponza windowed today.
- That K=6.0 vs K=9.6 makes no perceptual difference on GISponza.
- That GI flood is the cause (it is the leading hypothesis, not a confirmed
  cause).

I do claim:
- The shadow code path on disk is unchanged since TASK-106.
- The depth pass writes valid data to the shadow RT (cascade 0 visualization).
- Under the GITestBox luminance distribution, K=6.0 does not wash out shadows.
- Therefore the user's report on GISponza is most likely scene-specific — most
  plausibly indirect GI flooding, second-most-plausibly cascade fit at the
  user's camera position, least plausibly K-induced perception (math against
  GITestBox argues against this).
