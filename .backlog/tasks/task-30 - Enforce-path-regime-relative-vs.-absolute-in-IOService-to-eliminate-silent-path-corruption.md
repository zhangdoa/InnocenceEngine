---
id: TASK-30
title: >-
  Enforce path regime (relative vs. absolute) in IOService to eliminate silent
  path corruption
status: Done
assignee: []
created_date: '2026-04-13 18:12'
updated_date: '2026-04-17 02:01'
labels:
  - architecture
  - explicit-contracts
  - io
  - orthogonality
dependencies: []
references:
  - Source/Engine/Common/IOService.cpp
  - Source/Engine/Services/AssetService.cpp
  - Source/Engine/Services/Common/TextureResourceServiceImpl.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
IOService::loadFile unconditionally prepends the working directory to any path it receives. When given an absolute path (e.g., from AssetService::GetBinaryFilePath), it silently produces a doubly-rooted garbage path. The file fails to open, and the caller gets an empty result with no indication that the path construction itself was wrong. This caused BC texture binaries to silently fail to load (fixed in 24381950 by bypassing IOService entirely).

**Structural weakness:** The engine operates with two implicit path regimes — relative (relative to working/data directory) and absolute — with no API-level distinction between them. Any code that constructs a path must manually know which regime IOService expects. This is an invisible invariant enforced only by convention, not by the type system or the API.

**Target improvement:**
- IOService::loadFile should detect absolute paths (e.g., `std::filesystem::path::is_absolute()`) and skip prepending the working directory
- OR introduce typed path wrappers — `RelativePath` and `AbsolutePath` — so the regime is enforced at compile time and callers cannot accidentally mix them
- At minimum, IOService should log a warning when the constructed path does not exist before returning an empty result, so path construction bugs are immediately visible
- Audit all call sites of IOService::loadFile/loadBinaryFile that pass paths from AssetService::GetBinaryFilePath or similar absolute-path sources
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):** `IOService::{loadFile, saveFile, isFileExist}` now detect absolute paths via `fs::path(filePath).is_absolute()` and skip prepending `m_workingDir` in that case. Introduced a file-local `ResolvePath()` helper to centralise the decision. Error messages now include both the caller-supplied path and the resolved path, so future path-construction bugs are immediately visible.

TASK-30 limits itself to making IOService safe for both regimes at the API boundary. A stricter typed-path design (`RelativePath` / `AbsolutePath` wrappers) is a bigger refactor and was not in scope.

**Caller audit:** No IOService::{loadFile,saveFile} call site currently passes an absolute path (all go through `getDataDirectory()` / `getWorkingDirectory()` + relative suffix). `AssimpTextureProcessor::isFileExist` passes a `ModelBaseDir + normalizedFileName` string which is relative. `TextureResourceServiceImpl` intentionally bypasses IOService via `std::ifstream` for absolute BC-binary paths; that comment can stay but is no longer mandatory.

**Validation:** Build clean. RenderTest exit 0. Integration test unchanged (still TASK-52 TDR, as expected).
<!-- SECTION:NOTES:END -->
