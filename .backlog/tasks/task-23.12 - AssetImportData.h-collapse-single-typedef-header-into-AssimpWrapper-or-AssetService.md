---
id: TASK-23.12
title: >-
  AssetImportData.h: collapse single-typedef header into AssimpWrapper or
  AssetService
status: To Do
assignee: []
created_date: '2026-05-22 07:33'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: low
ordinal: 12000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/AssetImportData.h contains exactly one declaration:

```
typedef std::function<void(float, const char*)> AssetImportProgressCallback;
```

This is too thin to deserve its own header. One typedef + a doc comment = a header. The two consumers are:
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpWrapper.h` (the actual asset importer)
- `Source/Engine/Services/AssetService.h`

Plan: move the typedef into whichever TU is the "owner" of asset import. Most natural home is `AssimpWrapper.h` (closest to the actual import operation) — `AssetService.h` then includes `AssimpWrapper.h` or gets a forward decl.

Filename rationale ("AssetImportData.h" — "Data" suffix is bad name): see TASK-23.11. After collapse, this becomes a non-issue.

References:
- Source/Engine/Common/AssetImportData.h (to delete)
- Source/Engine/ThirdParty/AssimpWrapper/AssimpWrapper.h
- Source/Engine/Services/AssetService.h
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 AssetImportProgressCallback typedef moved to AssimpWrapper.h (or the chosen owner TU).
- [ ] #2 Source/Engine/Common/AssetImportData.h deleted.
- [ ] #3 Both consumers (AssimpWrapper.h, AssetService.h) compile with the new include chain.
- [ ] #4 No #include "AssetImportData.h" remains anywhere in the tree.
- [ ] #5 Main.exe -total_frames 10 exits 0.
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
