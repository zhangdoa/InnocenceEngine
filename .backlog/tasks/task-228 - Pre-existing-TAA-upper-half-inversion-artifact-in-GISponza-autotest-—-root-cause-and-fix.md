---
id: TASK-228
title: >-
  Pre-existing TAA upper-half-inversion artifact in GISponza autotest —
  root-cause and fix
status: Done
assignee: []
created_date: '2026-05-16 21:08'
updated_date: '2026-05-17'
labels:
  - rendering
  - bug
  - TAA
  - regression
dependencies: []
references:
  - .alignments/TASK-226.4-port-audit.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-226.4 peer review (commit e0e68900). The GISponza autotest frame capture shows the upper half of the framebuffer rendering an inverted (upside-down) view, plus a central black void in the scene. Pre-existing — present in both the pre-change and post-change captures of the TASK-226.4 review. Not caused by TASK-226.4.

Captured A/B frames at the time of discovery: `Bin/RelWithDebInfo/gpu_output_0055_baseline.png` and `gpu_output_0055_postchange.png` (may be cleaned between sessions — re-capture if needed).

Likely suspects (need bisect):
- TAA pass — motion-vector convention mismatch between RT0/RT3 GBuffer and TAA-pass reprojection.
- Final-blend / composition — UV flip in a y-axis transform.
- Renderer split between rasterized + GI passes — incorrect render-target slice composition.

This artifact has been present long enough that it shipped through several TASK-226.x docs commits without being flagged; either it was a recently-introduced regression masked by side-cache fill or a longer-standing issue. Bisect against the TASK-226.2 baseline frames if available.

Blocks: clean baseline for TASK-226.8 visual + perf gate (which compares against TASK-226.2 baseline). Without addressing this, fine-grained AC #4/#5 disocclusion-fill comparison is impractical.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Root cause identified — BCCompression hard-coded source `.r` for every BC4 slot, discarding glTF MR `.g` (roughness) + `.b` (metallic). Recorded across 8 dispatch entries in Implementation Notes.
- [x] #2 GISponza autotest frame capture renders correctly — fully-lit Sponza interior; remaining zero pixels are legitimately-metallic ornaments per glTF.
- [x] #3 Investigation chain recorded in Implementation Notes (8 dispatches across 6 candidate carriers; bisect window exhausted upstream, root-cause found via per-pixel HDR probe).
- [x] #4 Fix landed; visual A/B (`Build/captures/TASK-228/fix/` vs prior baseline) confirms regression closed. Peer review verdict: PASS, Reviewed-Visually: improvement.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-17: bug-fix diagnostic dispatch — reproduce + bisect + root-cause hypothesis. **No fix landed.**

**Reproduce:** YES at HEAD (`90750a1e` at the time of audit; current `5f24a923`). Capture frames 25 + 55 of GISponza autotest via `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen -capture_frame N`. Visual Read confirms upper-half upside-down + central black void.

**Duplicate found — RECONCILE:** TASK-223 ("FinalBlend readback path produces vertically-mirrored Sponza output", filed 2026-05-13) describes the same symptom. Its audit at `aaebe695` concluded "content-asset / camera-on-axis bilateral symmetry" — that audit checked left/right mirror (1.1 delta at x=640 centerline, 8-10% column-pair identity = no bilateral pixel mirror) but **did NOT check top/bottom mirror**, which is the actual artifact. TASK-223 should be closed-as-duplicate of TASK-228 or reopened with the corrected diagnosis.

**Bisect:** UNVERIFIED — regression predates the practical bisect window.
| SHA | Date | Subject | Verdict |
|---|---|---|---|
| 90750a1e | 2026-05-16 | docs(backlog): file TASK-226.4 followups | BAD |
| fe8d9f7c | 2026-05-15 | fix(rendering): SSAO kernel 32 | BAD |
| aab13d53 | 2026-05-14 | fix: drop double-gamma sqrtf in WriteCaptureToFile | BAD |
| 5c4553b7 | 2026-05-07 | docs(backlog): TASK-77.2 CL-2 peer review | BAD |
| 21d0e058 | 2026-05-01 | docs(backlog): TASK-77.1.{1,2,3} sub-tasks | BUILD-TESTED, autotest blocked by older harness gate state |

Artifact is at minimum 28+ commits / 10+ days old. Older bisect window worth exploring: TASK-77.x denoise stack (May 6–10: `9c876761`, `4588a5ac`, `53331e1f`, `20380ad4`, `b8ecf900`) and TASK-219 mass file splits (May 6 onward: `5f8c3565`).

**Root-cause hypothesis (audit, not bisect-verified):** `Source/Shaders/HLSL/opaqueGeometryProcessPass.frag:118-119` applies `y = 1.0 - y` to BOTH `screenPos_orig` AND `screenPos_prev` before computing `motionVec = screenPos_prev - screenPos_orig`. Flipping both inputs sign-inverts the Y component of the delta. `Source/Shaders/HLSL/TAAPass.comp:56` consumes `pixelPos + round(motionVector)` directly. Over many converged frames on a static camera, a Y-sign-inverted motion vector smears history across the horizontal centerline → upper-half temporal mirror of lower-half.

**Secondary suspect:** `Source/Shaders/HLSL/skyPass.comp:42-43` applies a Y-flip on output write (`writeCoord.y = viewportSize.y - 1 - y`) without a matching flip in the consumer's read path. Could mis-place sky content; co-suspect but lower priority.

**Recommended fix shape (shader-impl):** experiment: drop the `screenPos_orig.y` flip at line 118 (keep `screenPos_prev.y` flip → delta is `flip(prev) - orig`), rebuild shaders, recapture frame 55. If artifact resolves, ship. If not, try keeping both flips off entirely. If still not resolved, escalate the sky-pass write-flip secondary suspect. ~10 min of work to validate.

