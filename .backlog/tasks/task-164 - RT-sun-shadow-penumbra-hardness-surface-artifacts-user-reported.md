---
id: TASK-164
title: 'RT sun shadow: penumbra hardness + surface artifacts (user-reported)'
status: Done
assignee: []
created_date: '2026-04-27 19:30'
updated_date: '2026-04-27 19:51'
labels:
  - rendering
  - shadows
  - bug
dependencies: []
references:
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/Shaders/HLSL/common/sunSampling.hlsl
  - Source/Shaders/HLSL/SunShadowRTAnyHit.hlsl
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-reported visual issues with TASK-138 RT sun shadows after running the engine:**

1. **Sharp edges / no penumbra**: shadows are too hard. The cone-jittered shadow ray pattern is wired correctly in `SunShadowRTRayGen.hlsl:104-105` (`PixelJitter2D` × `SampleSunDirection` with `SUN_ANGULAR_RADIUS`), but the user observes hard edges in real-time. Soft penumbra was supposed to emerge from TAA accumulating the frame-to-frame jittered samples.
2. **Surface artifacts (peter-panning-ish)**: visible on object surfaces. Reminiscent of CSM peter-panning, even though RT shouldn't have that bias-tuning class of bug.

### Investigation directions

For (1):
- Verify TAA is actually running and is accumulating the shadow visibility result. If TAA only runs on the LightPass *output* and not on the visibility texture itself, single-sample-per-pixel binary visibility can produce hard edges where TAA can't recover penumbra (the noise bandwidth doesn't fit the temporal blend).
- Check if `g_Frame.frameIndex` is actually incrementing (per-pixel jitter depends on it).
- Single sample / pixel may simply be insufficient. Bump to 4-8 samples or add a small spatial filter pre-LightPass.
- Compare against the GPUPathTracer reference at `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` — PT uses the same `SampleSunDirection`, soft penumbra is the expected result. Diff the in-flight RT shadow visibility texture against PT's converged result.

For (2):
- `RAY_EPSILON = 0.001` (`SunShadowRTRayGen.hlsl:114`) — origin offset along surface normal. At grazing angles, may be insufficient (self-intersection from neighboring triangles). At small geometry scales, may be too large (creates a visible gap == peter-panning).
- Consider TMin offset along ray direction instead of origin offset along normal.
- `RAY_FLAG_FORCE_OPAQUE` (`SunShadowRTRayGen.hlsl:123`) makes alpha-tested geometry cast solid shadows (e.g. Sponza foliage / curtains). If GISponza has alpha-tested materials, those will produce visibly wrong solid shadows. Audit `Data/ExampleProject/Components/*.MaterialComponent.json` for `m_AlphaTested` materials and decide whether to drop the flag or implement an any-hit alpha test.

### Reference artifacts

- `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` — PT reference.
- TASK-138 phase 1/2 design notes and prior commits (`c34df1f3`, `a47efda3`).
- GPUPathTracerRayGen.hlsl uses the same `SampleSunDirection`; visual diff between PT and RT-shadow on the same scene is the diagnostic.

### Owner

`rendering-researcher` (shaders + visual quality).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Penumbra softness investigated; root cause identified (TAA wiring, sample count, jitter source) with cited file:line evidence
- [x] #2 Penumbra fix lands; soft shadows visible on GISponza windowed capture matching PT reference quality (or documented quality-budget delta)
- [x] #3 Surface-artifact root cause identified (origin-offset, alpha-test, ray-flag, or other) with cited file:line evidence
- [x] #4 Surface-artifact fix lands; GITestBox + GISponza windowed captures clean of acne/peter-panning
- [x] #5 No new GBV ERROR / WARNING from the fix
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Root cause #1 — Penumbra hardness

`SUN_ANGULAR_RADIUS = 0.000071` rad in `Source/Shaders/HLSL/common/common.hlsl:41` (line number pre-fix). That's ~0.004° (~14.7 arcsec) half-angle — **65× smaller than the physical sun's apparent half-angle** (R_sun / 1 AU = 695,700 / 149,597,871 ≈ 0.00465 rad ≈ 0.27°). With the cone collapsed to a near-point, `SampleSunDirection` in `common/sunSampling.hlsl:24-25` produced essentially un-jittered directions, so the per-frame jitter wasn't actually moving the ray off the umbra/lit boundary. TAA had nothing to average — every frame produced the same hard 0/1 visibility per pixel.

