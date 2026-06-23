---
id: TASK-256
title: >-
  Harden headless engine-run verification — offscreen stdout AND .Log both empty
  makes grep-predicates false-fail
status: To Do
assignee: []
created_date: '2026-06-23 16:24'
labels:
  - harness
  - verification
  - test-infra
  - audit-dump
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
EMPIRICAL (2026-06-23, closing TASK-241): a fully-healthy offscreen Audit run (exit 0, all 17 render-graph pass HDRs dumped to Bin/audit_*.hdr) produced a 0-byte captured stdout AND a 0-byte timestamped .Log. Invoke-EngineBounded redirects stdout to a file and Test-EngineRunOutcome greps it; with an empty sink the predicates report sceneLoaded=0, autoTerminated=0, D3D12-errors=0 — i.e. a FALSE FAIL on a healthy run (observed directly in the first bounded run this session). Scripts/CLAUDE.md claims 'stdout is the only reliable sink' — contradicted this session (Main.exe is /SUBSYSTEM:WINDOWS / WinMain and writes nothing to a redirected or inherited console). The reliable signals were the process EXIT CODE (StartEngineWin.ps1 does -Wait + exit $proc.ExitCode) and the PRODUCED ARTIFACTS (Bin/audit_*.hdr). Risk: an agent trusting Test-EngineRunOutcome wastes effort 'debugging' a healthy engine. FIX SHAPE: (1) make headless verification assert primarily on exit code + expected-artifact existence/size, with text-greps as a secondary signal only when the sink is non-empty; (2) correct the Scripts/CLAUDE.md 'stdout is the reliable sink' note for offscreen runs; (3) related: magick is broken locally (magick -version exits 5; every HDR convert/stat hung 44-178s), so TestGIScene.ps1's MAE compare silently can't run — health-probe 'magick -version' exit code (not just Get-Command presence) and skip cleanly if broken; (4) there is NO working dependency-light 'is this HDR black?' check (magick broken, Python imageio/cv2 import hung) — a tiny RGBE stat reader (PowerShell or small C++) would make the audit-dump evidence path machine-checkable instead of relying on a file-size proxy.
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
