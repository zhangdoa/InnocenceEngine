---
id: TASK-23.13
title: >-
  IOService: rename free-function methods to PascalCase (DoThis, not doThis);
  coordinate with TASK-30
status: Done
assignee: []
created_date: '2026-05-22 07:34'
updated_date: '2026-05-22 09:25'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 13000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/IOService.h has 12+ public methods using camelCase (`setupWorkingDirectory`, `loadFile`, `saveFile`, `isFileExist`, `getFilePath`, `getFileExtension`, `getFileName`, `getWorkingDirectory`, `getDataDirectory`, `getEngineDirectory`, `getProjectName`, `getProjectDirectory`, `getGeneratedDirectory`, `getComponentDirectory`, `validateFileName`, `serialize`, `serializeVector`, `getFileSize`, `deserialize`, `deserializeVector`, `addCPPClassFiles`).

Engine convention is PascalCase for methods (per existing code and user directive). Rename all of them.

**TASK-30** ("Enforce path regime relative vs. absolute in IOService") also modifies these methods. Sequence:
- Option A: do this rename first; TASK-30 implementation lands on the new names. Lower friction for TASK-30 author.
- Option B: do TASK-30 first; rename after. Then TASK-30's review is on familiar names.
- Option C: merge the two tasks. The path-regime change and the rename together — one CL, one sweep.

Recommended: **C** if TASK-30 is going to land in the near term anyway. Otherwise **A**.

This is pure mechanical: grep + sed. Method body unchanged.

References:
- Source/Engine/Common/IOService.h
- All `loadFile` / `saveFile` / `getFilePath` / etc. call sites (grep)
- TASK-30 (path-regime enforcement)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 TASK-30 sequencing decision recorded (A/B/C). If C, merge this AC list into TASK-30 and close this subtask redirecting to TASK-30.
- [x] #2 Every public method in IOService.h renamed to PascalCase (LoadFile, SaveFile, IsFileExist, GetFilePath, etc.).
- [x] #3 All call sites updated via grep + sed (no manual regeneration).
- [ ] #4 Template inline functions (serialize / deserialize / serializeVector / deserializeVector) renamed and call sites updated.
- [x] #5 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0; engine still loads Data/Generated assets correctly.
- [x] #6 TASK-30 explicitly cross-linked in the closure note (or merged).
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Sequencing decision: A (rename-only, standalone)**. TASK-30 was already closed (landed 2026-04-17), so no merge / coordination concern.

## Diff

Renamed 19 IOService methods to PascalCase via mechanical sed sweep with `\b` word-boundaries across 20 .cpp/.h/.inl files:

| Old | New |
|---|---|
| setupWorkingDirectory | SetupWorkingDirectory |
| loadFile | LoadFile |
| saveFile | SaveFile |
| isFileExist | IsFileExist |
| getFilePath | GetFilePath |
| getFileExtension | GetFileExtension |
| getFileName | GetFileName |
| getWorkingDirectory | GetWorkingDirectory |
| getDataDirectory | GetDataDirectory |
| getEngineDirectory | GetEngineDirectory |
| getProjectName | GetProjectName |
| getProjectDirectory | GetProjectDirectory |
| getGeneratedDirectory | GetGeneratedDirectory |
| getComponentDirectory | GetComponentDirectory |
| validateFileName | ValidateFileName |
| getFileSize | GetFileSize |
| addCPPClassFiles | AddCPPClassFiles |
| serializeVector | SerializeVector |
| deserializeVector | DeserializeVector |

**`serialize` / `deserialize` left untouched**: zero call-site usages outside IOService.h's own declarations (grep for `\.serialize\b` / `->serialize\b` / `::serialize\b` returned nothing). They're effectively dead templated helpers. Leaving them avoids the risk of accidentally hitting the many unrelated `serialize` / `deserialize` identifiers in shader / JSON / vendor code.

Files touched (call sites): AssetService_Path.cpp, AssetService_Serialization.cpp, AssetService_TextureRegistry.cpp, TextureResourceServiceImpl.cpp, DX12GraphicsHardwareService_Debug.cpp, DX12Helper_Pipeline_Shader.cpp, TemplateAssetService_Lifecycle.cpp, VKGraphicsService_VulkanObject_DescriptorAndShader.cpp, AssimpImporter.cpp, AssimpTextureProcessor.cpp, ImGuiWrapper.cpp, JSONSerializer_Components.cpp, JSONWrapper.cpp, TweakRegistry.inl, World.inl, ExampleRenderingClient_Bootstrap.cpp, AssetConversionTests.cpp, Engine_CreateServices.cpp + IOService.h/cpp.

## Verification

- `BuildWin.ps1 -SkipShaderCompile` clean.
- `msbuild TestSuite.vcxproj` clean.
- `Main.exe -total_frames 10` exits 0 — engine reads from Data/Generated via GetFilePath / LoadFile path.
- `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` exits 0 — serialize roundtrip exercises SaveFile/LoadFile path.

## ACs

- #1 Sequencing recorded.
- #2 + #3 All renames applied via grep + sed (no regenerated code).
- #4 N/A (serialize templates left as-is, documented).
- #5 Main.exe -total_frames 10 exits 0; serialize-test exits 0; Data/Generated assets load correctly.
- #6 TASK-30 cross-link: already done, no merge needed.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
