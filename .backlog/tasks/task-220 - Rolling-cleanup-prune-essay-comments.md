---
id: TASK-220
title: 'Rolling cleanup: prune essay comments'
status: In Progress
assignee: []
created_date: '2026-05-05 20:26'
updated_date: '2026-05-21 19:22'
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
2026-05-15 — Iteration #7 CL: prune essay comments in `Source/Engine/Engine_Terminate.cpp`. 11 lines removed, 3 inserted (two trimmed WHY one-liners replace multi-paragraph phase-essay blocks). Surveyed iteration #5's flagged candidate pool: `Engine_Terminate.cpp` (139 lines, 13 comment regions — concentrated Phase-1/Phase-2 essays with TASK-42 tombstone and two WHAT-restatement banners) vs `Engine_ParseInitConfig_Helpers.cpp` (193 lines, 16 comment regions — mostly parser-format hints that are load-bearing for users reading the parser, plus one TASK-68 bake-mode footnote). Picked `Engine_Terminate.cpp` for higher net essay-violation mass — parser hint comments in the sibling file are functionally documenting the CLI grammar at the parse site and have lower deletion yield. Deleted: 1-line WHAT-restatement `// Only wait for rendering task if it was created` paraphrasing the immediately-following `if (m_pImpl->m_RenderingExecutionTask)`; 5-line "Phase 1 of shutdown — GPU-alive finalization" essay with TASK-42 tombstone narrating "makes the invariant structural" design history (substantive WHY about GPU-alive ordering distilled into kept 2-line one-liner); 4-line "Phase 2 — LogicClient CPU-heavy shutdown" essay paraphrasing the IRenderingClient::FinalizeGPUResults declaration's contract (canonical contract lives at `IRenderingClient.h:18-22` already, where the long-running-CPU-work / TDR-window WHY is documented; deleted essay was a cross-file footnote at the consumer); 1-line WHAT-restatement `// Only terminate rendering-related services if not headless` paraphrasing the immediately-following `if (!m_pImpl->m_initConfig.isHeadless)`. Kept (already trimmed at neighbour, retained as-is): the 2-line "Drain the GPU before destroying resources" WHY (non-obvious — the last frame's commands are in-flight with no BeginFrame to wait). Distilled (trimmed to 2 lines): "GPU-alive finalization must run before LogicClient::Terminate; the next phase may stall the GPU long enough to trip TDR" — captures the load-bearing ordering invariant + TDR consequence without the TASK-42 tombstone or duplicated contract-paraphrase. Distilled (trimmed to 1 line): bake-mode skip rationale — "client was never Setup/Initialize'd" — captures the non-obvious WHY for the `!m_pImpl->m_initConfig.isBakeMode` guard. No #include orphaned (all 30 service headers still invoked by live `SystemTerm(...)` macros and `Get<...>()` calls below the comment region; `LogService.h` for the four remaining `Log()` calls at L52/L59/L72/L118/L128; `TaskScheduler.h` for the inline task submission; `Engine_Internal.h` for `m_pImpl`). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`) — Common.lib, Engine.lib, Main.exe, RenderTest.exe all linked. `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (run from `Bin/RelWithDebInfo`) ran to `Engine has been terminated.`. File now 131 lines (under 300-line ratchet). AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Iteration #6 CL: prune essay comments in `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp`. 20 lines removed, 9 inserted (three trimmed WHY one-liners replace multi-paragraph essays). Surveyed in-scope candidates from iteration #5's flagged pool: `Engine_Terminate.cpp` (139 lines — comments mostly load-bearing GPU-alive / TDR WHY, low essay-violation mass), `Engine_ParseInitConfig_Helpers.cpp` (193 lines — moderate essay-violation mass, mostly TASK-N tombstones + format-string paraphrases), `FrameManagementServiceImpl.cpp` (246 lines — high essay-violation concentration: TASK-140 + TASK-213 CL A tombstones, two callback-contract paraphrases, one cross-file DX12 footnote), `PerFrameDataService.cpp` (322 lines — over 300-line ratchet, skipped). Picked `FrameManagementServiceImpl.cpp` for highest essay-violation concentration. Deleted: 2-line cross-file footnote describing m_GlobalSemaphore creation in DX12 backend's CreateHardwareResources (cross-file footnote, belongs at the DX12 backend's CreateSyncPrimitives site); 3-line PreFrameCallback contract-paraphrase ("clients use this for debug/capture/instrumentation triggers... FrameManagement owns frame pacing; it does NOT own RenderDoc"); 3-line PostFrameCallback contract-paraphrase (same shape — contract belongs at the callback's declaration); 5-line ResolveGpuTimers essay with `TASK-140` tombstone narrating ordering; 3-line `(void)IsSteadyState()` essay with `TASK-213 CL A` tombstone narrating CL-staging design history. Kept (trimmed to two-line WHYs): BeginFrame/HasGPUError ordering invariant (fence-drain before error check is non-obvious); error-path CPU-callback rationale (still-run-callbacks invariant non-obvious — without it the logic client can't trigger auto-termination); ResolveGpuTimers dual ordering invariant (must run after per-pass submit AND after BeginFrame drained the older slot — both are non-obvious correctness consequences); `(void)IsSteadyState()` side-effect WHY (the discarded boolean is intentional; the side-effect — advancing the rolling window and firing the first-true log marker from inside — is the actual reason for the call). No #include orphaned (LogService.h at Log() sites L30/L42/L49/L57/L74/L83/L89/L139/L230/L232; all service headers still invoked by live code; Engine.h for g_Engine). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`) — Common.lib, Engine.lib, RenderTest.exe all linked. `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (run from `Bin/RelWithDebInfo`) ran to `Engine has been terminated.`. File now 236 lines (under 300-line ratchet). AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Picked up autonomously after TASK-139 closure. Rolling tracker; this CL covers one file or tight cluster, per task ground rules. Selecting a non-rendering subtree to avoid any TASK-77.4 overlap (foundation / services / common are good candidates).

