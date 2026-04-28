---
id: TASK-191
title: >-
  Sponza: stray objects rendered over building/pillars not present in Intel
  reference
status: To Do
assignee:
  - rendering-researcher
created_date: '2026-04-28 19:25'
labels:
  - bug
  - rendering
  - sponza
  - triage
dependencies: []
references:
  - >-
    https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Observation

In the Sponza scene, geometry appears rendered *over* the building structure and pillars that is absent from Intel's published Sponza screenshots. Cause is not yet diagnosed — could be either:

- **Rendering bug** (e.g. depth/sort/transparency issue, stale draw, debug overlay, incorrect layer)
- **Asset conversion bug** (e.g. mesh transform / unit scale / parent hierarchy lost, extra geometry imported that Intel's distribution doesn't include, materials/visibility flags dropped)

## Reference

- Intel Sponza reference: https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html (Sponza section)

## First steps for the owner

1. Capture a screenshot of our Sponza render from the same/comparable camera angle as the Intel reference.
2. Side-by-side compare; identify which objects are extra and where they sit in space.
3. Determine layer of failure:
   - If the extra objects are also visible in our scene's ECS dump / asset inspector when no rendering happens (e.g. via picker / list-entities), it's an asset-conversion / scene-build issue → reassign to software-architect (asset pipeline).
   - If the extra objects only appear in the rendered output but not in the ECS / asset listing, it's a rendering issue (likely a stale framebuffer, debug pass, or incorrect render-pass output) → stays with rendering-researcher.
4. File a fix task (or split this one) once cause is localized.

## Notes

- Diagnostic tooling that would help: TASK-183 (runtime visualization modes / debug output picker) and TASK-185 (RT picker). If those land first, this triage is much cheaper.
- Related: TASK-189 (yellow-green Sponza tint) is also a Sponza visual-difference observation; both are blocked on better diagnostic tooling but not on each other.
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
