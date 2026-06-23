---
id: TASK-256
title: >-
  Harden headless engine-run verification — offscreen stdout AND .Log both empty
  makes grep-predicates false-fail
status: Done
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
- [x] #1 N/A for the engine; the harness scripts are validated by running them (#3). No engine rebuild.
- [x] #2 No pre-existing test covers `Test-Engine.psm1`; validated by direct runs (#3).
- [x] #3 New verification (real, not mock): (a) a throwaway harness imported the module and ran `Test-EngineArtifacts` — render subset (BaseColor/LightPass/FinalBlend) Pass=True, a 5-byte file → degenerate, a missing glob → missing; (b) `TestGIScene.ps1` run end-to-end on the live (broken-magick, offscreen) box: engine exit 0 → primary PASS, empty `.Log` → INCONCLUSIVE (not FAIL), broken magick → clean WARN-skip → `PASS (exit-code + log assertions only)`, no orphans. The OLD script would have exited 1 on the empty log.
- [x] #4 Not mock-based: real module + real audit artifacts + a real engine run.
- [x] #5 User-observable: the `TestGIScene.ps1` terminal transcript shows the corrected behavior (exit-code gate, LogEmpty advisory, magick-health WARN-skip → PASS).
- [x] #6 NOT verified: (a) the HDR-non-black check is a file-SIZE heuristic, not a pixel decode — a full RGBE-RLE reader was judged over-engineering for a smoke gate (slow + bug-prone in PowerShell; size cleanly separates 280-666KB real from few-KB black/uniform), and it must target scene-render passes (LUTs/masks are legitimately small); (b) the windowed-run path (populated `.Log`) was not re-exercised; (c) the magick install is still broken locally — only the harness's tolerance of it was fixed.
<!-- DOD:END -->

## Closure (2026-06-23)

Fixed across four files:
- `Scripts/Lib/Test-Engine.psm1`: methodology header corrected (exit code +
  artifacts are the reliable offscreen signals, not stdout/.Log);
  `Test-EngineRunOutcome` now detects an empty log (`LogEmpty=$true`) and does
  NOT hard-fail load/terminate on it; `Invoke-EngineBounded` ExitCode read made
  robust (reap + guard); new `Test-EngineArtifacts` helper (existence +
  non-degenerate size; documented to target scene-render passes, not LUTs/masks).
- `Scripts/TestGIScene.ps1`: exit code is the primary pass gate; log-greps
  advisory; a magick HEALTH probe (`magick -version` exit) replaces the
  presence-only check, so a broken magick WARN-skips instead of hanging.
- `Scripts/CLAUDE.md` + `.omp/AGENTS.md`: the "stdout is the reliable sink"
  guidance corrected to "gate on exit code + produced artifacts."
Verified by a helper match/size test and a full `TestGIScene.ps1` run (PASS on
the broken-magick offscreen environment; the old script would have false-failed).
