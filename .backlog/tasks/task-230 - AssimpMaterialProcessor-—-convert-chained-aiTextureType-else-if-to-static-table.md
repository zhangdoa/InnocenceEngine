---
id: TASK-230
title: >-
  AssimpMaterialProcessor — convert chained aiTextureType else-if to static
  table
status: To Do
assignee: []
created_date: '2026-05-17 12:54'
labels:
  - rendering
  - asset-pipeline
  - hygiene
  - followup
dependencies: []
references:
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 peer review (`f407f461`). `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp:192-243` uses chained `else if` over `aiTextureType` where each branch sets 4 locals (slot index, sRGB flag, channel-source, slot-specific). A static `aiTextureType → { slot, sRGB, channelSource }` table would express the finite enumeration more clearly than chained branches and reduce the surface-for-mistake when adding new types.

Pre-existing shape; TASK-228 only added one local + one assignment per branch. Surfaced separately per `surface-dont-chase`.

Skill anchors: `code-organization` (split-by-concern), `safety-principles` (no magic-number scatter).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 aiTextureType assignment table extracted as a `static constexpr` (or equivalent) lookup
- [ ] #2 Branches replaced with a single table lookup + uniform application
- [ ] #3 Build green
- [ ] #4 No behavioural change vs `f407f461` (verify by re-running GISponza audit + UnitTest smoke)
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