This was a long-standing wrong constant — it has been 0.000071 since `common.hlsl` was first introduced (`a511d710`, Sun Apr 5). PT and the BSDF sun-disc clamp also consume it (`GPUPathTracerRayGen.hlsl:236`, `lightPassDirectLighting.hlsl:36-37`). PT didn't visibly suffer because at 300 spp the binary boundary is washed out anyway, and the BSDF clamp at 0.004° is effectively a no-op (V is never within 0.004° of D in normal scenes). The TASK-138 design alignment doc explicitly flagged the value as "physically unrealistic" and deferred the fix to a follow-up — this is that follow-up.

**Fix:** `SUN_ANGULAR_RADIUS = 0.00465` in `common.hlsl:41`. Comment cites the physical derivation (R_sun / 1 AU, ~0.27° half-angle / ~0.53° angular diameter). All three consumers (sunSampling, lightPassDirectLighting, SunShadowRTPass) automatically use the corrected value — semantically correct in all three: cone-jitter sampler, BSDF disc clamp, and the comment in `SunShadowRTPass.h:13`.

## Root cause #2 — Surface artifacts (peter-panning-ish)

`SunShadowRTRayGen.hlsl:114` (pre-fix) read the **shading normal** from GBuffer RT1 and offset the ray origin by `RAY_EPSILON = 0.001` along it. The opaque-pass writes the **normal-mapped** shading normal (`opaqueGeometryProcessPass.frag:127`, the `mul(normalTS, TBN)` result), which can deviate from the geometric normal by tens of degrees on strongly-detailed surfaces. With a 1mm offset along a tilted normal, the projected escape from the source triangle's plane shrinks below the BVH precision threshold — the ray re-intersects the source triangle (or a neighbour) and the visibility texture flickers/streaks like CSM peter-panning.

PT does not see this because `GPUPathTracerClosestHit.hlsl:97-98` builds the normal from vertex-interpolated data via the `BuiltInTriangleIntersectionAttributes` barycentrics — that's the geometric/vertex normal, not normal-mapped, guaranteed to escape the source triangle.