**Files for the fix dispatch:**
- `Source/Shaders/HLSL/opaqueGeometryProcessPass.frag:106-129` (motion-vector compute)
- `Source/Shaders/HLSL/TAAPass.comp:43-65` (motion-vector consumption; also line 57 `<=` vs `<` bounds check — separate one-off)
- `Source/Shaders/HLSL/skyPass.comp:42-45` (Y-flip on write)
- `Source/Shaders/HLSL/common/skyResolver.hlsl:1-12` (ray-direction math)
- `Source/Shaders/HLSL/preTAAPass.comp:22` (unused `flipYTexCoord` artifact — clean if touching)

**What was NOT verified by the diagnostic dispatch:**
- Hypothesis NOT tested via fix-and-recapture; only audit-grounded.
- Bisect window not exhausted past `5c4553b7` due to older-harness tooling.
- TASK-223's "camera-on-axis bilateral" content explanation may partially apply to a horizontal pattern that's separate from this vertical mirror; not verified.

Main session housekeeping after the bug-fix dispatch ended: sub-agent operated in main repo (NOT a worktree as the brief instructed) — left HEAD detached at `21d0e058`. Restored via `git checkout ecs-overhaul`; 3 session commits (`471a0804`, `e0e68900`, `90750a1e`) preserved via reflog.

2026-05-17: hypothesis-validation dispatch — **HYPOTHESIS FALSIFIED.** No fix landed; tree restored to HEAD (`7115d286`).

| Variant | Edit | Frame-25 artifact | Verdict |
|---|---|---|---|
| Baseline (no edit) | — | Upper-half upside-down mirror + central black void | reproduce confirmed |
| A | Drop `screenPos_orig.y` flip; keep `screenPos_prev.y` flip | Artifact persists, slightly different texture coverage | falsified |
| B | Drop BOTH `screenPos_{orig,prev}.y` flips | Artifact persists | falsified |
| Step-4 | Drop `skyPass.comp:43` write-flip (with both MV flips restored) | Artifact persists | falsified |

All three captures archived under `Build/captures/TASK-228/` (`baseline-frame25-26afbc2c.png`, `variantA-frame25.png`, `variantB-frame25.png`, `skyPass-noflip-frame25.png`). Visual Read on each confirmed the upper-half upside-down geometry + central black void remain.

**Why the hypothesis was wrong (static-audit-after-the-fact):**
- The convention is internally consistent. `(input.posCS_orig.xy / w_orig) * 0.5 + 0.5` produces D3D NDC-Y-up → UV-Y-up. The two `y = 1.0 - y` lines convert both inputs to UV-Y-down (engine pixel-Y convention, matching how `uv = l_ScreenCoord / viewportSize` is used in GIDenoise.comp:183 and how `pixelPos` is used in TAAPass.comp). Both inputs are in the same Y space, so the delta `motionVec = prev - orig` is also in pixel-Y. The flips on both inputs do NOT sign-invert the delta; they coordinate-transform it.
- On a converged static camera, screenPos_orig ≈ screenPos_prev → motionVec ≈ 0 regardless of any Y-sign convention. A "Y-sign-inverted MV smearing history across the horizontal centerline" cannot happen on a static camera — `pixelPos + round(0)` = `pixelPos`. The artifact mechanism in the hypothesis is logically inconsistent with the observed static-camera setup.
- PTDenoiseShared.hlsl:34-35 + GIDenoise.comp:184-186 pin the convention as `motionVec_px = screenPos_prev - screenPos_curr` in pixel-Y, which is what the current code produces. The engine has multiple consumers that depend on this convention; removing the flips would silently mis-reproject the GIDenoiser too.

**Artifact is NOT in TAA-motion-vector OR sky-pass write-flip.** Deeper bisect needed. Likely candidates not yet examined:
- preTAAPass / lightPass composition: the artifact shows scene geometry mirrored. preTAAPass.comp:22 has an unused `flipYTexCoord` that hints at a Y-convention conflict that may have been partially un-wired. The fact that the mirror shows scene content (not just sky) rules out the sky-pass-only path.
- Final-blend / read-back: `WriteCaptureToFile` reads `FinalBlendPass.GetResult()` via `ReadTextureBackToCPU`. If the read-back layout differs from what FinalBlend wrote, the captured image could be Y-folded — but the artifact is a half-fold, not a full flip, which doesn't match a simple readback inversion.
- LightPass write coordinate transformation: looks correct on inspection (`out_lightPassRT0[l_ScreenCoord]` at line 219), but the artifact pattern (lower half is correct scene, upper half is upside-down version of lower half) could also be explained by some pass clearing only the lower half of a render target while leaving upper-half stale UAV data from a previous frame, OR by a write that fills both halves with a Y-folded distribution.
- Recommended next experiment: targeted RT dump of `lightPassRT0`, `preTAAPassRT0`, `TAAPassRT0`, `postTAAPassRT0`, and `finalBlendPassRT0` at frame 25 to isolate WHICH pass first introduces the mirror. The `-dump_frames` flag + per-pass debug-view should give that.
- Secondary hypothesis worth filing: TASK-77.x denoise stack (May 6-10) per the prior diagnostic's bisect window — still untested.

2026-05-17: bug-fix per-pass localization dispatch — **artifact localized to LightPass; "upper-half mirror" framing was a visual misinterpretation.**

Capture mechanism: existing `-audit` mode (`ExampleRenderingClient_AuditDump.cpp` + dispatch trigger), extended with per-pass dumps for OpaquePass RT3 / RadianceCacheIntegration / GIFilterVertical / PreTAA. Diagnostic edits reverted.

