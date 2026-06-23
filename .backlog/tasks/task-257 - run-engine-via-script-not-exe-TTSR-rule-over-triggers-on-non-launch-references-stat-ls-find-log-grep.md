---
id: TASK-257
title: >-
  run-engine-via-script-not-exe TTSR rule over-triggers on non-launch references
  (stat/ls/find/log-grep)
status: Done
assignee: []
created_date: '2026-06-23 16:24'
labels:
  - harness
  - ttsr-rule
  - false-positive
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
EMPIRICAL (2026-06-23): the rule .omp/rules/run-engine-via-script-not-exe.md uses condition regex (?:Main|RenderTest)\.exe — a bare substring with no launch-vs-reference discrimination. It fired this session while I was merely READING the wrapper module (Scripts/Lib/Test-Engine.psm1, whose source contains 'Main.exe') to learn the SANCTIONED launch method — i.e. it interrupted compliance research, not a violation. The same regex matches every non-launch reference: stat-ing the binary mtime (stat Bin/RelWithDebInfo/Main.exe), listing it, find-ing it, or grepping a log path under Bin/ that contains the exe name. All of these are common (checking build freshness, locating the exe, scanning logs) and none launch the engine. The rule's intent — block naked Main.exe/RenderTest.exe LAUNCHES that get the CWD/-c/exit-code wrong — is correct; only its trigger surface is too broad. FIX SHAPE: anchor the pattern to actual invocation, e.g. the exe in command position or immediately followed by a launch flag ((?:Main|RenderTest)\.exe(?=\s+-) or at a command boundary), so stat/ls/find/grep/wc/cat references don't trip it. Per writing-hookify-rules: add/extend the rule's test cases (positive: naked launch + launch with flags; negative: stat/ls/find/log-grep/reading the wrapper) before/with the regex change.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 N/A — harness rule + doc change, no engine build.
- [x] #2 No pre-existing test for this rule; the regex was verified directly (see #3).
- [x] #3 New verification (real, not mock): the parsed regex `(?:Main|RenderTest)\.exe["']?\s+-` run against 5 launch samples (all MATCH) and 8 reference samples — stat / ls / find / wc / Get-Process, the module source lines `$mainExe = Join-Path $BinDir 'Main.exe'` and `Start-Process -FilePath $mainExe`, and the sanctioned `StartEngineWin.ps1 -Preset …` command (all NO-match). Match-table in the session transcript.
- [x] #4 Not mock-based: a match-table over real command strings.
- [x] #5 User-observable: the exact incident is reproduced as no-match — the two wrapper-module source lines that carried `Main.exe` no longer trigger.
- [x] #6 NOT verified / accepted gaps: (a) a truly argument-less `Main.exe` (no flags) slips through — no caller runs the engine that way and the wrappers always pass `-c`/`-test`; (b) `find -name Main.exe -<flag>` (name not last) still matches — rare, `find` tool is preferred over bash `find`. The exact rule-engine trigger surface (bash command vs. scanned output) was not instrumented; the tighter regex removes the false-positive regardless.
<!-- DOD:END -->

## Closure (2026-06-23)

Fixed. `.omp/rules/run-engine-via-script-not-exe.md` condition tightened from the
bare substring `(?:Main|RenderTest)\.exe` to `(?:Main|RenderTest)\.exe["']?\s+-`
(launch-with-flag), plus a body paragraph documenting it fires only on a real
launch and noting the accepted naked-launch gap. Verified by a regex match-table
(5 launch positives match; 8 reference negatives — including the wrapper-module
source lines that caused the original interrupt — no-match).
