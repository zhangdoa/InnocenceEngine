---
id: TASK-198
title: Unused-includes sweep across rendering subtree (clangd noise)
status: To Do
assignee: []
created_date: '2026-04-29 07:18'
labels:
  - hygiene
  - clangd
  - rendering
  - split-by-subtree
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

clangd surfaces unused-include warnings every session across the rendering subtree. Sample from ExampleRenderingClient.cpp alone (10 in one file): AnimationPass.h, TransparentGeometryProcessPass.h, TransparentBlendPass.h, VolumetricPass.h, MotionBlurPass.h, BillboardPass.h, DebugPass.h, BSDFTestPass.h, HIDService.h, Task.h. Similar piles in: VolumetricPass.cpp (4), DX12FrameManagementService.cpp (2), DX12GraphicsHardwareService.cpp (1), FinalBlendPass.cpp (2), RadianceCacheRaytracingPass.cpp (1), RadianceCacheReprojectionPass.cpp (2), RadianceCacheIntegrationPass.cpp (1), RenderingConfigurationService.{h,cpp} (2), JSONSerializer_Components.cpp (1), GPUDataStructure.h (1), LightPass.cpp (1) — all surfaced this session.

This is the "tool noise we learned to ignore" pattern (feedback_no_dismissing_tool_noise.md). Each warning is small; cumulatively they desensitize us to clangd output and bury real diagnostics.

## Goal

Eliminate the warning class entirely from the rendering subtree.

## Approach options

- **A — clangd `--tidy`-driven auto-fix (machine-applied).** Per-file run of clangd's "remove unused include" code action. Mechanical; safe with a build-verify after.
- **B — manual sweep, file by file.** More human-curated; can also catch headers that are "unused per clangd" but actually load-bearing for IWYU forward-declares or implicit transitive includes.

Pick during design. Combine if needed.

## Acceptance criteria

- [ ] All unused-include warnings cleared in `Source/ExampleProject/RenderingClient/`, `Source/Engine/Services/DX12/`, `Source/Engine/Services/RenderingConfigurationService.{h,cpp}`, `Source/Engine/Common/GPUDataStructure.h`
- [ ] `cmake --build Build --config RelWithDebInfo --target Main` clean post-sweep (no broken transitive-include reliance)
- [ ] Smoke test: short `Bin/RelWithDebInfo/Main.exe -total_frames 60` against Sponza, no regressions

## Owner

`rendering-researcher` for `Source/ExampleProject/RenderingClient/` files; `graphics-api-expert` for DX12/* files; `low-level-expert` for `Engine/Common/GPUDataStructure.h`. Coordinate on dispatch — three peer-reviewable batches, not one.

## References

- `.claude/disciplines/no-dismissing-tool-noise.md` if exists, else memory `feedback_no_dismissing_tool_noise.md`
- clangd warning class: `[unused-includes] (clangd)`
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
