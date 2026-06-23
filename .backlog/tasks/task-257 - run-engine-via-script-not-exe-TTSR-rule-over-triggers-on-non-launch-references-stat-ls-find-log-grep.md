---
id: TASK-257
title: >-
  run-engine-via-script-not-exe TTSR rule over-triggers on non-launch references
  (stat/ls/find/log-grep)
status: To Do
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
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
