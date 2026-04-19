---
id: TASK-97
title: 'Engine executables: set Main.exe / RenderTest.exe icon to the engine logo'
status: To Do
assignee: []
created_date: '2026-04-19 18:11'
labels:
  - engine
  - build
  - branding
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Main.exe, RenderTest.exe, and any other engine-side executables currently use the default Windows icon. They should carry the InnocenceEngine logo so the process is identifiable in taskbar / alt-tab / file-explorer.

## Scope

- Add a `.rc` resource file (or extend an existing one) with `IDI_ICON1 ICON "path/to/InnocenceEngine.ico"` per executable target.
- Add a source `.ico` asset (multi-size: 16, 32, 48, 256) in a tracked location.
- Wire into CMake via `target_sources(<target> PRIVATE foo.rc)` — MSVC auto-compiles `.rc`.
- Applies to Main, RenderTest, and any other Win32 executable target.
- Electron editor is a separate concern (electron-builder / BrowserWindow icon); out of scope unless trivial.

## Acceptance

- `Main.exe` and `RenderTest.exe` show the engine icon in the taskbar, window title bar, and explorer thumbnail.
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
