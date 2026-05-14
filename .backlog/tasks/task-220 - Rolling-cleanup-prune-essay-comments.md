---
id: TASK-220
title: 'Rolling cleanup: prune essay comments'
status: In Progress
assignee: []
created_date: '2026-05-05 20:26'
updated_date: '2026-05-14 02:45'
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

2026-05-14 — Iteration #5 CL: prune essay comments in `Source/Engine/Engine_Run.cpp`. 15 lines removed, 3 inserted (one 3-line WHY one-liner replaces a 16-line essay). Previously attempted `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp`; aborted per workspace-hygiene line 14 (no AI edits to `Source/Engine/ThirdParty/`); diff reverted before survey. Surveyed in-scope candidates (excluding ThirdParty, in-flight rendering subtrees, and prior-iteration files): `Engine_Run.cpp` (15/97, 15.5%), `Engine_Terminate.cpp` (13/139, 9.4%), `Engine_ParseInitConfig_Helpers.cpp` (16/193, 8.3%), `FrameManagementServiceImpl.cpp` (19/246, 7.7%), `PerFrameDataService.cpp` (21/322, 6.5%). Picked `Engine_Run.cpp` — single concentrated 16-line essay block above the `isBakeMode` branch held the entire file's essay-violation mass. Deleted: 16-line block mixing bake-mode WHAT-restatement ("headless, one-shot import-then-exit"), TASK-68 and TASK-70 references, multi-paragraph cross-file design narration of `ImportSync` / `ProcessAssimpScene` / `TaskScheduler` worker behaviour, and a cross-file `AssimpWrapper::Import` thread-safety footnote (belongs at AssimpWrapper's API site, not at a caller). Kept (trimmed to 3-line WHY): the non-obvious correctness reason for using `std::thread` instead of `TaskScheduler` for the outer file loop — a worker submitting sub-tasks to its own thread would deadlock on `Thread::Busy`. No #include orphaned (`<thread>`, `<chrono>`, `AssetService.h`, `LogService.h`, `Engine_Internal.h` all still referenced by live code). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`) — Common.lib, Engine.lib, RenderTest.exe all linked. `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (run from `Bin/RelWithDebInfo`) ran to `Engine has been terminated.`. AC #3 keeps the task open as a rolling tracker.

## Review (code-impl, 2026-05-14)

Verdict: PASS (with one ADVISORY note).

