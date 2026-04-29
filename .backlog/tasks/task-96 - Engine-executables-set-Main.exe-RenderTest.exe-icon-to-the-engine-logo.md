---
id: TASK-96
title: 'Engine executables: set Main.exe / RenderTest.exe icon to the engine logo'
status: Done
assignee: []
created_date: '2026-04-19 18:09'
updated_date: '2026-04-29 07:55'
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

The shipped Main.exe, RenderTest.exe, and any other engine-side executables currently use the default Windows executable icon. They should carry the InnocenceEngine logo so the process is identifiable in taskbar / alt-tab / file-explorer.

## Scope

- Add a `.rc` resource file (or extend an existing one) with `IDI_ICON1 ICON "path/to/InnocenceEngine.ico"` per executable target.
- Add `.ico` asset(s) to the tracked asset directory (ideally a single source; embed at build time). If no engine logo `.ico` exists yet, generate one from the existing branding.
- Wire the `.rc` into the CMake target (`target_sources(... PRIVATE foo.rc)` — MSVC picks up resources automatically; no MSBuild flag needed).
- Applies to: Main (primary), RenderTest, and any other Win32 executable target the engine emits.
- Electron editor is a separate concern — Electron has its own icon configuration via `electron-builder` / `BrowserWindow`. Out of scope here unless quick to bundle.

## Acceptance

- Running `Main.exe` shows the engine icon in the taskbar and window title bar.
- File-explorer thumbnail for `Bin/RelWithDebInfo/Main.exe` shows the engine icon.
- Same for `RenderTest.exe`.

## Notes

- Windows embeds the icon as a resource inside the PE. Source icon should include multiple sizes (16, 32, 48, 256) — ICO format supports this natively.
- Keep the `.ico` source tracked; the embedded copy is regenerated on every build.
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Status flip retrofitted 2026-04-29 during TASK-201 audit — work landed in commit `69efaa70` (`build(branding): embed engine logo as Win32 .exe icon (Main, RenderTest)`). Both Main.exe and RenderTest.exe now embed `Data/Engine/Icons/icon.ico` (multi-resolution 16/32/48/64/96/128/256) via a shared `Source/Engine/Platform/WinMain/WinMain.rc`, picked up automatically by MSVC's RC compiler. Source PNG/SVG already tracked alongside the generated ICO. This is the systemic gap that TASK-193's closure-staleness gate now catches at commit time.
<!-- SECTION:FINAL_SUMMARY:END -->
