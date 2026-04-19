---
id: TASK-98
title: 'R&D: cloth / fabric rendering (Sponza curtains as reference asset)'
status: To Do
assignee: []
created_date: '2026-04-19 18:11'
labels:
  - rendering
  - brdf
  - research
  - sponza
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Sponza ships with curtains that should read as fabric — two-sided, slightly translucent, with sheen and directional weave. The current BRDF treats them as opaque diffuse, which is visually wrong. Stand up a fabric/cloth shading path and wire it through the material system so curtains (and any future cloth asset) look right.

## Sub-scope

- **BRDF** — add a cloth/fabric term alongside the existing GGX. Common options: Ashikhmin (charlie) cloth sheen, or a fuzz/sheen lobe with inverted Gaussian NDF. Pick the one that fits radiance-cache + path tracer both; document the choice.
- **Two-sided shading** — curtains are single-sided geometry that must light on both sides. Flip normal at shading time when viewing back-face.
- **Material authoring** — add `MaterialType::Cloth` (or `MaterialFlags::Cloth`) and a `sheenColor` / `sheenRoughness` parameter pair. Pick sensible defaults if the asset has no explicit sheen map.
- **Asset wire-up** — Sponza's curtain mesh(es) get the new material tag. Confirm with a RenderDoc capture before/after.
- **Path tracer** — sample the sheen lobe in the GPU path tracer's MIS so convergence remains correct.

## Why

Sponza is the primary visual benchmark. Fabric is a common enough production material that the engine can't credibly skip it. Also unblocks future cloth physics / animation work.

## Deliverables

- RenderDoc before/after on GISponza frame 8.
- Works in both rasterized and path-traced modes.
- Doesn't regress non-cloth materials.
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