2026-05-14 — CL: prune essay comments in `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`. 96 lines removed, 0 inserted. Deleted: two large commented-out dead-code blocks (ProcessSkeleton, ProcessAnimations), four TASK-N/SHA design-history blocks (TASK-27, TASK-28, two TASK-144), `@TODO: Implement SkeletonComponent loading` stale TODO, two `// Note: For binary X data, additional fields are added by AssetService::Save` cross-file footnotes, and several WHAT-restatement / cross-file-reference lines. Kept: init-order WHY at TemplateAssetService re-entry, deferred-activation contract about BLAS readiness, perf WHY for off-loading STB decode to background thread. Build green (Engine.lib + Main.exe). `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran clean; final log line `Engine has been terminated.`; gpu_output.png + cpu_reference.png written. AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Iteration #2 CL: prune essay comments in `Source/Engine/Services/Common/FrameManagementServiceImpl_FrameQueries.cpp`. 39 lines removed, 4 inserted (two trimmed WHY one-liners replace multi-paragraph essays). Deleted: TASK-213 CL A/B design-history headers above `IsSteadyState` and `GetSteadyStateRelativeFrameCount`, multi-paragraph K=3 derivation citing a `Build/captures/...` log path (path will rot), 120-frame budget essay paraphrasing the constant name, "flap-back log" WHAT-restatement essay, "Update the rolling TLAS-stability counter" WHAT-restatement, "120-frame timeout watchdog (R2)" WHAT-restatement, TASK-213 CL B snapshot-rationale essay. Kept (trimmed to one-liner): K=3 invariant note about TLAS-rebuild gaps (concrete WHY for the magic number); harness-grep load-bearing warning on the "Auto-test: steady state reached at frame=" log string (external consumer reads it). No #include orphaned (every header still referenced by live code). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile`); `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran to `Engine has been terminated.`; load-bearing log line `Auto-test: steady state reached at frame=4` still fires. AC #3 keeps the task open as a rolling tracker.

2026-05-14 — Iteration #5 CL: prune essay comments in `Source/Engine/Engine_Run.cpp`. 15 lines removed, 3 inserted (one 3-line WHY one-liner replaces a 16-line essay). Previously attempted `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp`; aborted per workspace-hygiene line 14 (no AI edits to `Source/Engine/ThirdParty/`); diff reverted before survey. Surveyed in-scope candidates (excluding ThirdParty, in-flight rendering subtrees, and prior-iteration files): `Engine_Run.cpp` (15/97, 15.5%), `Engine_Terminate.cpp` (13/139, 9.4%), `Engine_ParseInitConfig_Helpers.cpp` (16/193, 8.3%), `FrameManagementServiceImpl.cpp` (19/246, 7.7%), `PerFrameDataService.cpp` (21/322, 6.5%). Picked `Engine_Run.cpp` — single concentrated 16-line essay block above the `isBakeMode` branch held the entire file's essay-violation mass. Deleted: 16-line block mixing bake-mode WHAT-restatement ("headless, one-shot import-then-exit"), TASK-68 and TASK-70 references, multi-paragraph cross-file design narration of `ImportSync` / `ProcessAssimpScene` / `TaskScheduler` worker behaviour, and a cross-file `AssimpWrapper::Import` thread-safety footnote (belongs at AssimpWrapper's API site, not at a caller). Kept (trimmed to 3-line WHY): the non-obvious correctness reason for using `std::thread` instead of `TaskScheduler` for the outer file loop — a worker submitting sub-tasks to its own thread would deadlock on `Thread::Busy`. No #include orphaned (`<thread>`, `<chrono>`, `AssetService.h`, `LogService.h`, `Engine_Internal.h` all still referenced by live code). Build green (`Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`) — Common.lib, Engine.lib, RenderTest.exe all linked. `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (run from `Bin/RelWithDebInfo`) ran to `Engine has been terminated.`. AC #3 keeps the task open as a rolling tracker.

