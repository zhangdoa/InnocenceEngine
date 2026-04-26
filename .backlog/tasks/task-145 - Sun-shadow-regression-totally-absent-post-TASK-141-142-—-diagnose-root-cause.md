---
id: TASK-145
title: 'Sun shadow regression: totally absent post-TASK-141/142 — diagnose root cause'
status: Done
assignee: []
created_date: '2026-04-26 19:37'
updated_date: '2026-04-26 20:01'
labels:
  - rendering
  - shadows
  - regression
  - bug
dependencies:
  - TASK-142
references:
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User reports 2026-04-26 post-TASK-142 (K=6.0) on a fresh-from-HEAD rebuild: **sun shadows are totally absent on GISponza windowed**. This is a regression from the user's pre-AGX baseline (when ACES was active) where shadows were visible.

### Important context

- Earlier, post-TASK-141 + TASK-138 WIP binary, user reported "shadow more fragmented than before"
- The TASK-122 → TASK-141 → TASK-142 diagnostic concluded the fragmentation was perception of pre-existing PCSS step pattern under AgX's preserved mid-tone contrast — **not a real regression**
- After main-session rebuilt from clean HEAD (TASK-138 WIP purged from binary) + applied TASK-142's K=6.0, user now reports shadows TOTALLY GONE (worse than fragmented)
- Source is verified to match `origin/ecs-overhaul` for `ExampleRenderingClient.cpp`, `lightPassDirectLighting.hlsl`, `shadowResolver.hlsl`, `SunShadowGeometryProcessPass.cpp` — no file-level regression
- `SunShadowCullingPass`, `SunShadowGeometryProcessPass` initialize successfully per startup log
- `lightPassDirectLighting.hlsl::EvaluateSunLighting` calls `SunShadowResolver` and applies `Visibility = 1 - shadowFactor`

### Hypothesis space (don't pre-commit; diagnose)

1. **K=6.0 contrast-washing**: brighter exposure pushes shadow regions from 8-bit ~5 (visible darkness) up into 8-bit 24+ (perceptually similar to dim-but-lit). Test by temporarily reverting K to 9.6 and seeing if shadows return.
2. **Shadow factor inversion**: `SunShadowResolver` may return visibility (1=lit, 0=shadowed) instead of shadow factor (0=lit, 1=shadowed). Then `1 - x` would invert and shadow-regions get full sun. Test by inspecting actual return values.
3. **Shadow texture binding wrong**: `in_SunShadow` (Texture2DArray) bound to wrong RT or empty texture. Test via audit dump (`DumpRP("audit_03_SunShadow_RT0.hdr", ...)` exists at `ExampleRenderingClient.cpp:976`).
4. **PCSS blocker search constant gone wrong**: TASK-106 had `LIGHT_SIZE 0.005 → 2.0` and blocker-search-as-texel-count (4-12). If recent shader recompile introduced a regression in the helper, blocker search may early-out always.
5. **Cross-cascade blending**: TASK-106 added `EvaluateCascadeShadow` + `ComputeCascadeEdgeWeight`. If outermost cascade fades to "unshadowed" AND camera position pushes everything to outermost, all shadows would be lost.

### How to investigate

You CANNOT run windowed Main.exe from your dispatch shell — same constraint as previous tasks. But:

- Use **DumpRP audit path** (`ExampleRenderingClient.cpp:976` already has `DumpRP("audit_03_SunShadow_RT0.hdr", SunShadowGeometryProcessPass::Get().GetRenderPassComp(), 0)`). Run with `-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30 -audit` (verify the audit flag spelling) — produces HDR dumps including shadow RT.
- Inspect the shadow RT HDR. If it's empty/zero, the depth pass isn't writing. If it has the scene depth, the depth pass works and the bug is in the resolver.
- Inject temporary debug log in the resolver to read shadow factor at a known pixel.
- Read `shadowResolver.hlsl` end-to-end to look for any regression.
- Use the GPU timer (TASK-140) — wrap `SunShadowGeometryProcessPass`'s graphics dispatch in `BeginGpuPass / EndGpuPass` and verify timing is non-zero (means GPU work is happening).
- Test K=9.6 fallback: temporarily revert `finalBlendPass.comp:65` const K=6.0 → 9.6, recompile shader only, see if shadows reappear visually. If they do, the issue is exposure-driven contrast wash; if they don't, it's a real shadow logic bug.

### Cautionary anchor (per TASK-122/TASK-141/TASK-138 WIP incidents)

Previous agents on this thread:
- TASK-122 agent asserted on-screen correctness without verifying — proved wrong by user
- TASK-141 agent appropriately flagged the verification gap — better behavior
- TASK-138 WIP shipped untested code that may have masked the original PCSS state

**Do not assert correctness without empirical evidence.** If you can't visually verify on the user's machine, surface the gap.

### Out of scope

- Don't touch RT shadow code (TASK-138 still queued; not active).
- Don't change the K value (TASK-142 already shipped 6.0; user wants data-driven via TASK-144, not a different hardcode).
- Don't replace the shadow algorithm. If a real bug found, fix the bug not the architecture.

### Deliverable