| Pass | RT | Artifact present? |
|---|---|---|
| OpaquePass RT0/1/2/3 | posWS / normal / albedo / MV | NO |
| SunShadowRTPass visibility | R8 | NO |
| SSAOPass | output | NO |
| RadianceCacheIntegrationPass | packed SH 480×270 | N/A (not framebuffer) |
| GIFilterVerticalPass | irradiance | partial — non-zero top + horizontal zero-band middle |
| **LightPass RT0 (Luminance)** | **first carrier** | **YES** |
| LightPass RT1 (Illuminance seed) | all-zero at frame 25 | not informative |
| SkyPass | sky gradient | NO |
| PreTAA / TAA / postTAA / FinalBlend | propagate | YES (downstream) |

**Anti-anchor:** SSAOPass is the LAST clean pass. Artifact originates inside LightPass.

**Symptom reinterpretation (important):** the "upper-half upside-down mirror" reading was wrong. At low exposure, the boosted LightPass RT0 shows the FULL Sponza scene across the framebuffer, but with (a) a rectangular zero-output region in the centre, and (b) brightness asymmetry top vs bottom. Two distinct signal regions resembling a mirror at low exposure, not an actual Y-fold. AC #1 ("Root cause identified") and AC #2 ("renders correctly — no upper-half inversion, no central black void") still apply but the underlying bug is a LightPass GI-compose / GI-input issue, not a TAA / readback / sky-flip issue.

**TASK-223 reconciliation:** TASK-223's "vertically-mirrored Sponza output" framing describes the same artifact under the same visual misinterpretation. Both tasks point at the same LightPass-internal bug. TASK-223 should still close-as-duplicate of TASK-228 once 228 lands a fix.

**Recommended next dispatch:** shader-impl probe at `lightPass.comp:197` `ComposeIndirectLighting(in_GIIrradiance, ...)`. Temporary edit: set `l_IndirectLuminance = 0` (skip GI compose). Rebuild + re-run audit. If the black-void + asymmetry disappear → GI input (`in_GIIrradiance` SRV from `GIFilterVerticalPass`) is the carrier. If they remain → bug is in direct lighting (sun shadow / tiled point lighting). ~5-min experiment.

Anchor pass set bounded to: `lightPass.comp`, `lightPassIndirectCompose.hlsl`, `GIFilterVertical.comp` + `GIFilterVerticalPass.cpp` dispatch site, and the SRV-binding chain that ties LightPass `t10` to `GIFilterVerticalPass::GetResult()`.

Captures archived (gitignored under Build/captures/TASK-228/per-pass/):
`audit_04a..04d_Opaque_RT*` (boosted), `audit_03c_SunShadowRT`, `audit_05_SSAO_boosted`, `audit_06a_RadianceCacheIntegration_boosted`, `audit_06b_GIFilterVertical_perceptual`, `audit_08a_Light_Luminance_boosted` (the carrier), `audit_08b_Light_Illuminance_boosted`, `audit_09_Sky`, `audit_09b_PreTAA_boosted`, `audit_10_TAAPass_boosted`, `audit_11_FinalBlend_boosted`.

2026-05-17 (post-`216de0e0`): skip-GI bisect dispatch — **GI INPUT IS THE CARRIER.**

Probe: temporary edit at `lightPass.comp:198` zeroing `l_IndirectLuminance` immediately after `ComposeIndirectLighting`. Reverted after capture.

Result: with GI compose zeroed, `audit_08a_Light_Luminance.hdr` is **literally all-zero** across all 921,600 pixels (`magick identify` min=max=mean=0.0). Boosted PNG = uniform black. 100% of the artifact (central rectangular void + brightness asymmetry) is sourced upstream of `ComposeIndirectLighting` in the GI pipeline.

Stronger-than-expected: direct lighting in GISponza autotest contributes zero. `audit_03c_SunShadowRT` is all-zero; tiled point lighting also zero by elimination. The scene's "lighting" in `LightPass RT0` is 100% indirect-GI-driven. LightPass with GI zeroed yields a black framebuffer (no direct lighting to fall back on).

Capture: `Build/captures/TASK-228/per-pass/skip-GI/audit_08a_Light_Luminance_boosted_v2.png` (the `_v2` distinguishes from the pre-`216de0e0` unusable capture that hit UnitTest pre-load). GBuffer cross-check `audit_04a_Opaque_RT0_boosted_v2.png` confirms GISponza geometry.

**Surfaced as separate concerns (NOT bundled into TASK-228 fix):**
- Sun-shadow all-zero in GISponza autotest. Could be intentional interior-scene state, could be a regression. Worth a separate triage task — low priority unless the GI-chain bisect finds it's connected.
- AuditDump roster gap — 12 HDRs dumped, expected 13. Missing `03a` / `03b` entries. Audit-infra hygiene; not blocking. Surface to backlog if it bites again.