## Review (code-impl, 2026-05-14)

Verdict (iteration #6): PASS (two ADVISORY notes, non-blocking).

Evidence:
- Diff is comment-only: `git diff HEAD -- Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — every added/removed non-blank, non-header line begins with `//`. Numstat `9 insertions / 20 deletions` matches implementer's claim. No code, whitespace-significant, or `#if` reflow.
- Path check: file is under `Source/Engine/Services/Common/`, not `Source/Engine/ThirdParty/` nor `Source/External/`. Workspace-hygiene clean.
- File size: `wc -l` reports 235 lines (implementer claimed 236; off-by-one but under the 300-line ratchet either way).
- `#include` orphan check: 10 `Log()` calls remain (LogService.h + LogServiceSpecialization.h still needed); SceneService used at L144, all service headers referenced by live code, Engine.h for g_Engine. No header orphaned.
- Subject length: 72 visible characters (em-dash 1 char, 3 bytes UTF-8). Within ≤72 bound.
- Commit type `chore(engine):` correct (not `refactor`); `[task-stays-open]` sentinel present; `Code-AI-Generated-By` / `Message-AI-Generated-By` / `Co-Authored-By` present; `Reviewed-By:` deferred to main session as briefed.
- Kept-comment accuracy verified site by site:
  - **BeginFrame/HasGPUError ordering** (L129–130): `BeginFrame()` calls `WaitOnCPU` on the per-queue fence values stored for this slot (`DX12FrameManagementService_Frame.cpp:22–24`); `HasGPUError()` at `FrameManagementServiceImpl.cpp:133` must observe a GPU that has caught up to prior work. Ordering invariant is non-obvious from `BeginFrame()` + `HasGPUError()` adjacency — kept correctly. Discipline `comment-discipline` § "Hidden invariant".
  - **Error-path CPU-callback rationale** (L142–143): traced the auto-termination signal chain — `m_UploadHeapPreparationCallback` (set in `Engine_RenderingCallbacks.cpp:23`) invokes `m_pImpl->m_LogicClient->Update()`; `WorldSystem::Update` (`World.inl:268–304`) increments `m_AutoFrameCount` and calls `IWindowService::Terminate()` on hitting `totalFrames`. Without the error-path callback invocation at L145, the GPU-error path would never advance auto-frame count or trigger termination. Rationale is concrete and load-bearing — kept correctly.
  - **ResolveGpuTimers dual ordering invariant** (L177–179): verified against `DX12GraphicsHardwareService_GpuTimers_Resolve.cpp` — section (1) at L21–93 resolves THIS frame's queries (requires post-submit so queries are in flight); section (2) at L95–148 reads back the readback buffer from `m_TimerResolveFrameCounter - GPU_TIMER_READBACK_FRAME_LATENCY` frames ago (requires BeginFrame's WaitOnCPU on that older slot's fence to make the map call safe without a sync stall). The kept comment captures both halves of the invariant correctly.
  - **`(void)IsSteadyState()` side-effect** (L213–215): `IsSteadyState()` in `FrameManagementServiceImpl_FrameQueries.cpp:38–99` mutates `m_TLASStableFrameCount`, `m_LastObservedInstanceCount`, `m_SteadyStateMarkerLogged`, `m_FirstSteadyStateFrame`, fires "Auto-test: steady state reached at frame=" (consumed by external capture-harness, per iteration #2 kept-note) and "Auto-test: steady state NOT reached" timeout log. Call is genuinely load-bearing for side effects; `(void)` cast at call site is intentional. Kept comment correctly flags the discard.
- Each deletion verified against `comment-discipline`:
  - **m_GlobalSemaphore cross-file footnote** at pre-image L48–49: described creation in DX12 backend's CreateSyncPrimitives from the engine-side caller — classic cross-file footnote. `comment-discipline` § "Cross-reference footnotes" — deletion correct.
  - **PreFrameCallback / PostFrameCallback contract-paraphrases** at pre-image L129–131 and L213–215: contract belongs at the callback's declaration. `comment-discipline` § "Multi-line essays paraphrasing the function signature" — deletion correct.
  - **TASK-140 ResolveGpuTimers essay**: distilled into kept 3-line dual-invariant WHY one-liner. The TASK-140 tombstone itself is `comment-discipline` § "Task IDs, commit hashes" — deletion correct. The substantive ordering content survives in the kept form.
  - **TASK-213 CL A `(void)IsSteadyState()` essay**: the deleted "Result is intentionally discarded this CL — CL B/C/D consume it" is `comment-discipline` § "Design history narration"; the TASK-213 CL A reference is `comment-discipline` § "Task IDs, commit hashes". The load-bearing side-effect WHY survives in the kept form.
- Sibling-file precedent: iteration #2 commit `c0d068d2` on `FrameManagementServiceImpl_FrameQueries.cpp` followed the same pattern (delete TASK-N tombstones, delete essays, keep trimmed one-liner WHYs for K=3 magic number and the harness-grep log-string contract). No rules-of-the-game change between iterations.
- Task-file note shape matches iterations #1–#5 (date, file, line-delta, deletion categories, kept-and-why with neighbour invariants spelled out, build line, Main.exe transcript line, AC #3 statement). Survey of candidate pool documented.
- Status stays In Progress per AC #3 ground rule (rolling tracker).

ADVISORY 1 (non-blocking — do not fold in):
- The deleted m_GlobalSemaphore cross-file footnote claimed the contract "belongs at the DX12 backend's CreateSyncPrimitives site." Inspected `DX12GraphicsHardwareService_Hardware_Pipeline.cpp:85–89` — the site creates the semaphore and calls `m_FrameManagementService->SetGlobalSemaphore(...)`, but carries no explanatory comment about being the canonical creation point or the FMS-side intentional absence. The deletion is discipline-correct (cross-file footnote at the consumer); the *missing* WHY at the canonical site is a pre-existing gap. Same shape as iteration #5's AssimpWrapper observation. Per `surface-dont-chase`: surface only, do not fold in.
- ADVISORY 2 (wording, non-blocking): the kept `(void)IsSteadyState()` comment says "the boolean is consumed by IsSteadyState() callers" — this is a forward reference to other call sites that read the return value, but at this call site the return IS discarded. Reader has to know the function has side-effects to parse the comment. A tighter wording would name the side-effects directly (e.g. "advances the K=3 rolling window and fires the 'steady state reached' log marker"). Wording is technically accurate; not a discipline violation. Not worth a re-spin.

Disciplines consulted: `comment-discipline`, `workspace-hygiene`, `fundamentals`, `safety-observability` (no log/assertion neighbour removed — verified by grep of `Log(` count, 10 calls remain), `cpp-style` (no naming/formatting touched), `threading-contracts` (re: m_GlobalSemaphore semaphore contract venue), `surface-dont-chase`.

## Review (code-impl, 2026-05-14) — iteration #5

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

## Review (code-review, 2026-05-21) — iteration #8 (Engine/Common sweep, commit `dfc9553c`)

**Verdict: ADVISORY** (one non-blocking finding + four surface observations). Ship as-is.

### Per-file accuracy: OK

6 touched files, kept-vs-deleted walks all check against `comment-discipline`. BCCompression.h (side-effect WHY only), DoubleBuffer.h (single-producer threading contract only — see Finding 1), Enum.h (using-alias WHY only), GPUDataStructure.h (luminance.w semantics, exposureMode legend, autoExposure unit, pointShadowBypass toggle, trimmed alignment WHY), Thread.h (Failed-state semantic, GetCaughtExceptionCount semantic), Thread.cpp (outer-try scope choice + Failed-transition consequence).

File-size ratchet clean (all 6 files under 300). Numstat matches commit (29 ins / 105 del). Commit-message policy clean (type+scope, [task-stays-open], attribution footers). Pattern continuity vs iteration #2 commit `c0d068d2`.

### Finding 1 (ADVISORY, non-blocking) — `DoubleBuffer.h:Flip()` stealth brace removal

Deleting the `// Busy-wait spin` comment incidentally dropped the Allman braces around the single-statement `std::this_thread::yield()` body. Behaviour-equivalent, but `cpp-style` convention favours braces (other `Thread.cpp` busy-wait loops use `{ ... }` even for single statements). Commit message subtly overclaims "comment-only." Resolution: restore braces in the next iteration touching `DoubleBuffer.h`, or accept as-is.

### Observation 1 — `DoubleBuffer.h` Write→Flip publishing contract undocumented

Single-producer contract on `Write` is documented; the Write→Flip ordering invariant (readers see new buffer only after Flip returns) is not. Pre-existing gap; surface for the next iteration.

### Observation 2 — `GPUDataStructure.h:188` `padding[16]` magic without alignment hint

The deleted "Adjusted padding after adding shader fields" essay was discipline-correct (design history) but its trim removed the only hint that the value is contract-dependent. Pre-existing concern; the gap is the lack of a hidden-invariant comment, not a violation of the discipline trim.

### Observation 3 — `GPUDataStructure.h:204/218` retained `Surfel` / `SurfelGrid` one-liners

Arguably WHAT-paraphrases of the type names; not in iteration #8's stated essay-violation scope. Surface for future iteration.

### Observation 4 — `Thread.h:32-34` Failed-state contract is inaccurate (pre-existing)

Kept comment says "Failed threads stop accepting new tasks." `Thread::AddTask` (`Source/Engine/Common/Thread.cpp:60-107`) has no `Failed` check; the CAS at line 74 (`expected = Idle`) and line 87 (`expected = Waiting`) will fail when the actual state is `Failed`, causing AddTask to spin-loop until the 5000ms timeout warning. Pre-image carried the same inaccuracy; this CL inherited it. Worth filing as a separate code-correctness task against `Thread::AddTask`.

### Resolution

No re-spin. Finding 1 (brace restore) folded into next iteration touching `DoubleBuffer.h`. Observation 4 worth filing as a separate `Thread::AddTask` Failed-state task. Observations 1–3 surface-only.

2026-05-21 — Iteration #9 (parallel fan-out): swept four non-overlapping scopes simultaneously via background code-impl agents. Plus a `DoubleBuffer::Flip` Allman-brace restore tightener from iteration #8 peer review (Finding 1 against commit `dfc9553c`). 113 files edited, 458 ins / 1793 del, net −1335 lines.

**Sub-scope A** (Engine root + Component + Interface + Export + RayTracer + Platform): 18 files, −163 lines. Excluded `Engine_Run.cpp` / `Engine_Terminate.cpp` / `Engine_ParseInitConfig.cpp` (iterations #5/#7/TASK-236).
**Sub-scope B** (Services/DX12 + Services/MT): 29 files, −195 lines.
**Sub-scope C** (Services top-level + Services/Common + Services/VK): 20 files, −234 lines. Excluded `SceneService.cpp` / `FrameManagementServiceImpl.cpp` / `FrameManagementServiceImpl_FrameQueries.cpp` (iterations #3/#6/#2). `PerFrameDataService.cpp` sub-scope-C trim **reverted** — file still at 306 lines after −16 trim, over the 300-line ratchet; folds into TASK-219 when split.
**Sub-scope D** (ExampleProject + TestSuite + TestClient + Tool): 45 files, −795 lines. Excluded `ExampleRenderingClient_{AuditDump, Setup, ExecuteCommands}.cpp` + `ExampleRenderingClient_Internal.h` (just touched this session for TASK-233/235/236) and `TestSuite/Removed/`.
**Brace restore**: `DoubleBuffer.h::Flip()` loop body wrapped in `{}` per cpp-style Allman convention (iteration #8 peer-review Finding 1).

Deletion themes: TASK-N tombstones (~25 distinct task IDs); incident-narrative essays; WHAT-restatement banners; cross-file footnotes pointing to DX12/VK/HLSL contracts that belong at the source-of-truth; signature paraphrases; commented-out dead-code (Reflector helper stubs, VK timeline-semaphore Present path, JSON serializer skeleton helpers); stale `@TODO` graveyard with no tracked task.

Kept-as-distilled-WHY themes: hidden invariants (MSVC SRW re-entry, m_Bypassed cross-thread dispatch contract, m_pImpl macro-expansion site requirement, DX12 GBV ResolveQueryData reject reason, raygen binding-count vs HLSL register decl mirror, NRD raw-D3D12 passthrough rationale); paper citations (Capsaicin GI-1.0 §2.4.2/2.4.3/2.1.5, Ramamoorthi–Hanrahan SH, AgX K=6.0); load-bearing magic-number rationales (K=25 audit settle, K=3 TLAS stability window, 256-per-queue GPU timer ceiling); cross-queue / cross-thread contracts; ownership/lifetime invariants; paper-port site-mirror constraints.

**Build green** (`Scripts/BuildWin.ps1 -SkipShaderCompile`, 21:09 timestamp on relinked `Main.exe` + `RenderTest.exe`). **Audit autotest green** (`Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 60 -offscreen audit`, 19:10:35–19:10:41 transcript): UnitTest scene-load + GISponza scene-load callbacks fire, AuditDump complete, all 11 HDRs written.

Iteration #9 (continued) — held-back files and surfaced findings.

**Files held back (over 300 lines, TASK-219 territory)**: `DrawCallService.cpp` (316), `DX12Context.cpp` (305), `Player.inl` (417), `World.inl` (406), `TweakRegistry.inl` (322), `GIDenoisePass.cpp` (405), `LightPass.cpp` (399), `ExampleRenderingClient.cpp` (318), `LightCullingPass.cpp` (309), `TestRenderingClient.cpp` (320), `PerFrameDataService.cpp` (306 post-trim, 322 pre-trim — revert kept the file out of this CL). `GraphicsPrimitive.h` (381) also remains held back from iteration #8.

**Surfaced findings (NOT folded in)**:
- VK backend Present path is non-functional (entire submit-and-present logic was dead-code-commented out).
- VK `GetIndex` unconditionally returns `std::nullopt` (texture bindless not implemented).
- VK `UploadToGPU` ignores `commandList` parameter (cross-frame upload ordering undefined).
- DX12 `CaptureCallstack` uses raw `malloc`/`free` instead of engine Memory service (`cpp-style` § no-raw-equivalents).
- DX12 `m_BeginCapture`/`m_EndCapture` members appear dead (no writers found).
- `Thread.h:32-34` `Failed`-state comment claims AddTask refuses, but AddTask spin-loops on Failed (iter #8 peer-review Observation 4 — worth filing as separate task against `Thread::AddTask`).
- Empty `Engine::ResolveDependencies` body (function deletion candidate).
- Ad-hoc `singletons_[type_index]=ptr` writes in `Engine_CreateServices.cpp` bypass the documented thread-safe insert path.
- Broken Linux platform stub at `LinuxWindowService.cpp:89`.
- Declared-but-unused `Engine.h::testCase` field and `CreateServices(extraHook)` parameter.
- `MTGraphicsService.h` missing-`../IGraphicsService.h` include (Metal backend bit-rot, pre-existing).
- Stale TODOs deleted in DX12 + ExampleProject mask real incomplete features: swap-effect FLIP_SEQUENTIAL, blend-op separate-alpha-vs-RGB, NRDDenoisePass shader-stage selection in VK `TryToTransitState`.
- Naming-suffix inconsistency in `RadianceCacheRaytracingPass.cpp:148` (no `/Compute` qualifier where peers use it).
- Nullptr-binding pattern in `GPUPathTracerPass_Dispatch.cpp:69-70` now lacks the WHY comment (intentional, but invisible after the sweep — candidate for future iteration to add a hidden-invariant comment).

AC #3 keeps the task open as a rolling tracker.

## Review (code-review, 2026-05-21) — iteration #9 (parallel sweep, commit `395c00fa`)

**Verdict: PASS.** Ship as-is. Three non-blocking observations.

### Coverage

Reviewer sampled ≈22 of 113 files in detail (≈19%) plus a 158-line non-comment filtered diff across all 113. Sub-scope sampling (≥2 files each): A — Engine.h / LightComponent.h / RayTracer.cpp / WinMain.cpp / WinWindowService.cpp; B — DX12GraphicsHardwareService.h / _Internal.h; C — AssetService.h / EditorService.cpp / EditorService.h / EditorService_Introspection.cpp / RenderPassResourceServiceImpl.cpp / VKGraphicsService.cpp / _CommandList.cpp / _EngineComponent.cpp / LinuxWindowService.cpp; D — GPUPathTracerPass_BindingLayout.cpp / _Dispatch.cpp / NRDIntegrationAdapter.h / NRDConstants.h / TaskSystemTests.cpp / AtomicTests.cpp / FixedSizeStringTests.cpp / Reflector.cpp; tightener — DoubleBuffer.h.

Stealth-pattern scan exhaustive: every non-comment non-whitespace diff line accounted for as intentional brace restoration, removal of empty loop/branch body (Reflector.cpp, VKGraphicsService_CommandList.cpp), or removal of commented-out dead-code block. File-size ratchet clean (all 113 files ≤300). Build artifacts on disk match cited times (Main.exe + RenderTest.exe 21:09:34/59 PM; 11 audit HDRs at 21:10:40–41 PM — commit cited 19:10 in UTC offset). Surfaced findings spot-checked: MTGraphicsService.h missing-include bit-rot, Engine.h::testCase / extraHook / ResolveDependencies still in tree, Engine_CreateServices.cpp ad-hoc singletons_[type_index]=ptr writes still in tree, VKGraphicsService.cpp PresentImpl still non-functional. All honest — NOT folded in.

### Non-blocking observations

1. **WinWindowService.cpp:73-78** — inline WIN32 CreateWindow arg annotations deleted. Clean call (WHAT-paraphrases of the signature; MSDN is the source of truth for `CW_USEDEFAULT` semantics).
2. **DX12GraphicsHardwareService.h:11-13** — asymmetric kept-WHY on `DX12GpuTimerSlot`. `m_SlotIndex` kept the slot-arithmetic invariant; `m_BeginRecorded` lost "Set by BeginGpuTimer; cleared by EndGpuTimer. Detects unmatched calls." (state-machine invariant; non-obvious). Inconsistent application but not blocking — candidate for restore in a future iteration touching this file.
3. **Bookkeeping discrepancy in commit body** — body says "458 ins / 1793 del" but actual numstat is `494 ins / 1793 del`. The 36-line difference is the backlog .md note (71 ins) minus an apparent re-count after PerFrameDataService.cpp revert. Trivial; not worth an amend.
4. **GPUPathTracerPass_Dispatch.cpp:64-65,70** — three `nullptr` binding-slot args without inline WHY. Pre-existing (the pre-image had only an above-block essay, no per-slot comment). Implementer surfaced as a follow-up candidate. Honest.

### Resolution

No re-spin. Observation 2 (m_BeginRecorded hidden-invariant restore) and observation 4 (nullptr binding-slot WHY) folded into next iteration touching those files. Bookkeeping discrepancy not worth amending.
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
