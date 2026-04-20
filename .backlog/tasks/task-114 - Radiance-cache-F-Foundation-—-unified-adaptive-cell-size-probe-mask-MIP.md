---
id: TASK-114
title: 'Radiance cache [F] Foundation — unified adaptive cell size + probe mask MIP'
status: To Do
assignee: []
created_date: '2026-04-20 17:35'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add unified `adaptive_cell_size(depth)` HLSL helper per GI-1.0 Algorithm 6 and use it consistently across reprojection, sampling, and filter (currently filter uses its own formula; reprojection uses a hardcoded MAX_PLANE_DISTANCE=0.5).

Add a new `RadianceCacheProbeMaskPass` (compute) that writes a 1-texel-per-tile sentinel texture — the sub-tile pixel coords of the spawned probe packed into a 32-bit integer, or INVALID for empty tiles. Generate its MIP chain where each level keeps the first valid probe found in the 2×2 upper level (paper's `probe_mask` / Figure 8).

Implement `find_closest_probe(pixel, offset)` (Algorithm 4) that walks the mask MIP instead of reading position textures. Replace every `in_ProbePosition.w > 0` check with this call.

This sub-project is the foundation for [S1], [S2], [I] — every filter/search routine downstream reads the mask MIP.

Scope does NOT include: sparse spawning (that's [S1]), filter rewrite (that's [S2]), light-pass changes (that's [I]).

Deliverables:
- Unified cell size helper in a shared HLSL header
- New pass: ProbeMask (writes sentinel texture + MIP chain)
- `find_closest_probe` helper in HLSL
- Reprojection and filter updated to consume both
- Re-enabled passes (lift the TASK-60 disablement)
- RenderDoc baseline + post-[F] capture at frame 8 GISponza

Doc: Documents/radiance-cache-roadmap.md §[F]</description>
<parameter name="labels">["rendering", "GI", "TASK-6"]
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
