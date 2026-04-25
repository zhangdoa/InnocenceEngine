---
id: TASK-6.3
title: >-
  LightPass world-cache fallback when 4-corner ring-search exhausts (replace
  AMBIENT_FLOOR constant)
status: To Do
assignee: []
created_date: '2026-04-25 19:45'
labels:
  - rendering
  - GI
  - radiance-cache
  - lightpass
dependencies:
  - TASK-125
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/GIDenoise.comp
  - Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Scoped successor to TASK-123 (closed-with-residue 2026-04-25). TASK-123 diagnosed the right cause for the GITestBox black-corner symptom — pixels where the 4-corner bilinear interpolation fails the ring-search + edge-aware weight test fall through to a relaxed fallback that reads from never-spawned probe tiles and gets zero — but shipped an unconditional scene-tinted constant `AMBIENT_FLOOR = (0.02, 0.025, 0.03)` in `lightPass.comp` instead of a structural fallback. This task replaces the constant with the structural fix.

## Acceptance criteria

- LightPass detects 4-corner ring-search exhaustion (i.e. all 4 bilinear corners failed the validity + edge-aware weight test, the path that today triggers the relaxed/`AMBIENT_FLOOR` branch).
- On exhaustion, sample the world cache (`in_WorldTileGrid`) at the pixel's world position with `(-viewDir, isShortRay)` as descriptor inputs — the same pattern `RadianceCacheClosestHit.hlsl` already uses for off-screen fallback. Use the result as the irradiance estimate for that pixel.
- Remove the `AMBIENT_FLOOR` constant and the `max(l_IrradianceFromCache, AMBIENT_FLOOR)` clamp from `lightPass.comp`.
- Verify on both GISponza and GITestBox that the previously-black regions render plausibly (per the path-tracer reference), without re-introducing the original symptom.
- Verify on at least one warm-interior scene (where the cool-blue AMBIENT_FLOOR looked wrong) that the new fallback adapts to scene tone instead of layering a fixed cool tint.

## Why this is its own task

TASK-123's Final Summary documents what shipped (the constant). Re-opening TASK-123 would conflate "what shipped at the time" with "what we eventually wanted" and lose the diagnostic record. This task carries only the corrective scope; TASK-123 stays Done with the closure-residue annotation pointing here.

## Dependencies / sequencing

- Must land after TASK-125 CL3 (the §2.4.3 paper-faithful denoiser) is stable. The denoiser swap may have changed how disocclusion / under-sampled pixels are blurred — the original symptom may now manifest differently (or be partially absorbed by the new spatial filter), so re-validate the symptom on current HEAD before assuming the fix is still needed at the same severity.
- World-cache descriptor proxy: at the LightPass call site we have viewDir (= -ray dir for primary visibility) and pixel world position. `RadianceCacheClosestHit.hlsl` already shows the working pattern for diffuse-bounce world-cache reads using a direction proxy; copy that pattern rather than inventing a new descriptor convention.
- If the denoiser swap reduced the symptom severity below "visibly wrong on every scene", reduce this task's priority and document the re-validation result in the closure notes; do not delete the AMBIENT_FLOOR removal step (the constant is structurally wrong even if its absence is invisible).
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
