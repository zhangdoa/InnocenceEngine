---
id: TASK-6.6
title: >-
  GISponza dimness post-LIGHTPASS_AMBIENT_FLOOR removal — verify against
  path-tracer reference; evaluate paper-faithful brightness paths
status: To Do
assignee: []
created_date: '2026-04-26 11:19'
labels:
  - rendering
  - GI
  - paper-faithful
  - validation
dependencies:
  - TASK-6.5
references:
  - Build/captures/TASK6_5_pre_sponza/
  - Build/captures/TASK6_5_post_sponza/
  - .alignments/TASK-6.3-lightpass-fallback.md
  - Source/Shaders/HLSL/RayTracing/GPUPathTracerRayGen.hlsl
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from TASK-6.5 closure (2026-04-26). Removing `LIGHTPASS_AMBIENT_FLOOR = (0.02, 0.025, 0.03)` exposed that on dim indirect-only scenes like GISponza, the constant was acting as a frame-wide ambient base — not just a fallback for relaxed pixels. Whole-frame mean intensity drops ~16% (post 104 vs pre 125 in tonemapped 0..255 space). The diagnostic from TASK-6.5 proves the dimmed pixels weren't relaxed — they were just dim because single-bounce SH-projected GI is inherently dim where light doesn't reach.

This is paper-faithful exposed reality, not a regression. GI-1.0 is single-bounce by paper design (§2.1, §2.4). The proper response is paper-faithful brightness, not reintroducing a constant.

### Two questions to answer

1. **Is GISponza supposed to be this dim under single-bounce GI?** Verify by capturing the CPU path-tracer reference (`RayTracer::Execute()` auto-fired at terminate per `World.inl:354`) under the same orbit camera and frame range. Compare against `Build/captures/TASK6_5_post_sponza/`. If the path tracer also produces this dimness, the answer is "yes, that's what single-bounce GI looks like" and any further brightness work goes into multi-bounce / sky-irradiance / volumetric ambient (TASK-99 territory). If the path tracer shows brighter ambient, the GI implementation is missing energy somewhere upstream of the fix.

2. **If single-bounce really is this dim, what's the paper-faithful path to acceptable brightness?**
   - **Multi-bounce GI / world-cache integration (paper §2.2).** The world cache feeds secondary path vertices; if screen-probe rays bounce off geometry and read the world cache for further-bounce lighting, indirect-indirect contribution lifts the dim regions without a constant.
   - **Sky / environment irradiance.** A baseline sky-cubemap convolution into low-frequency probes catches "no GI ray hit anything but missed to sky" — paper-prescribed and currently missing.
   - **More aggressive variable-radius blur on relaxed pixels.** The TASK-6.5 fix wakes up the blur-mask widening but the radius cap may still be too tight; the auditor noted *"if the widened bilateral blur produces an unsatisfactory result, the answer is paper-faithful (more aggressive blur)."*
   - **Inspect Capsaicin's GISponza output for direct comparison.** Capsaicin runs the same scene; if their output is brighter at single-bounce, our SH projection / ray budget / world-cache integration is missing something.

### Acceptance

- Path-tracer ground-truth captures of GISponza at the same orbit positions used in TASK-6.5's TASK6_5_post_sponza captures.
- Direct PT-vs-rasterized-GI comparison at e.g. frames 60, 80, 100, 119.
- Determination: is GISponza-POST dimness paper-faithful (matches PT) or under-energy (PT brighter)?
- If under-energy: file specific tasks for the missing paper-prescribed paths (multi-bounce / sky / wider blur), don't bundle into one work item.
- If paper-faithful: file appropriate brightness work (multi-bounce GI / sky-irradiance) as separate scoped tasks; close THIS task as "verified".

### Why medium priority

GISponza is the showcase scene; visible dimming matters for "looks playable". But the fix that exposed it (TASK-6.5) is paper-correct and shipped, so this isn't a regression rolled-back-able into the constant. The right next move is paper-faithful brightness work, not a hot-fix.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Path-tracer reference captures of GISponza at TASK6_5_post_sponza orbit positions (frames 60/80/100/119 minimum)
- [ ] #2 Direct PT-vs-rasterized-GI comparison documented with captures + diff
- [ ] #3 Determination: paper-faithful (matches PT) or under-energy (PT brighter)? Quoted in summary
- [ ] #4 If under-energy: specific follow-up tasks filed per missing paper-prescribed path (multi-bounce / sky-irradiance / wider blur)
- [ ] #5 If paper-faithful: brightness work (multi-bounce / sky-irradiance) filed as separate scoped tasks; this task closes as verified
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
