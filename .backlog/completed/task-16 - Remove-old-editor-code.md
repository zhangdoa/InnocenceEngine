---
id: TASK-16
title: Remove old editor code
status: Done
assignee: []
created_date: '2026-04-12 21:30'
updated_date: '2026-04-12 23:09'
labels:
  - cleanup
  - editor
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Remove retired editor code that is no longer used. The engine is moving toward a separate editor (being developed in a different workspace). Old in-engine editor infrastructure should be cleaned up to reduce dead code and build surface area.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 All old editor source files and references removed
- [x] #2 Build succeeds with no references to removed code
- [x] #3 RenderTest and Main.exe integration tests pass
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Removed all 40 files under Source/Editor/ (the old Qt-based property editor and main window) and cleaned up Source/CMakeLists.txt by removing the INNO_BUILD_EDITOR option and add_subdirectory("Editor") block. EditorService and Source/Editor-Next are the current WebSocket-based approach and were not touched. Build succeeded with zero errors; RenderTest exit code 0.
<!-- SECTION:FINAL_SUMMARY:END -->