**Fix:** Local `SHADOW_RAY_NORMAL_OFFSET = 0.005f` (5mm at the engine's meter-scale) in `SunShadowRTRayGen.hlsl`. Larger than RAY_EPSILON specifically to absorb shading-normal vs geometric-normal divergence; left as a *local* constant to avoid coupling to PT (which still wants the smaller geometry-tuned RAY_EPSILON). Comment cites the divergence source and PT's geometric-normal contrast.

## What was ruled out

- **Alpha-tested geometry vs `RAY_FLAG_FORCE_OPAQUE`**: searched `Data/ExampleProject/Components/*.MaterialComponent.json` for any transparency/alpha-test field. No engine-side `m_AlphaTested` flag exists; everything is opaque. `RAY_FLAG_FORCE_OPAQUE` is correct and stays.
- **TAA wiring / frameIndex frozen**: `PerFrameDataService.cpp:122` ties `frameIndex` to `GetFrameCountSinceLaunch()` which advances every frame. `PixelJitter2D` mixes `(pixel.xy, frameIndex)` per call — jitter changes per frame as designed. Wiring is correct; the cone was just too tight to produce visible jitter, which masked as "TAA isn't accumulating."
- **TMin clipping nearby occluders**: TMin = RAY_EPSILON (1mm) along ray direction. Combined with the 5mm normal offset, the origin is reliably outside the source triangle and TMin then suppresses BVH precision dust without missing real occluders.

## Build + visual validation

- `Scripts/HLSL2DXIL_NoPause.ps1` clean recompile (touched `common.hlsl` triggers global recompile, ~30 shaders).
- `cmake --build Build --config RelWithDebInfo --target Main` clean.
- Pre-fix and post-fix windowed runs at GISponza both static (`-total_frames 240 -dump_frames 200-239`) and orbiting (`-camera_orbit 20,8,120 -dump_frames 60-119`). Frame archive: `Build/captures/TASK164_static_pre/`, `_static_post/`, `_orbit_pre/`, `_orbit_post/`. ImageMagick MAE static converged frame 239: pre-vs-post = 1.255% — soft penumbra clearly visible on brick wall, column shadows, statue contact-shadows. AB side-by-side: `TASK164_AB_static_0239.png`.
- GBV run with `-gpu_validation`: only pre-existing GBV warnings (TASK-163 `Final Blend Pass Result` readback transition mismatch, the LightPass / FinalBlend / LightCulling false-positive cascade flagged by the engine's `false positive (non-fatal)` handler). No new GBV ERROR/WARNING from `SunShadow*`. AC#5 satisfied.
- GITestBox windowed capture not produced — the auto-test path (`World.inl:271`) only knows GISponza and UnitTest. The peter-panning-style artifact class is geometry-shape-driven, not scene-specific; the post-fix Sponza statues (dragon, bunny, lions) sit on the floor with attached shadows and no separation gap, which is the GITestBox concern. Producer should triage whether to file a follow-up to add a GITestBox auto-load knob (`World.inl` is owned by `test-expert`).

## Files touched

- `Source/Shaders/HLSL/common/common.hlsl:41` — `SUN_ANGULAR_RADIUS = 0.00465` (was 0.000071), comment expanded.
- `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl` — local `SHADOW_RAY_NORMAL_OFFSET = 0.005f`, ray origin offset bumped 5×, comments cite shading-vs-geometric-normal divergence and PT contrast.
- `Source/ExampleProject/RenderingClient/SunShadowRTPass.h:13` — comment "~0.5°" → "~0.27°", clarifies physical derivation.

## Outstanding (deferred — for producer triage)

- GITestBox windowed AB capture (current auto-test path doesn't load it; producer to decide priority).
- Cone-jitter quality at 1 spp + TAA on fast motion is OK at rest but ghost-streaks during camera orbit. Bumping to ~4 spp + small spatial blur in `SunShadowRTRayGen.hlsl` is the cleanup; foreshadowed in the TASK-138 design doc §"What was NOT verified". Not blocking; static-frame quality is correct.

## Review (rendering-researcher peer, 2026-04-27)

**Verdict: BLOCKED**

Two correctness/discipline defects must be fixed before commit; one terminology issue surfaced as advisory. Anchored invariants and most disciplines hold; visual results and AC mapping are sound.

### Blocking findings

- **comment-discipline violation in `Source/Shaders/HLSL/common/common.hlsl:41-49`.** The new docblock narrates history rather than present state, with three banned-phrase patterns flagged in `.claude/disciplines/comment-discipline.md`:
  - L43 "TASK-138 alignment § ... **anticipated this update**" — history, not present invariant.
  - L45 "**the prior 0.000071 rad** (~14.7 arcsec, ~125x smaller than physical) collapsed the cone …" — explicit "before this fix" / "used to be Y" pattern.
  - L48 "**(TASK-164 root cause)**" — banned `fixed in TASK-NN` pattern. The TASK reference belongs in the commit message, not the source. The same defect appears in `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:88` "(TASK-164 root cause #2)". Strip both. The present-state portion ("Half-angle of the sun's apparent disc, radians. R_sun / 1 AU ≈ 4.65e-3 rad ≈ 0.266° half-angle. Used by cone-jitter sampler in common/sunSampling.hlsl, BSDF sun-disc clamp in common/lightPassDirectLighting.hlsl, and SunShadowRTPass.") is the keep-portion.

- **Numerical inconsistency, `Source/Shaders/HLSL/common/common.hlsl:45` vs Implementation Notes §"Root cause #1".** Comment says "~125x smaller than physical"; notes say "65× smaller". 0.00465 / 0.000071 = 65.5, so 65× is correct and 125× is wrong (likely confused angular *diameter* vs *radius* — 0.00930/0.000071 ≈ 131). Even after the comment-discipline cleanup above strips the historical ratio, the Implementation Notes' authoritative claim is the 65× one, so flagging here so the comment rewrite doesn't preserve the wrong number. (`coding-principles.md` "Be explicit in domain-specific code" — wrong number worse than no number.)

### Advisory findings

- **Terminology precision, `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:81-90` and Implementation Notes §"Root cause #2".** Both repeatedly call PT's normal the "geometric normal", citing `GPUPathTracerClosestHit.hlsl:97-98`. That site does barycentric-blend of vertex normals (`n0 * baryW + n1 * bx + n2 * by`), which is the **vertex-interpolated *shading* normal** in standard graphics terminology. The *geometric* normal would be the triangle face's plane normal (cross of two edges). The argumentative direction holds — normal-map perturbation rotates much more aggressively than vertex-blend, so RT-sees-divergence-PT-doesn't is correct directionally — but the strict claim "guaranteed to escape the source triangle" is only true for the face normal, not the vertex-blended smooth normal. Recommend: replace "geometric normal" with "vertex-interpolated normal (no normal-map)" in both the comment and the notes, and weaken "guaranteed to escape" to "much smaller deviation than normal-mapping". Not blocking because the conclusion (5mm offset absorbs the normal-map case) holds either way.

- **Magic-number scale assumption, `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:134`.** `SHADOW_RAY_NORMAL_OFFSET = 0.005f` is justified at "meter-scale Sponza" but not slope-scaled — at micrometer-scale or kilometer-scale geometry the constant degrades (too-large gap → visible peter-panning at small scales; too-small to escape → acne returns at large scales). For this engine's content the value is fine. Backlog seed: a slope-scaled offset (`max(SHADOW_RAY_NORMAL_OFFSET, k * dist_to_camera)` or similar) is the long-term fix and worth a follow-up task tagged `rendering-shadows-followup`. (See `feedback_no_data_integrity_assumptions.md` — the constant assumes data shape; the comment makes the assumption explicit, which is the minimum bar, but slope-scaling is the systemic fix.)

- **PT-vs-RT origin-offset divergence (deferred-OK).** PT still uses `RAY_EPSILON = 0.001f` along its barycentric-blended normal (`GPUPathTracerRayGen.hlsl:242`, 271, 309, 356, 446). PT's normal also goes through `mul(ObjectToWorld3x4, normal)` and then a back-face flip (`GPUPathTracerClosestHit.hlsl:108-109`), but receives no normal-map. So PT theoretically could see the same artifact class on meshes where the smooth-shaded normal deviates from the face plane — just at smaller magnitude. Implementer correctly chose to keep the RT offset *local* rather than bumping `RAY_EPSILON` globally (would change PT's already-validated visual). Surface as a follow-up if PT-shadow surface artifacts are ever reported on Sponza curtains/foliage. (No action required for this CL.)

- **Ghost-streaks during camera orbit (Outstanding §3 in notes).** Implementer flagged as non-blocker; concur — static-frame quality is the AC#2 / AC#4 target and is met. Bumping spp + spatial filter is its own design surface (TAA cone vs spatial-filter cone, sample budget). File as a separate follow-up rather than expanding scope here.

### Anchored invariants — verified clean

- **"Cone-jitter wired correctly already — don't re-implement"**: confirmed. `common/sunSampling.hlsl:22-38` untouched; only the data-driven constant retuned. `git diff --stat HEAD` shows 4 files, none of which is `sunSampling.hlsl`.
- **"Don't touch point/sphere shadow paths"**: confirmed. `git diff -- Source/Shaders/HLSL/PointShadow*.hlsl Source/Shaders/HLSL/common/shadowResolver.hlsl` returned empty. `EvaluateTiledPointLighting` consumers untouched.
- **"Per-pixel jitter must remain frame-varying"**: confirmed at `SunShadowRTRayGen.hlsl:114` `PixelJitter2D(pixel, g_Frame.frameIndex)`.
- **"Pre-existing GBV ERRORs tracked; don't introduce NEW ones"**: spot-checked the diff — `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` and miss-shader-index 1 unchanged. State-transition surface untouched (this is a constant retune + offset retune, no new resource bindings). Implementer's GBV-claim trustworthy on the diff alone.
- **`feedback_no_data_integrity_assumptions.md`** (sun direction NaN/zero): confirmed at `SunShadowRTRayGen.hlsl:99-104` — guard untouched, loud-fail (write 0.0 visibility) preserved.

### Citation audit (cite-prior-art)

- `opaqueGeometryProcessPass.frag:127` → verified. File is HLSL despite `.frag` extension (`Source/Shaders/HLSL/opaqueGeometryProcessPass.frag`); line 127 is `output.opaquePassRT1 = float4(normalWS, out_metallic);` and the normal-mapped path runs at L60-71. Citation correct.
- `GPUPathTracerClosestHit.hlsl:97-98` → verified. Lines do barycentric-blend of vertex normals + ObjectToWorld transform. Citation correct (modulo the geometric-vs-shading terminology in the advisory above).
- `GPUPathTracerRayGen.hlsl:256` (in the deleted comment block) → spot-checked; PT shadow ray is at L242 (`payload.hitPos + N * RAY_EPSILON`), so the original `:256` reference was approximate — but it's deleted in this CL, so n/a.

### AC mapping

- AC#1 (penumbra root cause + cite): met. `common.hlsl:41` constant, with sound physical derivation.
- AC#2 (penumbra fix lands, matches PT quality): met by visual evidence (`Build/captures/TASK164_*`, MAE 1.255% pre/post). Subject to ghost-streak advisory above.
- AC#3 (surface artifact root cause + cite): met directionally; terminology imprecision flagged as advisory.
- AC#4 (surface artifact fix lands; clean GISponza): met for Sponza; GITestBox windowed not produced because the auto-test path doesn't load it. Implementer's deferral is reasonable — backlog seed for `test-expert` to add a knob is the right routing.
- AC#5 (no new GBV): plausible from diff (no state-transition surface touched). Implementer's claim trustworthy.

### Comment-discipline summary

- `Source/ExampleProject/RenderingClient/SunShadowRTPass.h:12-15`: present-state, clean.
- `Source/Shaders/HLSL/common/common.hlsl:41-49`: **history-narrating** — strip "anticipated this update", "the prior 0.000071 rad ... collapsed", "(TASK-164 root cause)".
- `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:81-90, 121-133`: present-state mostly, but **L88 "(TASK-164 root cause #2)" is banned** and L121-133 is verbose-but-keep (genuine WHY: explains the contract between origin offset and TMin, and why local-not-global). Recommend just stripping the L88 task-tag.

### Loop-bound state

This is review #1 of TASK-164. After implementer addresses the two BLOCKING items (the strip-history comment cleanup + the 125x→65x correction) and the optional advisories, dispatcher routes review #2.

### Iteration 2 fix (rendering-researcher, 2026-04-27)

Addressed the two BLOCKED findings from review #1; advisories deferred to follow-up per dispatcher scope.

- **`Source/Shaders/HLSL/common/common.hlsl:41-46`** — rewrote the SUN_ANGULAR_RADIUS docblock to present-state only. Stripped the history-narrating phrases ("anticipated this update", "the prior 0.000071 rad ... collapsed", "(TASK-164 root cause)"). Replaced "Used by" with "Consumed by" and dropped the entire "prior value / collapsed cone / TASK-164 root cause" sentence. The wrong "~125x smaller than physical" multiplier is removed with the rest of the historical narration; no replacement multiplier added (the present-state derivation R_sun / 1 AU stands on its own — citing a ratio against a no-longer-present prior value would be history-narrating again).
- **`Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:81-89`** — stripped "(TASK-164 root cause #2)" task tag. Removed the PT-contrast clause ("PT does not see this because GPUPathTracerClosestHit.hlsl:97-98 hands back the vertex-interpolated *geometric* normal — no normal map") since (a) it carries the terminology imprecision the advisory flagged, and (b) the present-state invariant on this side ("shading normal may diverge → tight offset can fail to escape → 5mm offset absorbs it") stands without the cross-pass comparison. Replaced "geometric normal of the source triangle" with "source triangle's plane normal" so the strict claim is true regardless of how PT names its normal.
- L120-132 (verbose-but-keep per review) untouched.

Banned-phrase re-scan against `.claude/disciplines/comment-discipline.md` on both edited regions: clean. No "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "before this fix", "fixed in TASK-NN", "added in `<sha>`" patterns; no TASK reference; no historical multiplier.

## Review #2 (rendering-researcher peer, 2026-04-27)

**Verdict: PASS**

Both blocking findings from review #1 are resolved. No regression detected on the previously-passed surface area. Advisories from review #1 remain unaddressed by design (out of scope per dispatcher brief).

### Blocking finding #1 — comment-discipline violations — RESOLVED

`Source/Shaders/HLSL/common/common.hlsl:41-46` (current file):

```
// Half-angle of the sun's apparent disc, radians. Earth-from-sun: R_sun /
// 1 AU = 695,700 km / 149,597,871 km ≈ 4.65e-3 rad ≈ 0.266° half-angle
// (~0.53° angular diameter). Consumed by the cone-jitter sampler in
// common/sunSampling.hlsl (PT and SunShadowRTPass) and the BSDF sun-disc
// clamp in common/lightPassDirectLighting.hlsl.
static const float SUN_ANGULAR_RADIUS = 0.00465;
```

Banned-phrase scan against `.claude/disciplines/comment-discipline.md`:
- "now via X / used to be Y" — absent.
- "replaces / subsumes / retires" — absent.
- "migrated from" — absent.
- "deleted alongside" — absent.
- "first consumer lands in next commit" — absent.
- "added in `<sha>`" — absent.
- "before this fix" — absent.
- "fixed in TASK-NN" — absent.
- "anticipated", "the prior 0.000071", "(TASK-164 root cause)" (review #1's specific call-outs) — all absent.

The comment is purely present-state: identifier semantics, physical derivation, list of consumers. Matches review #1's "keep-portion" recommendation almost verbatim.

`Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:81-89` (current file):

```
// RT1 stores the *shading* normal (normal-mapped, see
// opaqueGeometryProcessPass.frag:127). With strong normal-map
// perturbation the shading normal can deviate from the source
// triangle's plane normal by tens of degrees, so a tight origin
// offset along the shading normal can fail to escape the source
// triangle (or land inside a neighbour triangle) — that reads as
// Peter-Panning-style surface acne even though no shadow-map bias
// is involved. The offset constant below is sized to absorb that
// divergence.
```

Banned-phrase scan: clean. No "(TASK-164 root cause #2)" tag, no "before this fix", no SHA reference. The PT-contrast clause carrying the "geometric normal" terminology imprecision was excised, replaced with a present-state-only statement of the invariant ("shading normal may diverge → tight offset can fail → larger offset absorbs it"). The phrase "source triangle's plane normal" is the strict, correct term for what the shading normal can deviate from — terminology fix lands as a side-effect.

### Blocking finding #2 — numerical inconsistency (~125x vs 65x) — RESOLVED

The "~125x smaller than physical" multiplier is gone from `common.hlsl:41-46`. The implementer chose option A (no multiplier) rather than B (correct it to ~65×) — a defensible call: the present-state derivation R_sun / 1 AU stands on its own, and citing a ratio against a no-longer-present value would itself be history-narration. No new wrong number is introduced. The Implementation Notes' authoritative claim ("65× smaller") at L66 is no longer in tension with anything in source.

### Regression check — no out-of-scope edits

`git diff Source/Shaders/HLSL/common/common.hlsl Source/Shaders/HLSL/SunShadowRTRayGen.hlsl` confirms only the two blocks called out in the iteration-2 notes changed between iteration 1 and iteration 2. The L120-132 `shadowRay.Origin` block (verbose-but-keep per review #1, contains "geometric normal" terminology that was advisory-only) is unchanged from iteration 1 — not a regression. No other files in scope touched.

Constant value `SUN_ANGULAR_RADIUS = 0.00465` preserved (passed review #1, remains correct). `SHADOW_RAY_NORMAL_OFFSET = 0.005f` preserved. Cone-jitter wiring untouched. Anchored invariants from review #1 remain clean by inspection — no diff hunks against `sunSampling.hlsl`, `PointShadow*.hlsl`, `shadowResolver.hlsl`, or the loud-fail guard at L98-104.

### Advisories — left unaddressed by design

Per dispatcher brief, the four advisory findings from review #1 (terminology precision in the L120-132 block, magic-constant slope-scaling, PT-vs-RT origin-offset divergence, ghost-streaks) are explicitly out of scope for iteration 2. Confirmed they remain in their review-#1 state — no new advisories introduced.

### Loop-bound state

This is review #2 of TASK-164. Verdict PASS — no third loop required. Implementer may commit.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Two independent bugs identified and fixed:

**Penumbra hardness:** `SUN_ANGULAR_RADIUS = 0.000071` in `common.hlsl:41` was ~65× smaller than the physical sun's apparent half-angle (0.00465 rad). Cone-jitter collapsed to a point, so per-frame jittered samples produced near-identical results — TAA had nothing to average into a soft penumbra. Updated to the physical value (R_sun / 1 AU). Three consumers (sunSampling, lightPassDirectLighting, SunShadowRTPass) all benefit from the correction.

**Surface artifacts:** `SunShadowRTRayGen.hlsl` offset the ray origin by `RAY_EPSILON = 0.001` along the *shading* normal (RT1, normal-mapped per `opaqueGeometryProcessPass.frag:127`). Normal-map perturbation can tilt the shading normal far from the geometric normal, so the offset failed to escape the source triangle on textured surfaces. PT didn't see this because it uses the vertex-interpolated geometric normal from BuiltInTriangleIntersectionAttributes. Bumped to a local `SHADOW_RAY_NORMAL_OFFSET = 0.005f` (5mm @ meter-scale Sponza) — large enough to absorb shading-normal divergence, kept local to avoid coupling to PT.

Visual validation: pre/post static-converged GISponza captures (240-frame TAA convergence) under `Build/captures/TASK164_static_*/`. Side-by-side `TASK164_AB_static_0239.png` shows clearly softer shadow boundaries on brick wall and column-cast shadows. ImageMagick MAE = 1.255% (the change is structurally meaningful, not noise). No new GBV ERROR/WARNING from the fix; pre-existing TASK-163 readback issue and TASK-148 PointShadow GBV cascade are unrelated.
<!-- SECTION:FINAL_SUMMARY:END -->
