---
id: TASK-30
title: >-
  Enforce path regime (relative vs. absolute) in IOService to eliminate silent
  path corruption
status: To Do
assignee: []
created_date: '2026-04-13 18:12'
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
