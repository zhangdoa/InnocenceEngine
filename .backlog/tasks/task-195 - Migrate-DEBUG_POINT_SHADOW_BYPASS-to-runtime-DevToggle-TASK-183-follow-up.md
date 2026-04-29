---
id: TASK-195
title: Migrate DEBUG_POINT_SHADOW_BYPASS to runtime DevToggle (TASK-183 follow-up)
status: To Do
assignee: []
created_date: '2026-04-28 19:41'
labels:
  - rendering
  - tooling
  - diagnostic
dependencies:
  - TASK-183
references:
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Engine/Services/DevToggleRegistry.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Follow-up to **TASK-183** (engine-side runtime visualization modes shipped). The TASK-148 A/B toggle `DEBUG_POINT_SHADOW_BYPASS` (in `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl:130`) is still `#define`-gated; it forces per-light inline-RT visibility to 1 to A/B against unshadowed direct lighting.

TASK-183 covered visualization picker modes (one-of-N enum). `DEBUG_POINT_SHADOW_BYPASS` is a different shape — a binary feature-bypass toggle (orthogonal to the picker), so it was deferred.

### What this delivers

1. New `DevToggle` `PointShadowBypass` (bool) registered alongside `RasterizedGI` in `ExampleRenderingClient.cpp`.
2. New `g_Frame.pointShadowBypass` (uint, 0/1) field in `PerFrame_CB` / `PerFrameConstantBuffer` — or piggyback on a debugFlags bitmask if more A/B toggles arrive.
3. `lightPassDirectLighting.hlsl::EvaluateTiledPointLighting` reads the field at runtime; the `#define DEBUG_POINT_SHADOW_BYPASS` block + `#if/#else` is deleted.
4. Visual A/B capture archived showing toggle on/off in Sponza.

### Why low priority

The original `#define` is still in source; flipping it requires a rebuild but the existing visualization modes (`DebugView_DirectLightingOnly`) cover the same diagnostic question (is point-shadow contribution the only delta?) at a different angle. This is a polish task, not a blocker.

## Acceptance Criteria
- [ ] #1 `PointShadowBypass` DevToggle live; flips at runtime
- [ ] #2 `#define DEBUG_POINT_SHADOW_BYPASS` deleted from `lightPassDirectLighting.hlsl`
- [ ] #3 Visual A/B capture archived
- [ ] #4 Peer review per discipline
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
