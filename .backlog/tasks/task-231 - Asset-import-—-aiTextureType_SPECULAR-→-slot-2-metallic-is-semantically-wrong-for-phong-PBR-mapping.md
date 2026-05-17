---
id: TASK-231
title: >-
  Asset import — aiTextureType_SPECULAR → slot 2 (metallic) is semantically
  wrong for phong PBR mapping
status: To Do
assignee: []
created_date: '2026-05-17 12:54'
labels:
  - rendering
  - asset-pipeline
  - bug
  - followup
dependencies: []
references:
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 peer review (`f407f461`). `AssimpMaterialProcessor.cpp:214-219` maps `aiTextureType_SPECULAR` (phong specular color texture) to slot 2 (metallic) in the BC4 import path. `ProcessMaterialProperties` at lines 119-129 deliberately AVOIDS translating phong specular color to metallic (different concepts; produces spurious gold cloth on phong-authored assets). The texture-side mapping disagrees with the attribute-side decision.

Likely no visible bite right now (Sponza uses glTF MR not phong specular; UnitTest spheres use simple materials), but the inconsistency is a latent bug — any FBX asset with `aiTextureType_SPECULAR` will get its specular color packed as metallic, breaking PBR semantics.

Pre-existing shape; TASK-228 left the mapping untouched per scope. Surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Decision recorded: drop SPECULAR mapping, or route SPECULAR to a non-metallic-conflicting slot, or document why current behaviour is acceptable
- [ ] #2 If a code change lands: regression-test with an FBX phong asset to verify spurious-metallic is resolved
- [ ] #3 Build green
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