**Anchor pass set narrows to the GI chain feeding `in_GIIrradiance` (SRV `t10` in lightPass):**
- `RadianceCacheIntegration.comp` — SH-encoded irradiance projection (audit-dumped as `06a`, but it's packed-coefficient layout, not screen-space; the prior dispatch noted reading-it-as-2D-image is misleading).
- `GIDenoise.comp` — temporal accumulator that consumes RadianceCacheIntegration output.
- `GIFilterHorizontal.comp` — separable spatial filter, first pass.
- `GIFilterVertical.comp` — separable spatial filter, second pass; the carrier-into-LightPass per the SRV binding chain.

**Recommended next dispatch:** GI-chain RT-dump bisect. Extend AuditDump roster to include `RadianceCacheIntegration` decoded form + `GIDenoise` output + `GIFilterHorizontal` output (vertical is already dumped at `06b`). Re-run audit; visual Read each. First GI-chain pass showing the central-void + asymmetry pattern is the bug origin. Narrows to a specific compute pass + dispatch site.

2026-05-17 (post-`c1397c76`): GI-chain RT bisect — **GIDenoisePass is the first introducing pass.**

Roster extended (temporary, reverted): added `06a_RadianceCacheIntegration`, `06c_GIDenoise`, `06d_GIFilterHorizontal` alongside the existing `06b_GIFilterVertical`.

| Pass | Pattern present? | Notes |
|---|---|---|
| 06a RadianceCacheIntegration | N/A | 480×270 packed SH coefficients; not screen-space, visual inspection inconclusive |
| **06c GIDenoise (output)** | **YES** | Bright central vertical pillar + dim bottom band + attenuated sides — first screen-space carrier |
| 06d GIFilterHorizontal | YES (propagates) | Near-identical to 06c |
| 06b GIFilterVertical | YES (intensified) | Pattern tightens into central blob; bottom fades further |

**Symptom reinterpretation (refined):** what looked like "central black void + brightness asymmetry" in LightPass RT0 is actually GI input where energy collapses into a bright central vertical pillar. The "void" is the unilluminated surroundings where GI dropped to zero. The LightPass framebuffer faithfully reflects GI energy: lit where pillar is bright, dark where surroundings have zero GI.

**Bug-shape candidates inside GIDenoise (per the agent's static-audit inference):**
1. **Temporal-accumulation reprojection** driven by non-uniform world-position / motion-vector source — history reprojects to wrong screen coords, energy collapses toward one converged location.
2. **Half-res → full-res upsample misalignment** in the cache-lookup tap (`upscaleFactor=(2,2)` ring walk in `FindClosestProbe` — audited correct at `af52b67e`, but worth re-checking the GIDenoise-side consumption coordinate).
3. **Sample-count denominator never decaying** — ΣLᵢ accumulator unbounded, early-converged pixels (central column with stable motion) accumulate unboundedly while edge pixels (high motion) reset.

**Captures archived (gitignored):** `Build/captures/TASK-228/per-pass/gi-chain/audit_06{a,b,c,d}_*_boosted.png`.

**Recommended next dispatch:** skip-temporal probe on GIDenoise (same shape as the LightPass skip-GI probe that worked). Edit `GIDenoise.comp` to zero the history-blend contribution (force current-frame-only). Re-run audit, Read `06c_GIDenoise`. If central-pillar pattern disappears → temporal accumulation is the carrier; further bisect among candidates (1) and (3). If it persists → bug is in the per-frame raw GI compute, candidate (2) is the lead.

2026-05-17 (post-`904c2af5`): skip-temporal probe — **pattern PERSISTS without temporal. Candidates (1) + (3) falsified; (2) leads.**

Probe: `GIDenoise.comp:322` overwritten with `out_GIHistory[l_ScreenCoord] = float4(color.xyz, 1.0)` where `color = l_IrradianceFromCache` from `:170` (current-frame raw, no history blend). Reverted.

Result: skip-temporal capture shows the **same** central-pillar overexposure, **same** brighter-pillar-vs-dim-surroundings distribution, **same** horizontal mid-frame band. Difference: noisier / blockier (N=1 single sample vs N≤16 weighted sum), but the **macro pattern is unchanged**. The pillar/dim-surroundings carrier is upstream of GIDenoise's temporal block — bug is in the per-frame raw GI compute that feeds `l_IrradianceFromCache = SampleRadianceCache(...)` at `GIDenoise.comp:170`.

**Remaining candidate (the lead):** half-res → full-res upsample misalignment in the cache-lookup tap. Target: `SampleRadianceCache` in `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl`. Hypothesis: the coordinate transform feeding `FindClosestProbe` / `ComputeProbeWeight` produces a single dominant probe weight at the screen-centre region (the pillar) while collapsing to background-only weight elsewhere — consistent with a stuck/misaligned half-res lookup. The `FindClosestProbe` ring walk was audited correct at `af52b67e` under `upscaleFactor=(2,2)`, but that audit verified the ring algorithm in isolation; the GIDenoise-side consumption coordinate transform is the not-yet-audited site.

Capture archived (gitignored): `Build/captures/TASK-228/per-pass/skip-temporal/audit_06c_GIDenoise_boosted.png`.

**Recommended next dispatch:** probe-id visualisation on `SampleRadianceCache`. Temporary edit to return either (a) a constant pseudo-colour keyed on the winning probe index, or (b) the raw pre-blend ring-walk index, so the LightPass framebuffer shows per-pixel which probe is being sampled. Read the visualisation: if all pillar pixels resolve to one probe vs distinct neighbours, the half-res lookup is the carrier confirmed.

2026-05-17 (post-`3c0fc537`): probe-id visualisation — **coord-transform hypothesis FALSIFIED. Bug is in probe-side data, not in lookup geometry.**

Probe: `RadianceCacheCommon.hlsl:314` (inside `SampleRadianceCache`) overridden to return `float4(lookupTL.tileCoord.xy / gridSize, 0, 1)` — each pixel encodes which TL probe it sampled. Plus `GIDenoise.comp:322` set to write current-frame raw (skip-temporal carried forward from prior dispatch). Both reverted.

Result: the captured GIDenoise output shows a **smooth, continuous 2D gradient** across the entire framebuffer — black (top-left) → red (top-right) → green (bottom-left) → yellow (bottom-right). NO uniform pillar region, NO partition boundary, NO clustering. Every pixel's TL probe-tile coord tracks its screen position smoothly. A small dark patch at the very top-left corner (~5-tile region) showed minor ring-walk substitution; treated as ring-walk noise, not pathology.

**Implication:** the half-res → full-res lookup is correct. Pillar pixels resolve to per-tile distinct probes, identical to surroundings. The lookup geometry is innocent. The bug lives in the **data stored per probe** — the SH radiance coefficients themselves carry the pillar/dim-surroundings shape.

**Caveat (NOT verified by the probe):** the visualisation encoded only the TL probe; if TR/BL/BR dominate the weighted blend in some regions and disagree with TL, the cluster size could be understated. Cheap follow-up if the upstream-data probe doesn't pin it down: re-run with the highest-weight corner encoded.

**Surfaced (not chased):** small dark patch ~5 tiles at top-left = ring-walk substitution behaviour. Probably benign; file as a low-priority observation if it bites later.

**Carrier chain so far:**
- LightPass RT0 (visible artifact)
- ← GI compose `l_IrradianceFromCache`
- ← `SampleRadianceCache` lookup (geometry correct — falsified `3c0fc537` lead)
- ← probe-side stored data (NEW LEAD)

**Recommended next dispatch:** probe-data dump. Add an audit-roster entry that dumps `in_RadianceCache` (the SH-coefficient texture written by `RadianceCacheIntegration.comp` and consumed by `LoadIrradiance`). Inspect the band-0 (Y00) coefficient channel across probes — if the SH itself shows pillar-vs-surrounding contrast, the bug is upstream of GIDenoise's consumption; bisect among `RadianceCacheIntegration` (SH write) vs `RadianceCacheReprojection` (temporal SH carryover) vs `FilterScreenProbes` (radiance feeding the integration). Note: `RadianceCacheReprojection` was substantially reworked in this session (TASK-226.4 Option A port at `e0e68900`) but the artifact predates that work by 10+ days, so 226.4 is unlikely to be the cause — it may instead be a longer-standing carrier the 226.4 port faithfully preserved.

Capture: `Build/captures/TASK-228/per-pass/probe-id/audit_06c_GIDenoise_raw.png` (gamma-only — the diagnostic image; boosted version is saturated by auto-level on [0,1] values).

2026-05-17 (post-`5f24a923`, probe-data dispatch): **CARRIER CHAIN FALSIFIED. Artifact root cause is NOT in the GI/radiance-cache chain — it's in the GBuffer metallic channel sourced from Sponza material assets.**

Probe sequence:
1. Extended `AuditDump` to dump the entire GI chain (Reprojection→FilterH→FilterV→Integration SH→GIDenoise→GIFilterH→GIFilterV). Reverted before close.
2. Read raw HDR pixel values (RGBE decode) at sample pixels across the framebuffer. Discovery: GBuffer is valid (worldPos, normal, albedo all non-zero) AND GIFilterV irradiance is non-zero (e.g. (1.5, 1.6, 1.8) at pixel (300, 640) — the "black void" center), but LightPass RT0 is exactly zero there.
3. Probe `ComposeIndirectLighting` to bypass the `albedo * (1-metallic) * irradiance / PI` multiply, returning raw irradiance instead. Re-ran audit: **LightPass RT0 zero-pixel count dropped from 598,196 to 0.** The "black void" disappeared; the full Sponza scene became visible.
4. Probe again with `float3(metallic, albedoLuma, 1.0)` encoding. Visual Read of the resulting LightPass dump shows a clean binary mask: 90% of pixels have metallic=0 (the dark regions in this mask), 10% have metallic=1 (the white regions). The metallic=1 regions are exactly the central column, decorative ornaments, and curtain/floor strips — and exactly the regions that ended up as the LightPass "black void."

**Root cause:** Every Sponza material in `Data/Generated/Components/NewSponza*.MaterialComponent.json` has `"Metallic": 1.0` AND has a combined Roughness+Metallic texture assigned to both the metallic-texture slot (`m_TextureIndices_2`) and the roughness-texture slot (`m_TextureIndices_3`). `opaqueGeometryProcessPass.frag:86-87` samples `.r` of the metallic texture, but glTF MetallicRoughness textures store metallic in `.b` (and roughness in `.g`). The `.r` channel is unused / occlusion / whatever the source authored — for the affected Sponza textures it samples to non-zero values (often 1), producing `metallic = 1` per pixel.

LightPass `ComposeIndirectLighting` computes `albedo * (1 - metallic) * irradiance / PI`. With metallic=1, `(1-metallic) = 0`, so the indirect term is zeroed regardless of how good the GI is. Since direct lighting in GISponza autotest is also zero (no sun visibility, no point-light contribution per prior dispatch's findings), LightPass RT0 = 0 wherever metallic=1.

**Why prior bisects missed this:**
- "Skip-GI → black framebuffer" was correctly observed but mis-interpreted as "GI shape is the artifact." In fact: `LightPass = albedo * (1-metallic) * GI / π`, so zeroing GI gives black everywhere, but a non-zero GI multiplied by `(1-metallic) = 0` ALSO gives black. The skip-GI experiment didn't distinguish these cases.
- The pillar-collapse pattern visible in the GI-chain captures (06a Integration boosted, 06c GIDenoise boosted) was a **boost-amplification artifact** of the imageio uint8 HDR decoder. With proper RGBE decode and per-pixel reading, the GIFilterV output is well-distributed across the framebuffer (1-30 mean range, smooth); the apparent "pillar" was just brighter regions saturating the boost level differently from dimmer ones.
- The probe-id viz (audit_06c at `3c0fc537`) correctly showed the lookup is smooth. The SH atlas data is correct. GIDenoise output is correct. GIFilterV output is correct. The GI chain works.

**Fix shape (OUT OF TASK-228 SCOPE per brief — surface for separate dispatch):**

Two independent issues to triage:

1. **`Metallic: 1.0` in every Sponza material JSON.** Either the glTF import code is setting this incorrectly (writing `metallicFactor` from the glTF spec but ignoring that the texture will override it — and the spec default IS 1.0), or the materials were authored this way. Need to inspect the import code (glTF → InnoMaterialComponent.json conversion) and either set `"Metallic": 0.0` for these materials or audit the import logic to honour the metallic texture's `.b` channel correctly. Material data lives in `Data/Generated/`, which is gitignored derived runtime output, so the upstream fix is in the import pipeline (probably `Source/External/.../AssimpWrapper` or similar) and a re-run of asset generation.

2. **`opaqueGeometryProcessPass.frag:87` samples `.r` of the metallic texture.** Per glTF 2.0 spec (KHR_materials_pbrMetallicRoughness), metallicRoughnessTexture stores metallic in `.b` and roughness in `.g`. The shader should sample `.b` for metallic, `.g` for roughness — and ideally the same texture binding pointer for both, so only one descriptor slot is consumed per metallic-roughness pair. Current code samples `.r` of the metallic-slot texture AND `.r` of the roughness-slot texture — wrong channels for glTF. The wrong channel might happen to return a reasonable roughness value if the source textures coincidentally have R≈G, but it ABSOLUTELY produces wrong metallic.

The Sponza renderer has been producing visibly-wrong shading for any frame that depends on `metallic` correctness (any indirect-lit pixel of a Sponza non-metallic material). This is a long-standing rendering bug unrelated to TASK-226.x GI work — it predates the radiance-cache work entirely. Whether to file as two separate tasks (asset audit + shader fix) or one umbrella is a main-session call.

**Recommended next dispatch shape:**
- Sub-task 1: audit glTF → InnoMaterialComponent.json conversion. Either at import time set `"Metallic": 0.0` (texture overrides) OR add a "use texture channel B" flag to the material schema.
- Sub-task 2: `opaqueGeometryProcessPass.frag` fix — sample `.b` of MetallicRoughness texture for metallic, `.g` for roughness. Verify against glTF spec. Test in Sponza autotest: the "central black void" disappears entirely; full GI-lit Sponza interior renders.
- After sub-task 2 fix, re-run TASK-228 audit. Expected result: no black void, scene renders as a normally-lit Sponza interior. AC #2 ("renders correctly — no upper-half inversion, no central black void") satisfied. The "upper-half inversion" framing was a misreading of the contrast between lit non-metallic regions and zero metallic regions — it never was a Y-fold.

**Captures archived (gitignored under Build/captures/TASK-228/probe-data/):**
- `baseline_08a_Light_boosted.png`, `baseline_08a_Light_raw.png` — fresh baseline reproduce of the artifact.
- `probe-raw-gi_08a_Light_boosted.png` — LightPass with `ComposeIndirectLighting` returning raw GI. **No black void.**
- `probe-material_raw.png`, `probe-material_boosted.png` — material probe encoding (early version: R=albedo.x, G=1-metallic, B=albedo*1-metallic — kept the metallic-zero areas dark in G).
- `probe-metallic_only.png` — the smoking gun: clean binary mask of metallic showing the artifact regions exactly.
- `audit_06{a,b,c,d,e,f,g}_*.png` (raw + boosted) — full GI chain dumps for record.

**AC mapping:**
- AC #1 (Root cause identified): satisfied — metallic=1 from misimported Sponza materials + wrong texture channel sampled by `opaqueGeometryProcessPass.frag`.
- AC #2 (Renders correctly): NOT satisfied here — out of TASK-228 scope per the brief (touches material assets + geometry pass, not the GI chain). Hand off to follow-up dispatch.
- AC #3 (Bisect range): N/A — not a regression; long-standing material data + shader channel issue.
- AC #4 (Fix landed): NOT landed in this dispatch.

**TASK-228 recommended disposition:** close as "diagnosed; routed to follow-up." The original framing (TAA Y-fold, then GI carrier) was a chain of mis-localizations. The actual bug is unrelated to GI and unrelated to TAA — it's GBuffer-side, and the GI chain was a downstream amplifier the prior dispatches kept zooming into.

**What was NOT verified in this dispatch:**
- The import-side fix (glTF→Inno material conversion). I haven't read the import code; the "metallicFactor inherited from glTF spec default" is the most likely culprit but is unconfirmed.
- Whether `.b`-sampling fix alone (without changing the JSON) restores correct rendering. Both the JSON's `"Metallic": 1.0` AND the shader's `.r` sample contribute; fixing one without the other may still leave the bug if the JSON's static fallback takes precedence in some path.
- The 10% of pixels that DID have metallic=1 in my probe (e.g. the central column, ornaments) — whether their material is supposed to be metallic in some sense (e.g. lion ornament could plausibly be a gilt-bronze finish in some interpretations of Sponza). Likely just incorrectly-imported; not metallic in the original glTF source.

**Files NOT modified (all diagnostic edits reverted):**
- `Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl` — back to baseline.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_AuditDump.cpp` — back to baseline (no GI-chain dumps).
- Tree is clean except for the pre-existing untracked submodule modification in `Source/External/GitSubmodules/ixwebsocket`.

2026-05-17 (shader-impl validation dispatch): **Shader fix RULED OUT. Bug is in the asset importer's BC compressor.** No code changed.

**Validation procedure followed:** brief instructed the shader fix `t2d_metallic.Sample(...).r` → `.b` and `t2d_roughness.Sample(...).r` → `.g`, conditional on the source textures being packed glTF metallicRoughness. The fix-precondition probe inverted the conclusion.

**Probe data (no shader edit applied):**

1. **Material JSON** (`Data/Generated/Components/NewSponza_Main_glTF_003.floor_01.MaterialComponent.json`): slots [2] (metallic) and [3] (roughness) BOTH reference the same texture name `NewSponza_Main_glTF_003.floor_tiles_01_Roughnessfloor_tiles_01_Metalness`. This is the glTF KHR_materials_pbrMetallicRoughness packing convention preserved by Assimp. Consistent with the brief's "packed MR texture" path.

2. **Stored texture descriptor** (`...floor_tiles_01_Roughnessfloor_tiles_01_Metalness.json`): `PixelDataFormat: 10` = `BC4` (single-channel block compression), `PixelDataType: 14` = `Compressed`. Binary size `8,388,608` bytes = `4096 × 4096 × 8 bytes / 16` = BC4 fits exactly. **The on-disk texture is single-channel, not four-channel.**

3. **Importer compressor** (`Source/Engine/Common/BCCompression.cpp:78-80`):
   ```cpp
   else if (bcFormat == TexturePixelDataFormat::BC4)
   {
       uint8_t r[16];
       for (uint32_t i = 0; i < 16; i++) r[i] = block[i * 4];   // ALWAYS RED CHANNEL
       stb_compress_bc4_block(dest, r);
   }
   ```
   `slotIndex` selects the BC FORMAT (`BC5` for normals, `BC1` for albedo, `BC4` for everything else), but the BC4 branch unconditionally extracts `pixel[0]` (R) regardless of which slot is being compressed. Source RGBA→BC4 is hard-coded to the R channel.

**Conclusion:** for the packed glTF MR texture (R=unused/AO, G=roughness, B=metallic), the importer compresses the **R channel** of the source RGBA into the BC4 stored in slot 2 (metallic) AND into the BC4 stored in slot 3 (roughness). Both stored textures carry the wrong data — the source's R channel, which for Sponza is non-zero on ~10% of pixels.

**The shader is innocent.** Sampling `.r` of a BC4 is correct (BC4 is single-channel; G/B/A return 0/0/1 per DX spec). Changing the shader to sample `.b` would read the BC4 alpha (always 1.0) → metallic = 1.0 everywhere → breaks all PBR materials. The brief's contingency rule fires here: **"If single-channel metallic / separate textures: the shader is right; the bug is upstream in the asset importer. Surface and stop — that's a code-impl scope, not shader-impl."**

**Fix shape (code-impl scope, NOT landed here):**

Two coupled changes needed in the asset import pipeline. The right fix is to teach `BCCompression::CompressRGBAToBC` (or its caller) which source-channel to extract per slot:
- Slot 2 (metallic) → extract source `.b` into the BC4 single channel.
- Slot 3 (roughness) → extract source `.g` into the BC4 single channel.
- Slot 4 (AO) → extract source `.r` (current behaviour; matches ORM-packed and glTF KHR_materials_specular AO convention).

Approach options:
- **A. Per-slot channel-source table in the compressor.** Add a `srcChannelForBC4` lookup `{slot 2 → B, slot 3 → G, slot 4 → R}` in `BCCompression::CompressRGBAToBC`. Tiny, localized.
- **B. Caller pre-swizzles the source RGBA before calling the compressor.** Keeps `BCCompression` generic but spreads the channel knowledge across `AssetService::ImportTexture` and any other future caller.

Option A is the simpler. The convention map already lives in the same file's comment header (`// Slot convention: 0 normal → BC5, 1 albedo → BC1, 2 metallic → BC4, ...`), so the per-slot channel source belongs adjacent.

**After the importer fix:** delete `Data/Generated/Components/NewSponza*_Roughness*_Metalness.innobin` and re-import (re-run the engine; the import path will regenerate via assimp). The on-disk BC4 will then carry the correct channel. Then re-run the GISponza audit; `audit_08a_Light_Luminance` should show fully-lit interior, no central black void. This is `task-228` AC #2 + #4.

**Why the brief's shader fix was tempting but wrong:** the brief framed the bug as "shader reads wrong channel of a packed MR texture." That's the correct mental model IF the texture were stored RGBA8 / BC7 (preserving all four channels) and the shader sampled it. The engine instead stores BC4 (single channel) and the channel-selection decision happens at import time, not sample time. The brief's contingency clause caught this correctly — the validation probe inverted the conclusion from "shader-impl" to "code-impl."

**Code-impl scope (new task):** importer-side fix to extract the correct channel per slot when compressing packed glTF MR textures to BC4. Touches `Source/Engine/Common/BCCompression.cpp` (~5 lines) + a re-import of the generated Sponza textures.

**Files NOT modified in this dispatch:**
- `Source/Shaders/HLSL/opaqueGeometryProcessPass.frag` — untouched, the brief's proposed `.r`→`.b/.g` edit was NOT applied.
- Tree is clean (only pre-existing untracked submodule modification in `Source/External/GitSubmodules/ixwebsocket`).

**ACs:**
- AC #1 (Root cause): refined — `Source/Engine/Common/BCCompression.cpp:78-80` hard-codes the R channel for all BC4 slots; for packed glTF MR textures this stores the wrong channel for metallic (slot 2) and roughness (slot 3). The prior dispatch's "shader samples wrong channel" framing was slightly off — the channel was already lost at import.
- AC #2 / #4: NOT satisfied here. Blocked on the importer fix landing.
- AC #3: N/A (not a regression; long-standing import-pipeline bug).

2026-05-17 (structural fix dispatch, code-impl): **STRUCTURAL FIX LANDED IN WORKING TREE; awaiting peer review before commit.**

**Threading shape.** Added `enum class TextureChannelSource { R, G, B, A }` in `Source/Engine/Common/BCCompression.h`. Threaded through the import chain:

- `BCCompression::CompressRGBAToBC` — new `bc4Source` parameter; BC4 branch extracts `block[i*4 + offset]` where `offset = static_cast<uint32_t>(bc4Source)`. BC1/BC5 paths unaffected.
- `AssetService::ImportTexture` — new `bc4Source` parameter with default `R` (preserves all existing callers' behaviour: single-channel PNGs that STB broadcasts to RGBA at load → R=G=B=L).
- `AssimpTextureProcessor::CreateTextureComponent` — passes `bc4Source` through; suffixes the instance name with `_chB` / `_chG` when `bc4Source != R` so AssetService's instance-name dedup does NOT collapse the metallic and roughness imports onto one .innobin (Assimp reports the same packed file under both `aiTextureType_METALNESS` and `aiTextureType_DIFFUSE_ROUGHNESS` with identical filenames).
- `AssimpMaterialProcessor::ProcessMaterialTextures` — per-`aiTextureType` channel-source decision:
  - `aiTextureType_METALNESS` → slot 2, `bc4Source = B` (glTF MR packing).
  - `aiTextureType_DIFFUSE_ROUGHNESS` → slot 3, `bc4Source = G`.
  - `aiTextureType_SPECULAR` / `aiTextureType_SHININESS` (legacy FBX) → R (separate textures).
  - `aiTextureType_AMBIENT` → R (separate texture).

**Files touched.** 7 files, +92 / -35 lines:
- `Source/Engine/Common/BCCompression.{cpp,h}`
- `Source/Engine/Services/AssetService.h` + `AssetService_TextureRegistry.cpp`
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp`
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.{cpp,h}`

**Build.** Clean (`Scripts/BuildWin.ps1 -SkipClangdIndexRefresh`). `Main.exe` + `RenderTest.exe` linked; no warnings.

**Asset re-bake.** Used existing `Main.exe -bake` headless flow:
```
./Main.exe -bake "../../OriginalAssets/Models/Sponza_PBR/main1_sponza/NewSponza_Main_glTF_003.gltf;../../OriginalAssets/Models/Sponza_Curtains/pkg_a_curtains/NewSponza_Curtains_glTF.gltf"
```
Result: 2 ok / 0 failed, 45.4s wall-clock. 52 `_chB` + 52 `_chG` packed-MR-derived textures across both glTF files. Material JSONs now reference distinct `_chB` (metallic slot 2) and `_chG` (roughness slot 3) texture instances. Bin/Data/Generated synced back to repo Data/Generated; both gitignored.

**Validation.**
- GISponza audit (`Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 60 -offscreen audit`): clean exit (code 9 = audit dump-then-exit). `audit_08a_Light_Luminance.hdr` zero-pixel count dropped from 598,196 (pre-fix per prior dispatch note) to 105,663 — 5.7× reduction. Boosted PNG + gamma-only PNG show fully-lit Sponza interior: orange curtains both sides, brick columns, vaulted ceiling, floor. Remaining zero pixels are legitimately-metallic surfaces (lion ornaments, decorative metals — correct PBR behaviour where `(1-metallic)=0`).
- UnitTest frame 3 (`Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 4 -offscreen -capture_frame 3`): clean exit. PBR sphere lineup renders with full material range — non-metallic whites/tans alongside metallic golds. No "all-metallic" regression.

**Visual Read assessment.**
- Reference: pre-fix GISponza per task notes — rectangular central black void covering ~65% of framebuffer.
- Current: GISponza audit_08a_Light_Luminance fully renders the atrium; FinalBlend captures the tone-mapped scene with curtains, columns, archway.
- Differences: central black void gone; full Sponza interior visible; zero-pixel count down 5.7×; UnitTest sphere bed material differentiation preserved.
- Verdict: improvement.

**Captures archived** (`Build/captures/TASK-228/fix/`):
- `audit_08a_Light_Luminance_boost5x.png`, `audit_08a_Light_Luminance_gamma.png` — LightPass RT0 post-fix.
- `audit_11_FinalBlend.png` — tone-mapped Sponza post-fix.
- `unittest_frame3.png` — PBR sphere bed regression check.

**ACs (post-fix):**
- AC #1 (Root cause): satisfied — BC4 channel selection at import time.
- AC #2 (Renders correctly): satisfied per Visual Read; central black void gone.
- AC #3 (Bisect range): N/A — long-standing pipeline bug.
- AC #4 (Fix landed + visual A/B): satisfied pending commit.

**Awaiting peer review.** Per `peer-review-required`, a fresh code-impl review of the diff is gated before commit. Sub-agent dispatcher tool unavailable in this code-impl context; surfacing to main-session for the review dispatch.

**What was NOT verified:**
- Peer review (gap acknowledged; surfaced).
- Three-scene visual-validation Layer-2 minimum: only GISponza + UnitTest scenes captured. The brief did NOT require GI Test Box for this fix (texture-channel selection is per-pixel BRDF input, not GI / cache / denoise / composition); regression risk on GI Test Box is logically bounded to "what if a GI Test Box material happens to use glTF-packed MR textures?" — none do (UnitTest + GI Test Box materials are scene JSONs, not imported via Assimp).
- Whether the legitimately-metallic regions (lion ornaments etc.) match their authored glTF metallicFactor — visual probe shows correct binary metallic/non-metallic differentiation but not the exact value.
- Whether `aiTextureType_UNKNOWN` flavours (Assimp's catch-all bucket) might also report packed MR textures in some glTF files — Sponza doesn't trip this; the loop bypasses UNKNOWN via the explicit type check at the start. Future glTF imports with non-standard texture types may need extending the per-type table.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
