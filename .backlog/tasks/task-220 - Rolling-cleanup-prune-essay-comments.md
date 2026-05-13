---
id: TASK-220
title: 'Rolling cleanup: prune essay comments'
status: In Progress
assignee: []
created_date: '2026-05-05 20:26'
updated_date: '2026-05-13 23:25'
labels:
  - tech-debt
  - code-quality
  - rolling-cleanup
dependencies: []
references:
  - .claude/disciplines/always/comment-discipline.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rolling sweep against `disciplines/always/comment-discipline.md`. Per CL: pick one file (or tight cluster), drop comments not justified by the discipline, no behavior change, build + qualifying test green. Owner: stage that owns the file.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Each CL prunes one file or cluster; build + test green; no behavior change.
- [x] #2 Comments kept only when the discipline says keep.
- [ ] #3 Stays open as a rolling tracker; new violations caught at write-time, not refiled here.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-14 — Picked up autonomously after TASK-139 closure. Rolling tracker; this CL covers one file or tight cluster, per task ground rules. Selecting a non-rendering subtree to avoid any TASK-77.4 overlap (foundation / services / common are good candidates).

2026-05-14 — CL: prune essay comments in `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`. 96 lines removed, 0 inserted. Deleted: two large commented-out dead-code blocks (ProcessSkeleton, ProcessAnimations), four TASK-N/SHA design-history blocks (TASK-27, TASK-28, two TASK-144), `@TODO: Implement SkeletonComponent loading` stale TODO, two `// Note: For binary X data, additional fields are added by AssetService::Save` cross-file footnotes, and several WHAT-restatement / cross-file-reference lines. Kept: init-order WHY at TemplateAssetService re-entry, deferred-activation contract about BLAS readiness, perf WHY for off-loading STB decode to background thread. Build green (Engine.lib + Main.exe). `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran clean; final log line `Engine has been terminated.`; gpu_output.png + cpu_reference.png written. AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Iteration #2 CL: prune essay comments in `Source/Engine/Services/Common/FrameManagementServiceImpl_FrameQueries.cpp`. 39 lines removed, 4 inserted (two trimmed WHY one-liners replace multi-paragraph essays). Deleted: TASK-213 CL A/B design-history headers above `IsSteadyState` and `GetSteadyStateRelativeFrameCount`, multi-paragraph K=3 derivation citing a `Build/captures/...` log path (path will rot), 120-frame budget essay paraphrasing the constant name, "flap-back log" WHAT-restatement essay, "Update the rolling TLAS-stability counter" WHAT-restatement, "120-frame timeout watchdog (R2)" WHAT-restatement, TASK-213 CL B snapshot-rationale essay. Kept (trimmed to one-liner): K=3 invariant note about TLAS-rebuild gaps (concrete WHY for the magic number); harness-grep load-bearing warning on the "Auto-test: steady state reached at frame=" log string (external consumer reads it). No #include orphaned (every header still referenced by live code). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile`); `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran to `Engine has been terminated.`; load-bearing log line `Auto-test: steady state reached at frame=4` still fires. AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Iteration #3 CL: prune essay comments in `Source/Engine/Services/SceneService.cpp`. 34 lines removed, 10 inserted (two trimmed WHY one-liners replace multi-paragraph essays). Picked over `EditorService.cpp` for higher essay density. Deleted: numbered-phase WHAT-restatement banners (`// 0. Flush all GPU work...`, `// 2. Free GPU resources...`, `// 3. Destroy scene-scoped components`, `// 4. Clear transform hierarchy...`, `// 5. Clear physics simulation state...`, `// Load the new scene`, `// Loaded phase: // 6. Refresh...`, `// 7. Client loaded callbacks (GIDataLoader, VXGIRenderer, WorldSystem, Editor)` — cross-reference list of consumers); 5-line callback-ordering essay narrating "the previous order" history; 7-line `// 2b.` essay with `TASK-52` tombstone narrating page-fault symptom; 10-line `// 5b.` essay with `TASK-109` tombstone + "shader-ball-varies-every-launch" incident narrative. Kept (trimmed to one-liner): WHY for client-callback ordering vs resource destruction (non-obvious lifecycle invariant); WHY for asset-table generation bump (non-obvious correctness consequence of name collisions); WHY for synchronous drain + GPU-idle before resuming rendering (non-obvious data hazard). No #include orphaned (every service header still invoked by live code). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile`) — `Engine.lib` and `RenderTest.exe` linked; `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran clean to `Engine has been terminated.`. AC #3 keeps the task open as a rolling tracker.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