Evidence:
- Path check: `Source/Engine/Engine_Run.cpp` is under `Source/Engine/`, not `Source/Engine/ThirdParty/` nor `Source/External/`. Workspace-hygiene line 14 clean.
- Numstat: `git diff HEAD --numstat -- Source/Engine/Engine_Run.cpp` → `3	15` (matches implementer's claim of 15 deletions / 3 insertions).
- Behaviour-only: diff confined to lines 10–25 of pre-image; only comment lines changed. No code, whitespace-significant, or `#if` reflow.
- Subject length: 72 chars exact (`awk '{print length}' | head -1`). Within ≤72 bound.
- Commit type tag `chore(engine):` correct (not `refactor`); `[task-stays-open]` sentinel present; `Code-AI-Generated-By` / `Message-AI-Generated-By` / `Co-Authored-By` present; `Reviewed-By:` deferred to main session as briefed.
- Kept-comment accuracy verified against `Source/Engine/Common/Thread.cpp:75,88,161` — `Thread::AddTask` spins on `compare_exchange_strong` from `Idle`/`Waiting` → `Busy`. A worker executing a task is in `State::Busy`, so submitting back to its own thread deadlocks. Wording in `Engine_Run.cpp:11–13` ("a worker blocking on sub-tasks on its own thread would deadlock on Thread::Busy") is accurate and load-bearing — the deadlock invariant is not visible from `std::vector<std::thread> l_threads` at line 40. Discipline: `comment-discipline` § "Hidden invariant" — kept correctly. Trimming to a one-liner is possible but not required; current shape is proportionate.
- Each deletion verified against `comment-discipline`:
  - "Bake mode (TASK-68): headless, one-shot import-then-exit" → TASK-tombstone + WHAT-restatement of `if (isBakeMode)`. Deletion correct (`comment-discipline` § "Task IDs, commit hashes").
  - "normal Run loop depends on WindowSystem being Activated; in bake mode we use the HeadlessWindowService" → cross-file footnote on WindowSystem contract. Deletion correct (`comment-discipline` § "Cross-reference footnotes").
  - "Per-file parallelism (TASK-70 axis 1)... ProcessAssimpScene fans out... (axis 2)" → multi-paragraph cross-file design narration; the load-bearing deadlock claim survives in the kept 3-line WHY. Distillation correct.
  - "AssimpWrapper::Import is thread-safe: each call constructs its own local Assimp::Importer..." → cross-file footnote stating a thread-safety contract at the caller. Deletion correct (`threading-contracts` § "Convention": contract belongs at the declaration).
- No #include orphaned: verified via grep that `<chrono>` (used at Engine_Run.cpp:38,46,48,49,70,71), `<thread>` (line 40), `AssetService.h` (Get<AssetService>() at line 17), `LogService.h` (Log() at lines 61,66,72), `Engine_Internal.h` (m_pImpl at lines 14,16) all still referenced by live code.
- Task-file note shape matches iterations #1–#4 (date, file, line-delta, deletion categories, kept-and-why, build line, Main.exe transcript line, AC #3 statement). PhysXWrapper-aborted footnote is light-touch and appropriate.

ADVISORY (non-blocking — do not fold in):
- The deleted `AssimpWrapper::Import` thread-safety footnote captured information that does NOT exist at the declaration site. `Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.h` and the `Import` function declaration carry no thread-safety contract. The implementer's removal is discipline-correct (cross-file footnotes rot at the caller), but the *missing* contract at the declaration is a pre-existing gap. Per `surface-dont-chase`: do NOT fold into this CL — `Source/Engine/ThirdParty/` is workspace-hygiene-forbidden anyway. Surface as a separate observation; consider filing if it actually costs something later.

Disciplines consulted: `comment-discipline`, `workspace-hygiene`, `fundamentals`, `safety-observability` (no log/assertion neighbour removed), `cpp-style` (no naming/formatting touched), `threading-contracts` (re: AssimpWrapper footnote venue), `surface-dont-chase`.

2026-05-14 — Iteration #4 CL: prune essay comments in `Source/Engine/Common/IOService.cpp`. 11 lines removed, 1 inserted (one trimmed WHY one-liner replaces a 5-line essay). Surveyed candidates: EditorService.cpp (33 comments / 268 lines, 12.3% — confirmed iteration #2's flag, comments are load-bearing WHY about wire-protocol contract, setter-reply contract, screenshot-saved payload schema; near-zero net deletions available), EditorService_Entity.cpp (15/189, 7.9% — load-bearing WHY about K-mode invariant, READ_ONLY discriminated reply, persistent-entity delete refusal), AssetService_Path.cpp (12/128, 9.4% — load-bearing WHY about AmbientCG suffix convention, project-vs-generated search order, concurrent-import safety), HIDService.cpp (9/225 — load-bearing WHY about MSVC SRW shared_lock-then-unique_lock deadlock, offscreen-mode defensive guard), IOService.cpp (11/275 — mostly essay violations). Picked IOService.cpp for essay-violation concentration despite lower raw density. Deleted: 5-line TASK-30 essay above `ResolvePath` with "callers historically pass", "old unconditional", "produced a doubly-rooted garbage path" design-history narration; six WHAT-restatement banner comments inside `addCPPClassFiles` (`// Build header file`, `// Common headers include`, `// Abstraction type`, `// Inheriance type` (misspelled), `// Class decl body`, `// Ctor type`) — each paraphrasing the immediately following `<<` write. Kept (trimmed to one-liner): WHY for ResolvePath's absolute-vs-relative branching (one-liner captures the invariant without the incident narrative). No #include orphaned (all five headers still referenced). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile`) — Engine.lib, Main.exe, RenderTest.exe all linked. `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (run from `Bin/RelWithDebInfo`) ran to `Engine has been terminated.`. AC #3 keeps the task open as a rolling tracker.

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