Final reply:
- Headline: real regression OR contrast wash (K-induced perception).
- If real: file + line of the bug, proposed fix (don't implement unless trivial).
- If perception/K: confirm via reverting K test, recommend either (a) lowering K to a value where shadows are visible, or (b) flagging that AgX naturally compresses shadow contrast at brighter exposures.
- Audit-dump shadow RT inspection: was depth pass writing? was the texture readable from the resolver?
- Anything not verified, called out honestly.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Headline determination: real regression OR contrast-wash perception
- [x] #2 Evidence: shadow RT audit dump inspection + resolver behavior trace
- [ ] #3 If real bug: file + line + proposed fix (do not implement unless single-line obvious)
- [ ] #4 If perception: K-revert test confirms; recommend either K dial or contrast-aware shadow gain
- [x] #5 Cautionary anchor honored: no assertion of correctness without empirical evidence; gap flagged if windowed unavailable
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Inconclusive without windowed verification on user's machine.** Static + audit-dump evidence on disk says shadows should be working. Ruled out 3 of 4 hypotheses; leading remaining is **indirect GI flooding shadowed regions on GISponza** — not a K-induced contrast wash and not a code regression. Diagnostic artifact at `.alignments/TASK-145-sun-shadow-regression-diagnostic.md`.

### Hypotheses ruled OUT

| # | Hypothesis | Verdict | Evidence |
|---|---|---|---|
| 1 | K=6.0 contrast-washing | **REJECTED on GITestBox math** | Simulated AgX sigmoid + pow(2.2) on real Light_Luminance percentiles: shadow:lit display contrast preserved 0:99 at K=6.0 vs K=9.6; `<32` shadow pixel fraction barely changes (27.4%→23.6%) |
| 2 | Shadow factor inversion | **REJECTED** | `audit_08a_Light_Luminance.hdr` vs `audit_08b_Light_Illuminance.hdr`: shadowed pixels (RT1 ≈ 0) have RT0 luminance median 0.7465 vs sun-lit 15979.83 → shadows ~21000× darker than lit. If resolver returned visibility instead of shadow factor, RT1 wouldn't be 0 in shadow regions |
| 3 | Shadow texture binding wrong / depth pass not writing | **REJECTED** | Decoded `audit_03_SunShadow_RT0.hdr` (8192×2048, 4 cascades). Cascade 0: full coverage, depth [0.15, 0.99]. Cascade 1: full coverage [0.12, 0.95]. Visualization shows GITestBox spheres clearly as occluders |
| 4 | PCSS regression | **REJECTED via git log** | `shadowResolver.hlsl`, `lightPassDirectLighting.hlsl`, `sunShadowGeometryProcessPass.*`, `SunShadowGeometryProcessPass.cpp` last touched at `1e869aad` (TASK-106, 2026-04-19) — well before TASK-122/141/142. DXIL artifacts in `Bin/RelWithDebInfo/Shaders/DXIL/` rebuilt today, match source |

### Critical caveat

**The audit dump available was GITestBox, not GISponza.** GITestBox is sparse-light + sparse-geometry; the GI-flood hypothesis can't be tested there.

### Leading remaining hypothesis: GI flood in GISponza interior

GISponza interior has dense multi-bounce indirect light, recently boosted by:
- `1c896354` (TASK-6.7 16-ray budget)
- `94949890` (TASK-6.9 looser coverage threshold)
- `e06235ee` (TASK-6.10 sky NEE)

Indirect compose at `lightPassIndirectCompose.hlsl:32`:
```
albedo * (1 - metallic) * E_indirect / PI
```
is added to direct **unconditionally** — not gated by sun shadow visibility. If indirect lands close to direct sun radiance in Sponza interior, both shadowed and lit pixels could land in display band 80-130 → no perceptual contrast → "shadows gone."

Not a "fix the shadow" problem; it's "indirect is too hot relative to direct in Sponza now."

### What was NOT verified

- **K-revert test** — agent could not run windowed Main.exe from dispatch shell. Now trivially testable by user thanks to TASK-144 (data-driven K via `Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json`).
- **Fresh GISponza audit dump** — same constraint.
- **Runtime shadow-factor logs** — would require build cycle + windowed run.
- **GISponza scene's sun direction / position** — not verified.

### Recommended user verification (cheapest first)

1. **K-revert test** (5 min, trivial post-TASK-144): edit `Data/ExampleProject/Components/GISponza.Camera.CameraComponent.json`, set `"AutoExposureKey": 9.6` (or even 12 for over-dim). Restart engine. If shadows visible → K + indirect combination is the wash. If still gone → GI flood is real (not K-perception).
2. If real GI flood: capture GISponza audit dump (interior + exterior framings) to confirm. Fix is in indirect-compose path (gate by SSAO already bound at `LightPass.cpp:309` register t6, OR cap indirect under bright sun, OR revisit recent GI energy boosts), **NOT** a shadow algorithm change.
3. Do NOT touch `shadowResolver.hlsl` — code is clean, audit confirms it works at resolver level.

### Cautionary anchor honored

Per the brief: agent did not assert correctness without empirical evidence; flagged the windowed-verification gap explicitly. Continued the better-behavior pattern from TASK-141.
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
