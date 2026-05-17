---
id: TASK-232
title: >-
  AssimpTextureProcessor — extract `"RGBA"[channel-source]` indexing into named
  constant
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
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp
  - Source/Engine/Common/BCCompression.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 peer review (`f407f461`). `AssimpTextureProcessor.cpp:30` uses `"RGBA"[static_cast<uint32_t>(BC4Source)]` to suffix the texture-instance name with `_chB` / `_chG` when `bc4Source != R`.

Works correctly (`TextureChannelSource` is `uint8_t` 0-3, `"RGBA"` is 5-byte literal `R/G/B/A/\0`), but a `static constexpr const char* kChannelTag = "RGBA";` near the enum declaration would localize the convention and avoid the inline-string-literal indexing pattern.

Single-use hygiene improvement; non-blocking. Surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Named constant for channel-source-tag chars introduced near the `TextureChannelSource` enum (or equivalent localized owner)
- [ ] #2 AssimpTextureProcessor.cpp:30 references the named constant instead of inline string-literal
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
