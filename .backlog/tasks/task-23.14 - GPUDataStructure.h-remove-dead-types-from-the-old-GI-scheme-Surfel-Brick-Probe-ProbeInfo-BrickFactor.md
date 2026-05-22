---
id: TASK-23.14
title: >-
  GPUDataStructure.h: remove dead types from the old GI scheme
  (Surfel/Brick/Probe/ProbeInfo/BrickFactor)
status: To Do
assignee: []
created_date: '2026-05-22 07:34'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: low
ordinal: 14000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/GPUDataStructure.h declares several types from an older GI scheme (the pre-SSRC / pre-radiance-cache work):

- `Surfel`, `SurfelGrid` (`using SurfelGrid = Surfel;` is itself dubious)
- `Brick`, `BrickFactor`
- `Probe`, `ProbeInfo`

Grep result (`Surfel|Brick|Probe|ProbeInfo` in Source/Engine): **only GPUDataStructure.h itself**. Shader files (`GIResolveSurfelPass.comp`, `GIBakeBrickFactorPass.frag`, etc.) use these names as shader-local structs, not C++ symbols — confirm by reading the .comp / .frag files.

Plan:
1. Confirm zero C++ consumers (already done).
2. Confirm the shader-side definitions are independent (not generated from this header). Spot-check `GIResolveBrickPass.comp` and `GIResolveSurfelPass.comp`.
3. Delete the stale C++ structs from GPUDataStructure.h.
4. Optionally: audit other types in GPUDataStructure.h that look similarly stale (`VoxelizationConstantBuffer` is referenced in LightDataService.cpp — keep; `AnimationConstantBuffer` is referenced in AnimationDrawCallService.cpp — keep).

References:
- Source/Engine/Common/GPUDataStructure.h
- Source/Shaders/HLSL/WIP/GI*Pass.{comp,frag} (shader-side, expected to be independent)
- Source/ExampleProject/RenderingClient/SSRC*.cpp (verify SSRC code doesn't pull from these)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Surfel, SurfelGrid, Brick, BrickFactor, Probe, ProbeInfo struct declarations deleted from GPUDataStructure.h.
- [ ] #2 Confirmation in closure note that shader-side .comp/.frag definitions are independent (not depending on the deleted C++ symbols).
- [ ] #3 Build green; Main.exe -total_frames 10 exits 0; SSRC GI test path still renders.
- [ ] #4 Optional: any other stale types found during this audit listed in closure note for future cleanup.
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
