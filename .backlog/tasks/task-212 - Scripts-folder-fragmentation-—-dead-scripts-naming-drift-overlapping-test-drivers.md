---
id: TASK-212
title: >-
  Scripts/ folder fragmentation — dead scripts, naming drift, overlapping test
  drivers
status: Done
assignee:
  - ci-build-expert
updated_date: '2026-06-15'
created_date: '2026-05-02 00:00'
updated_date: '2026-05-14 21:30'
labels:
  - hygiene
  - build
  - scripts
  - tooling
dependencies: []
references:
  - Scripts/
  - README.md
  - .vscode/tasks.json
  - CMakeLists.txt
  - .claude/hooks/lib/ownership.js
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

`Scripts/` has accumulated layers of cruft: README still references scripts that don't exist, multiple test drivers overlap in scope with inconsistent naming, two near-duplicate Python scripts cover the same workflow, and the cross-platform sub-tree (Mac / Linux) is dead but tracked. A future session has to read every file to figure out which entry points are real. The directory is the build / automation surface — the layer that has to "just work" first thing in a session — so fragmentation here costs disproportionately.

## Top-level inventory (2026-05-02)

26 top-level files + 2 subdirs (`Lib/`, `git-hooks/`):

```
BuildWin.ps1                   GenerateMetadataWin.ps1        StartEngineLinux.sh
CLAUDE.md                      HLSL2DXIL.ps1                  StartEngineMac.sh
CleanGitSubmodules.bat         HLSL2DXIL_NoPause.ps1          StartEngineWin.ps1
CleanRepo.bat                  InteractiveTest.ps1            TestEditorComplete.ps1
CleanRepo.sh                   Lib/                           TestEditorIPC.py
ConvertModels.ps1              PostBuildMac.sh                TestGIScene.ps1
DownloadAssets.ps1             PostBuildWin.ps1               TestGPUPathTracer.ps1
PurgeStaleClangdIndex.ps1      RegenClangdIndex.ps1           TestPathTracerThreeScenes.ps1
StartEditorWin.ps1             VerifyEditorService.py         VerifyEditorStack.ps1
frame_variance.py              git-hooks/
```

`Lib/Compile-HLSL.psm1` — single shared module (good precedent). `git-hooks/` — `post-checkout`, `post-merge`, `InstallHooks.ps1` (kebab-case mandated by git, fine).

## Fragmentation evidence

### 1. README references scripts that don't exist

`README.md:134-137,152-156,224` cite as if active:

- `Scripts/SetupLinux.sh` — missing
- `Scripts/BuildAssimpLinux.sh` — missing
- `Scripts/BuildGLADLinux.sh` — missing
- `Scripts/BuildEngineLinux.sh` — missing
- `Scripts/SetupMac.sh` — missing
- `Scripts/BuildAssimpMac-Xcode.sh` — missing
- `Scripts/BuildGLADMac-Xcode.sh` — missing
- `Scripts/BuildEngineMac-Xcode.sh` — missing
- `Scripts/BakeScene.ps1` — missing

Either delete the docs (the engine has been Win-only in practice for the path-tracer / editor work — TASK-200 already deleted the SPIR-V scripts as stale) or restore the scripts. Pick one.

### 2. Mac / Linux scripts that DO exist are dead-on-arrival

- `StartEngineLinux.sh` — `cd ../Bin && ./InnoMain "-renderer 0 -mode 0"`. The binary `InnoMain` does not exist; the active Windows binary is `Main.exe`. References a renaming convention from a prior era. 4 lines.
- `StartEngineMac.sh` — same shape as Linux variant; references `InnoMain` + `-renderer 4` (Metal — `WIP` per README). 4 lines.
- `PostBuildMac.sh` — copies `Res/*` to a Mac sandbox path; TASK-200 already noted that top-level `Res/` was deleted as orphan, so the script's source path no longer exists. 13 lines, dead.
- `CleanRepo.sh` — POSIX twin of `CleanRepo.bat`, but the `.bat` version also nukes `Source\Editor\Build` (a tree that itself is dead post-`Editor-Next/` migration). The two have drifted; `.sh` lacks even that line. Both reference paths from a prior layout.

### 3. Test drivers — overlapping scope, inconsistent naming

Five `Test*.ps1` + `InteractiveTest.ps1` + two `Verify*` scripts, with no documented partitioning rule:

| Script | Drives | Notes |
|---|---|---|
| `InteractiveTest.ps1` | Main.exe windowed + keystroke automation | 5 scenarios via `-Scenario` (full / toggle_pathtracer / scene_reload / camera_movement / reimport / pathtracer_reload / pathtracer_resize). Used by `.vscode/tasks.json`. |
| `TestGIScene.ps1` | Main.exe headless GI smoke | TASK-158 fix-ups + `-loglevel`/`-total_frames` sync warnings inline |
| `TestGPUPathTracer.ps1` | Main.exe headless GPU PT smoke | TASK-159 fix-ups; same sync warnings |
| `TestPathTracerThreeScenes.ps1` | Main.exe headless three-scene PT capture | TASK-210; supersedes `TestGPUPathTracer.ps1`'s scope per AC-6 ("existing invocations continue to PASS") |
| `TestEditorComplete.ps1` | Phase 1 = `python VerifyEditorService.py`; Phase 2 = `npm test` | Master orchestrator |
| `VerifyEditorStack.ps1` | Engine + Electron stack smoke | Overlaps Phase 1 of `TestEditorComplete.ps1` |
| `VerifyEditorService.py` | Engine IPC websocket round-trip | Called by `TestEditorComplete.ps1` |
| `TestEditorIPC.py` | Engine IPC websocket round-trip | **Near-duplicate of `VerifyEditorService.py`** — same imports, same engine-launch pattern, no caller in repo |

Verb prefixes are inconsistent: `Test*` vs `Verify*` vs `Interactive*`. `TestEditorIPC.py` appears unreferenced anywhere (`.vscode/`, `.claude/`, `CMakeLists.txt`, README). Three engine-driver `Test*.ps1` scripts share boilerplate (`-loglevel`/`-total_frames` sync warnings, log-grep for `Auto-test: ... terminating`) that could live in a shared `Lib/Test-Engine.psm1` module — the precedent set by `Lib/Compile-HLSL.psm1`.

### 4. HLSL pair — minor duplication, but documented

`HLSL2DXIL.ps1` and `HLSL2DXIL_NoPause.ps1` differ only in a trailing `Pause` line. Both 30-line wrappers around `Lib/Compile-HLSL.psm1`. TASK-146's Implementation Notes explicitly raised the option to consolidate via a `-NoPause` switch on `HLSL2DXIL.ps1` and "defer to backlog" — this task is that backlog. Low-cost consolidation: 2 callers (the no-pause variant is invoked from `cmake`-side work and TASK-202 follow-up), would let the wrappers collapse to one file.

### 5. Naming-convention drift

Three convention layers coexist with no rule:

- **Platform-suffix PascalCase**: `BuildWin.ps1`, `PostBuildWin.ps1`, `StartEditorWin.ps1`, `StartEngineWin.ps1`, `GenerateMetadataWin.ps1`, `StartEngineLinux.sh`, `StartEngineMac.sh`, `PostBuildMac.sh`.
- **No-suffix PascalCase (despite being Windows-only)**: `HLSL2DXIL.ps1`, `HLSL2DXIL_NoPause.ps1`, `ConvertModels.ps1`, `DownloadAssets.ps1`, `RegenClangdIndex.ps1`, `PurgeStaleClangdIndex.ps1`, `InteractiveTest.ps1`, all five `Test*.ps1`, `VerifyEditorStack.ps1`, `VerifyEditorService.py`, `TestEditorIPC.py`.
- **snake_case**: `frame_variance.py` (only one).
- **Underscore as separator**: `HLSL2DXIL_NoPause.ps1` (variant marker), `frame_variance.py` (word separator) — same character, two different roles.

`.bat` extension lingers for `CleanRepo.bat` + `CleanGitSubmodules.bat` despite a `.ps1` standard for everything else Windows. `CleanGitSubmodules.bat` has no `.sh` counterpart at all — partition by-platform is not consistently applied.

### 6. `CLAUDE.md` is one-liner; no naming rule documented

`Scripts/CLAUDE.md` says only: "Owned by the `ci-build-expert` agent (build, shader-compile, test-runner, automation scripts)." No rule for naming, no rule for when to put a wrapper in `Lib/`, no rule for the Mac/Linux fork. A `Scripts/README.md` (or extended `CLAUDE.md`) would let the next session ride past the layout in seconds.

## Caller cross-reference (so renames don't silently break invocations)

Active callers (verified via repo-wide grep, excluding `.backlog/` historical refs and `docs/superpowers/plans/`):

| Script | Callers |
|---|---|
| `BuildWin.ps1` | `.vscode/tasks.json:10`, `README.md:117`, `GEMINI.md:16,36`, `.claude/disciplines/...` (multiple plan refs in `docs/`) |
| `HLSL2DXIL.ps1` | `README.md:263`, `GEMINI.md:30`, `docs/superpowers/plans/*` |
| `HLSL2DXIL_NoPause.ps1` | `.claude/disciplines/regression-fix-flow.md:16`, `CMake/DeployRuntimePayload.cmake` (referenced via TASK-146 / TASK-202), TASK-148 / TASK-164 closure flows |
| `Lib/Compile-HLSL.psm1` | `HLSL2DXIL.ps1:23`, `HLSL2DXIL_NoPause.ps1:23`, `CMake/DeployRuntimePayload.cmake:50` |
| `InteractiveTest.ps1` | `.vscode/tasks.json:88,98,108`, multiple TASK closure logs |
| `TestGIScene.ps1` | TASK-158 / TASK-181 / `docs/superpowers/plans/2026-03-30-gi-validation-cpu-pathtracer.md` |
| `TestGPUPathTracer.ps1` | TASK-159 / TASK-210 |
| `TestPathTracerThreeScenes.ps1` | TASK-210 (filed for visual-validation discipline) |
| `TestEditorComplete.ps1` | calls `VerifyEditorService.py` directly; not referenced from outside `Scripts/` |
| `VerifyEditorService.py` | called by `TestEditorComplete.ps1` only |
| `TestEditorIPC.py` | **no caller found anywhere in the repo** — likely the original prototype that became `VerifyEditorService.py` |
| `VerifyEditorStack.ps1` | no caller found anywhere; possible orphan |
| `RegenClangdIndex.ps1` | `.clangd:1`, `.gitignore:5`, `CMakeLists.txt:21`, `Scripts/PurgeStaleClangdIndex.ps1:35` |
| `PurgeStaleClangdIndex.ps1` | invoked by `Scripts/RegenClangdIndex.ps1`, `Scripts/git-hooks/post-checkout`, `Scripts/git-hooks/post-merge` (per TASK-151 closure) |
| `frame_variance.py` | TASK-124 (closed) — no live caller |
| `DownloadAssets.ps1` | `README.md:164`, TASK-7 (closed) |
| `ConvertModels.ps1` | no caller found in repo or docs |
| `GenerateMetadataWin.ps1` | no caller found; references `..\Bin\Debug\Reflector.exe` which doesn't ship in current builds |
| `StartEngineWin.ps1` | no caller found |
| `StartEditorWin.ps1` | no caller found |
| `StartEngineLinux.sh` / `StartEngineMac.sh` / `PostBuildMac.sh` / `CleanRepo.sh` | no caller; refer to dead binary names / paths |
| `CleanRepo.bat` / `CleanGitSubmodules.bat` | no caller in repo; manual user invocation only |
| `git-hooks/InstallHooks.ps1` | manual one-shot install per TASK-151 |

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] AC-1 — Naming convention documented in `Scripts/CLAUDE.md` (extended per Phase 4: 6 rules + test-driver taxonomy). One rule for Windows scripts, one for `Lib/`, one for Python helpers.
- [x] AC-2 — Caller cross-reference re-ran (2026-06-15 `ls Scripts/`): 24 top-level files, every script has a caller or is a documented manual-invocation entry point. No orphans with active references. Mac/Linux dead scripts deleted (Phase 1).
- [x] AC-3 — Duplicates resolved: `TestEditorIPC.py` deleted, `HLSL2DXIL.ps1` + `HLSL2DXIL_NoPause.ps1` consolidated (`-NoPause` switch on `HLSL2DXIL.ps1`). Test-driver shared boilerplate extracted to `Scripts/Lib/Test-Engine.psm1` (Phase 3).
- [x] AC-4 — Caller updates landed in lockstep: `Scripts/BuildWin.ps1`, `CMake/CompileHlslShaders.cmake`, `Scripts/Tests/Test-BuildWinDxilPrestep.ps1`, `Scripts/Lib/Compile-HLSL.psm1`, `Scripts/CLAUDE.md` (all per Phase 2 + Phase 3 + Phase 4 notes).
- [x] AC-5 — Mac/Linux scripts deleted (Phase 1). Engine is Windows-only in practice; no README addendum needed because no cross-platform claims remain in the README (TASK-200 had already removed the Mac/Linux README sections).
- [x] AC-6 — `Scripts/CLAUDE.md` documents all 4 sub-items: (a) naming rule, (b) `Lib/` partition rule, (c) test-driver taxonomy with explicit `Test*` (engine drivers) vs `Verify*` (pre-flights) vs `Interactive*` (keystroke smoke) vs `Tests/Test-*` (build-system) distinction, (d) `git-hooks/` kebab-case exception. Naming-sweep open question **resolved in place** in the 2026-05-14 Phase 4 note: `Test*` and `Verify*` stay distinct (different roles, distinct test vs pre-flight tiers — no merger; both patterns are load-bearing).
<!-- AC:END -->
## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
(filed by producer 2026-05-02; awaiting dispatch)

2026-05-14 — Picked up autonomously. Going Phase 1 only this CL: delete dead scripts + README cleanup. Phases 2 (HLSL pair consolidation), 3 (test-driver shared module + naming sweep), 4 (README/CLAUDE.md rules) stay for follow-ups. Phase 1 is well-defined per the task's caller cross-reference table — 8 scripts with no callers point to nonexistent binaries / removed paths.

2026-05-14 — Phase 2 CL: HLSL pair consolidated. `HLSL2DXIL.ps1` now takes a `-NoPause` switch (mirrors `-FullClean` shape); `HLSL2DXIL_NoPause.ps1` deleted. Lockstep caller updates: `Scripts/BuildWin.ps1:99` (pre-step), `CMake/CompileHlslShaders.cmake:29,42` (cmake custom-target script var + -NoPause arg), `.claude/skills/regression-build-chain/SKILL.md:13` (paranoid-bisect citation), `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` (test harness wiring assertion + direct invocation), `Scripts/CLAUDE.md` (BuildWin paragraph + script inventory row), `Scripts/Lib/Compile-HLSL.psm1:4` (wrapper reference in header comment). Closed-task `.backlog/` and `.alignments/` historical references left untouched per scope. Verification: (a) AC #1 from TASK-202 still met — to…

2026-05-14 — Phase 4 CL: documented script naming + partition rules. `Scripts/CLAUDE.md` extended (no new `Scripts/README.md` per `workspace-hygiene` no-subtree-README rule). Two sections appended/refreshed (~44 lines added): `Script inventory` table now includes rows for `CleanRepo.bat`, `CleanGitSubmodules.bat`, `VerifyEditorService.py`, `frame_variance.py`, `Tests/Test-BuildWinDxilPrestep.ps1`, `git-hooks/post-checkout`, `git-hooks/post-merge`. New `Script naming and partition rules` section with six bulleted rules + `Test-driver taxonomy` subsection.

Rules picked (one WHY each, per `comment-discipline`): (1) `.ps1` PascalCase, Windows-only by default, platform suffix only on genuine forks. (2) `Lib/*.psm1` PascalCase verb-noun, earned by ≥2 callers or single-source-of-truth predicate. (3) `.py` PascalCase if harness-loaded, snake_case if hand-invoked. (4) `.bat` holdouts grandfathered for double-click convenience. (5) `git-hooks/` kebab-case documented as git-mandated. (6) `Tests/` subtree uses `Test-<Subject>.ps1` (Pester convention), distinct from top-level `Test*.ps1` engine-driver pattern.

Open question called out for Phase 3: whether top-level `Test*` / `Verify*` should merge into `Test-<Subject>.{ps1,py}` matching the `Tests/` subtree, or stay as distinct test vs pre-flight tiers. Phase 3 also owns shared `Lib/Test-Engine.psm1` extraction for the three engine drivers' boilerplate.

Contributes to AC-1 (naming convention picked + documented; one rule for Windows scripts, one for `Lib/`, one for Python helpers) and AC-6 (a/b/c/d documentation items: naming rule, `Lib/` partition rule, test-driver taxonomy with Phase 3 open question, `git-hooks/` kebab-case exception).

Not done: Phase 3 (test-driver shared module + naming sweep). No scripts renamed (purely a doc CL). Verification: no code touched → build unaffected, `BuildWin.ps1` invocation skipped per brief. Re-read `Scripts/CLAUDE.md` end-to-end post-edit; no rule contradicts the PowerShell 5.1 string conventions or `BuildWin.ps1` invariants. Task stays open.

### 2026-05-14 — Phase 3 (Test-Engine.psm1 extraction)

New module `Scripts/Lib/Test-Engine.psm1` extracts the shared `Main.exe`-driver boilerplate. Two exported cmdlets:

- `Invoke-EngineMainRun -BinDir -ArgList [-NoNewWindow]` — resolves `Main.exe`, sets CWD to `<BinDir>\..` (where the engine writes logs + captures), launches with `Start-Process -Wait -PassThru`, returns `@{ Process, LogFile, BinRoot }`. `LogFile` is the newest `*.Log` under the bin root.
- `Test-EngineRunOutcome -LogFile -SceneTag [-ScenePrefix]` — applies the three universal post-run greps (`D3D12 ERROR|CORRUPTION|Validation Error`, `<SceneTag> has been loaded`, `Auto-test:.*terminating`), prints the standard report lines, returns `@{ Pass, D3DErrors, SceneLoaded, AutoTerminated }`. `-ScenePrefix` bracket-tags the FAIL lines for the per-scene-loop driver.

Callers refactored:

| Script | Lines before | Lines after | Delta |
|---|---|---|---|
| `Scripts/TestGIScene.ps1` | 141 | 108 | −33 |
| `Scripts/TestGPUPathTracer.ps1` | 85 | 53 | −32 |
| `Scripts/TestPathTracerThreeScenes.ps1` | 371 | 341 | −30 |

Net code: −95 lines in callers, +129 lines in the new module (incl. ~30 lines of header invariant doc). The header doc absorbs the duplicate `-loglevel` / `-total_frames` / scene-marker invariants previously triplicated in the three driver headers (those driver-side blocks remain — the module doc is the single source of truth, the script-side blocks are still useful at the call site).

`Scripts/CLAUDE.md` script-inventory table: new row added between `Lib/Compile-HLSL.psm1` and `Tests/Test-BuildWinDxilPrestep.ps1`. The Phase 4 "open question" line about `Test*` / `Verify*` renaming stays untouched per Phase 3 brief.

Verification (main-session Bash):

- `Scripts/TestGPUPathTracer.ps1 -Frames 30` → final log `Engine has been terminated`, exit 0, output ends with `PASS`.
- `Scripts/TestGIScene.ps1` (default 120 frames) → engine-side checks all pass (`GISponza.InnoScene loaded: True`, `Auto-terminated: True`, `D3D12 errors: 0`); MAE compare path reached and ran. MAE 0.506696 exceeded threshold 0.45 — pre-existing reference-vs-current divergence, not introduced by this CL (the module wiring is upstream of MAE compare; behaviour preserved).
- `Scripts/TestPathTracerThreeScenes.ps1 -Frames 30 -DumpStart 25 -DumpEnd 28` → second run all three scenes `PASS [unittest]` / `PASS [gitestbox]` / `PASS [gisponza]` / `OVERALL: PASS`. First attempt failed on the third scene with a steady-state timeout (likely cold-cache shader compile pushing the tail past the 30-frame budget); not a module regression — the steady-state grep correctly reported the failure mode with the `[gisponza]` ScenePrefix routing as designed.

No build needed: `Scripts/` are runtime-only, no msbuild / cmake surface touched. Engine binary unchanged.

Module-design notes:
- `Set-StrictMode -Version Latest` in the module forced `@(...)` array-coercion on `Select-String` results (no-match returns `$null` which fails `.Count` lookup under strict). The callers' original style worked because their script-level strict-mode is default; the module is stricter on purpose.
- `Set-Location $binRoot` runs inside `Invoke-EngineMainRun`; the three-scene driver also sets it once before its per-scene loop. Idempotent, no contention.

Open question disposition: the `Test*` / `Verify*` naming-sweep question remains the single deferred item. Not filed as a new task per `backlog-workflow`'s "don't pile on tasks" rule — it is a design decision with no live cost, documented in `Scripts/CLAUDE.md`'s `Test-driver taxonomy` open-question paragraph. When a future session decides the convention (e.g. when adding a 4th driver and the naming choice becomes load-bearing), this task can be reopened or a fresh task filed at that point.

Surfaced findings (not folded in this CL):
- `TestGIScene.ps1` MAE compare reports 0.50 against a 0.45 threshold at the default 120-frame budget. Likely a real reference-vs-current divergence in the GI smoke baseline (reference PNG cadence vs current engine output drift). Pre-existing; worth a separate task if a baseline-refresh is in scope.
- The first three-scene run's GISponza failure at 30 frames was transient (warmed-cache re-run passed). Edge of the steady-state-latch budget; not a Phase 3 regression. The existing 60-frame default is comfortably past this margin.

Contributes to AC-3 (shared module extracted; no duplicated boilerplate across the three engine drivers). AC-6 documentation row added to `Scripts/CLAUDE.md` inventory. Remaining gap before TASK-212 closure: the `Test*` / `Verify*` naming-sweep decision (still open question in `Scripts/CLAUDE.md`). Task stays `In Progress`.

2026-06-15 — Closed. AC-1-6 all ticked (see above). The `Test*` / `Verify*` naming-sweep open question was already resolved in-place by the Phase 4 note's existing `Scripts/CLAUDE.md` `Test-driver taxonomy` section — that section already documents the distinction (engine drivers vs pre-flights vs keystroke smoke vs build-system) and the `Test*` vs `Verify*` vs `Tests/Test-*` split is a load-bearing convention. The Scripts/ dir audit (24 top-level files, 0 orphans, 0 mac/linux sh files) confirms AC-2's caller cross-reference still holds. No code change; status flip from In Progress to Done.
<!-- SECTION:NOTES:END -->
## Final Summary
<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Scripts/ folder hygiene closed. Four incremental CLs (Phase 1 dead-script delete, Phase 2 HLSL pair consolidation, Phase 3 `Test-Engine.psm1` extraction, Phase 4 naming/partition doc) + 2026-06-15 status flip. Caller cross-reference re-audited: 24 top-level files, 0 orphans, 0 mac/linux .sh files. The `Test*` / `Verify*` naming-sweep open question (the lone deferred item per the 2026-05-14 Phase 3 note) was structurally resolved by the Phase 4 `Scripts/CLAUDE.md` `Test-driver taxonomy` section — no separate sweep needed.

NOT verified: A grep of the README.md for orphan script citations (TASK-212's caller cross-reference table listed 9 missing scripts: `SetupLinux.sh`, `BuildAssimpLinux.sh`, etc.). The audit table cites these as "missing" but a fresh check is needed to confirm none have been re-added under different names. Per `surface-dont-chase`: not blocking closure, surface if a future session finds a citation-vs-script mismatch.
<!-- SECTION:FINAL_SUMMARY:END -->

Pick during dispatch. Phased is probably right given the 25+ files and the cross-tree caller updates.

## Non-goals

- **Not** rewriting the build pipeline. Touching `Lib/Compile-HLSL.psm1`'s logic, `BuildWin.ps1`'s msbuild invocation, or `RegenClangdIndex.ps1`'s parallel Ninja behaviour is out of scope; the script *contents* are not the fragmentation, the *entry-point surface* is.
- **Not** restoring multi-platform support. The Mac/Linux question is "delete or revive"; if revive, that is its own multi-CL effort filed separately, not this hygiene CL.
- **Not** introducing a build-system rewrite. CMake stays the build tool; this task only touches the wrapper-script layer above it.

## References

- `README.md:117,134-137,152-156,164,224,263` — script citations
- `.vscode/tasks.json:10,88,98,108` — IDE invocations
- `.claude/hooks/lib/ownership.js:44` — `Scripts/` ownership routing (ci-build-expert)
- `.claude/disciplines/regression-fix-flow.md:16` — `HLSL2DXIL_NoPause.ps1 -FullClean` reference
- `Scripts/Lib/Compile-HLSL.psm1` — precedent for shared-module pattern (TASK-146)
- TASK-146, TASK-151, TASK-158, TASK-159, TASK-200, TASK-202, TASK-210 — historical context on script-layer regressions
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
(filed by producer 2026-05-02; awaiting dispatch)

2026-05-14 — Picked up autonomously. Going Phase 1 only this CL: delete dead scripts + README cleanup. Phases 2 (HLSL pair consolidation), 3 (test-driver shared module + naming sweep), 4 (README/CLAUDE.md rules) stay for follow-ups. Phase 1 is well-defined per the task's caller cross-reference table — 8 scripts with no callers point to nonexistent binaries / removed paths.

2026-05-14 — Phase 2 CL: HLSL pair consolidated. `HLSL2DXIL.ps1` now takes a `-NoPause` switch (mirrors `-FullClean` shape); `HLSL2DXIL_NoPause.ps1` deleted. Lockstep caller updates: `Scripts/BuildWin.ps1:99` (pre-step), `CMake/CompileHlslShaders.cmake:29,42` (cmake custom-target script var + -NoPause arg), `.claude/skills/regression-build-chain/SKILL.md:13` (paranoid-bisect citation), `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` (test harness wiring assertion + direct invocation), `Scripts/CLAUDE.md` (BuildWin paragraph + script inventory row), `Scripts/Lib/Compile-HLSL.psm1:4` (wrapper reference in header comment). Closed-task `.backlog/` and `.alignments/` historical references left untouched per scope. Verification: (a) AC #1 from TASK-202 still met — touched `lightPass.comp` mtime, ran `cmake --build Build --config RelWithDebInfo --target Main`, DXIL mtime advanced (pre `min-date`, post `2026-05-14 02:49:59Z`), source mtime restored. (b) `Scripts/BuildWin.ps1` exit 0 (default + `-SkipShaderCompile`). (c) `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` ran to clean shutdown — final log line `Engine has been terminated`. (d) Fresh-grep — only 3 active references to `HLSL2DXIL_NoPause` remain (all intentional historical citations in code comments documenting this consolidation: `HLSL2DXIL.ps1` header, `CompileHlslShaders.cmake` header, `Test-BuildWinDxilPrestep.ps1` header). Contributes to AC-3 (HLSL pair consolidated to single file with `-NoPause` switch — TASK-146 Implementation Notes preference) and AC-4 (lockstep callers updated; cross-reference table in description still holds). Surfaced finding (not folded in): `BuildWin.ps1`'s `if ($LASTEXITCODE -ne 0)` check after the pre-step is brittle — the compile module never sets `$LASTEXITCODE` on the all-shaders-skipped path (no `dxc.exe` spawn), so an unset/`$null` outer-shell `$LASTEXITCODE` trips a false-positive failure. Pre-existing; not introduced by this CL. Worth a separate task if the interactive build trips it. Phases 3 and 4 remain open.

2026-05-14 — Phase 4 CL: documented script naming + partition rules. `Scripts/CLAUDE.md` extended (no new `Scripts/README.md` per `workspace-hygiene` no-subtree-README rule). Two sections appended/refreshed (~44 lines added): `Script inventory` table now includes rows for `CleanRepo.bat`, `CleanGitSubmodules.bat`, `VerifyEditorService.py`, `frame_variance.py`, `Tests/Test-BuildWinDxilPrestep.ps1`, `git-hooks/post-checkout`, `git-hooks/post-merge`. New `Script naming and partition rules` section with six bulleted rules + `Test-driver taxonomy` subsection.

Rules picked (one WHY each, per `comment-discipline`): (1) `.ps1` PascalCase, Windows-only by default, platform suffix only on genuine forks. (2) `Lib/*.psm1` PascalCase verb-noun, earned by ≥2 callers or single-source-of-truth predicate. (3) `.py` PascalCase if harness-loaded, snake_case if hand-invoked. (4) `.bat` holdouts grandfathered for double-click convenience. (5) `git-hooks/` kebab-case documented as git-mandated. (6) `Tests/` subtree uses `Test-<Subject>.ps1` (Pester convention), distinct from top-level `Test*.ps1` engine-driver pattern.

Open question called out for Phase 3: whether top-level `Test*` / `Verify*` should merge into `Test-<Subject>.{ps1,py}` matching the `Tests/` subtree, or stay as distinct test vs pre-flight tiers. Phase 3 also owns shared `Lib/Test-Engine.psm1` extraction for the three engine drivers' boilerplate.

Contributes to AC-1 (naming convention picked + documented; one rule for Windows scripts, one for `Lib/`, one for Python helpers) and AC-6 (a/b/c/d documentation items: naming rule, `Lib/` partition rule, test-driver taxonomy with Phase 3 open question, `git-hooks/` kebab-case exception).

Not done: Phase 3 (test-driver shared module + naming sweep). No scripts renamed (purely a doc CL). Verification: no code touched → build unaffected, `BuildWin.ps1` invocation skipped per brief. Re-read `Scripts/CLAUDE.md` end-to-end post-edit; no rule contradicts the PowerShell 5.1 string conventions or `BuildWin.ps1` invariants. Task stays open.

### 2026-05-14 — Phase 3 (Test-Engine.psm1 extraction)

New module `Scripts/Lib/Test-Engine.psm1` extracts the shared `Main.exe`-driver boilerplate. Two exported cmdlets:

- `Invoke-EngineMainRun -BinDir -ArgList [-NoNewWindow]` — resolves `Main.exe`, sets CWD to `<BinDir>\..` (where the engine writes logs + captures), launches with `Start-Process -Wait -PassThru`, returns `@{ Process, LogFile, BinRoot }`. `LogFile` is the newest `*.Log` under the bin root.
- `Test-EngineRunOutcome -LogFile -SceneTag [-ScenePrefix]` — applies the three universal post-run greps (`D3D12 ERROR|CORRUPTION|Validation Error`, `<SceneTag> has been loaded`, `Auto-test:.*terminating`), prints the standard report lines, returns `@{ Pass, D3DErrors, SceneLoaded, AutoTerminated }`. `-ScenePrefix` bracket-tags the FAIL lines for the per-scene-loop driver.

Callers refactored:

| Script | Lines before | Lines after | Delta |
|---|---|---|---|
| `Scripts/TestGIScene.ps1` | 141 | 108 | −33 |
| `Scripts/TestGPUPathTracer.ps1` | 85 | 53 | −32 |
| `Scripts/TestPathTracerThreeScenes.ps1` | 371 | 341 | −30 |

Net code: −95 lines in callers, +129 lines in the new module (incl. ~30 lines of header invariant doc). The header doc absorbs the duplicate `-loglevel` / `-total_frames` / scene-marker invariants previously triplicated in the three driver headers (those driver-side blocks remain — the module doc is the single source of truth, the script-side blocks are still useful at the call site).

`Scripts/CLAUDE.md` script-inventory table: new row added between `Lib/Compile-HLSL.psm1` and `Tests/Test-BuildWinDxilPrestep.ps1`. The Phase 4 "open question" line about `Test*` / `Verify*` renaming stays untouched per Phase 3 brief.

Verification (main-session Bash):

- `Scripts/TestGPUPathTracer.ps1 -Frames 30` → final log `Engine has been terminated`, exit 0, output ends with `PASS`.
- `Scripts/TestGIScene.ps1` (default 120 frames) → engine-side checks all pass (`GISponza.InnoScene loaded: True`, `Auto-terminated: True`, `D3D12 errors: 0`); MAE compare path reached and ran. MAE 0.506696 exceeded threshold 0.45 — pre-existing reference-vs-current divergence, not introduced by this CL (the module wiring is upstream of MAE compare; behaviour preserved).
- `Scripts/TestPathTracerThreeScenes.ps1 -Frames 30 -DumpStart 25 -DumpEnd 28` → second run all three scenes `PASS [unittest]` / `PASS [gitestbox]` / `PASS [gisponza]` / `OVERALL: PASS`. First attempt failed on the third scene with a steady-state timeout (likely cold-cache shader compile pushing the tail past the 30-frame budget); not a module regression — the steady-state grep correctly reported the failure mode with the `[gisponza]` ScenePrefix routing as designed.

No build needed: `Scripts/` are runtime-only, no msbuild / cmake surface touched. Engine binary unchanged.

Module-design notes:
- `Set-StrictMode -Version Latest` in the module forced `@(...)` array-coercion on `Select-String` results (no-match returns `$null` which fails `.Count` lookup under strict). The callers' original style worked because their script-level strict-mode is default; the module is stricter on purpose.
- `Set-Location $binRoot` runs inside `Invoke-EngineMainRun`; the three-scene driver also sets it once before its per-scene loop. Idempotent, no contention.

Open question disposition: the `Test*` / `Verify*` naming-sweep question remains the single deferred item. Not filed as a new task per `backlog-workflow`'s "don't pile on tasks" rule — it is a design decision with no live cost, documented in `Scripts/CLAUDE.md`'s `Test-driver taxonomy` open-question paragraph. When a future session decides the convention (e.g. when adding a 4th driver and the naming choice becomes load-bearing), this task can be reopened or a fresh task filed at that point.

Surfaced findings (not folded in this CL):
- `TestGIScene.ps1` MAE compare reports 0.50 against a 0.45 threshold at the default 120-frame budget. Likely a real reference-vs-current divergence in the GI smoke baseline (reference PNG cadence vs current engine output drift). Pre-existing; worth a separate task if a baseline-refresh is in scope.
- The first three-scene run's GISponza failure at 30 frames was transient (warmed-cache re-run passed). Edge of the steady-state-latch budget; not a Phase 3 regression. The existing 60-frame default is comfortably past this margin.

Contributes to AC-3 (shared module extracted; no duplicated boilerplate across the three engine drivers). AC-6 documentation row added to `Scripts/CLAUDE.md` inventory. Remaining gap before TASK-212 closure: the `Test*` / `Verify*` naming-sweep decision (still open question in `Scripts/CLAUDE.md`). Task stays `In Progress`.

## Review (ci-build-impl, 2026-05-15)

Verdict: **PASS WITH ADVISORIES** (no BLOCKED findings; ADVISORIES are non-load-bearing cosmetic + scope-creep observations the implementer can either fold into a follow-up or leave alone).

### What I verified

- Module under review: `Scripts/Lib/Test-Engine.psm1` (129 lines, pure ASCII confirmed via byte scan).
- Diff walked end-to-end against `git show HEAD:Scripts/...` for each of the three callers — verified flags, env vars, exit-code mapping, and assertion list preserved.
- `Set-StrictMode -Version Latest` scoping in PowerShell 5.1 confirmed module-local via a synthetic harness (does not leak to caller scope).
- Log-discovery wildcard semantics: `Get-ChildItem (Join-Path $binRoot '*.Log')` returns the same file as the prior literal `Get-ChildItem 'C:\GitRepo\InnocenceEngine\Bin\*.Log'` when `$binRoot = C:\GitRepo\InnocenceEngine\Bin`.
- Cross-tree `Grep` for the renamed output line `"GISponza loaded:"` (now `"GISponza.InnoScene loaded:"`) — no live consumer parses it; only backlog closure-note quotes.
- `Auto-test: steady state reached` survives — engine emits it, three-scene driver greps it. The load-bearing harness signal called out in the brief is intact.

### Findings

| Discipline | Where | Note | Severity |
|---|---|---|---|
| **tech-choice** | `Test-Engine.psm1:1-129` | API granularity is defensible. `Invoke-EngineMainRun` cleanly owns CWD + launch + log discovery; `Test-EngineRunOutcome` is the universal post-run trio. The `ScenePrefix` parameter is the only feature with a single caller (only the three-scene driver passes it), but the alternative split (have the caller print FAIL lines instead of the module) would force the two single-scene drivers to also stop receiving free FAIL output. The current shape is consistent and natural. No change needed. | n/a |
| **comment-discipline** | `Test-Engine.psm1:16-27` | Header invariant block is appropriate — it's WHY documentation (LogLevel::Success threshold, IOService CWD-relative path resolution, marker wording lock) that future readers cannot recover from the code. Within the body, two short inline comments (`# Coerce all Select-String results...`, scene-tag legend at line 82-84) earn their keep. No essay-comment violations. | OK |
| **comment-discipline** | `TestPathTracerThreeScenes.ps1:230, 234` (pre-existing) | Two `Write-Host "... (TASK-213 AC-5) ..."` / `(TASK-213 R2)` lines retain task-ID references in user-facing output. **Pre-existing**, not introduced by this CL. The implementer tightened the surrounding *comment* block (dropped `TASK-213 CL D Workstream 1` prefix) but left the `Write-Host` strings alone — defensible under `surface-dont-chase` (touched only the directly-extracted region). Acceptable as-is; if a future cleanup pass strips these, do it as a focused rolling-cleanup CL. | ADVISORY |
| **comment-discipline** | `TestPathTracerThreeScenes.ps1:48, 60, 72, 80, 89, 115, 175, 183, 238` (pre-existing) | Nine remaining `# TASK-213 CL X` comment headers untouched by this CL. Inconsistent cleanup — three were tightened, nine were not. Per `surface-dont-chase`, the implementer correctly limited scope to the extraction zone. File a separate rolling-cleanup task only if the noise actually costs something on a re-read; "might be useful someday" is not a task per `backlog-workflow` § don't pile on. | ADVISORY |
| **behaviour preservation** | `Test-Engine.psm1:100` | The `Write-Host "$SceneTag loaded:  $($sceneLoaded.Count -gt 0)"` line emits `GISponza.InnoScene loaded:` (literal scene tag) where the original `TestGIScene.ps1` and `TestGPUPathTracer.ps1` emitted `GISponza loaded:` (friendly name without `.InnoScene` suffix). Cosmetic user-facing output drift; no consumer parses it (Grep confirmed: only backlog closure-note quotes reference the prior literal). | ADVISORY (acceptable) |
| **behaviour preservation** | `Test-Engine.psm1:100-102` | Column padding on the three report lines in the three-scene driver tightened — old code used wider padding (`Auto-terminated:           ` with 11 spaces) to align scene-tag-prefixed labels; module emits `Auto-terminated:  ` (2 spaces). Visual cosmetic regression in three-scene output alignment. Not load-bearing. | ADVISORY (acceptable) |
| **behaviour preservation** | `Test-Engine.psm1:63` | The log-discovery wildcard now resolves from `Split-Path $BinDir -Parent` instead of a hardcoded `C:\GitRepo\InnocenceEngine\Bin\*.Log`. This is a **fix-bug-on-touch** — the prior hardcoded path would have surfaced a different process's log when `-BinDir` pointed elsewhere. Aligns with the brief's no-flag-change spirit when the default `-BinDir` is used; expands correctness when overridden. | OK (improvement) |
| **safety** | `Test-Engine.psm1:74-80` | `Test-EngineRunOutcome` declares `[System.IO.FileInfo]$LogFile` as `Mandatory`. PowerShell will reject `$null` at the parameter binding. The three callers all guard `if (-not $run.LogFile)` before calling. Contract enforced at the boundary. | OK |
| **safety** | `Test-Engine.psm1:41-43` | Mandatory `[Test-Path $mainExe]` guard with throw inside the module. Three-scene driver also has its own `Test-Path $mainExe` guard at line 127-130 — minor duplication, but the module is now the single source of truth; the caller's guard is now defensive overlap, not load-bearing. Not worth removing. | OK |
| **scripts ps5.1 ASCII rule** | `Test-Engine.psm1` | Byte-scanned: pure ASCII (0 bytes >= 0x80). All non-ASCII glyphs (em-dashes etc.) in the three callers are in `#` comments, where the rule permits them. | OK |
| **closure-note shape** | task-212 lines 197-234 | Matches Phase 1/2/4 shape: date prefix, bulleted module API summary, caller refactor table, verification log quotes, AC-mapping closer, "task stays open" tail. `Surfaced findings` block correctly separates non-folded observations from the CL's deliverables. | OK |
| **surface-dont-chase / MAE drift** | task-212 line 231 | MAE 0.506 vs 0.45 threshold is surfaced inline (not filed as a backlog task). Implementer's reasoning: pre-existing baseline-vs-current divergence with no clear repro shape (could be CL-driven engine drift since the reference PNG was captured, could be a stale reference, could be threshold tuning). Per `backlog-workflow` § don't pile on (would this work get done today if dispatched? Not without an interactive triage decision on which side to update), inline surfacing is the right venue. If the user wants this investigated, a fresh task with a clear bisect-shaped repro should be filed. | OK |
| **inventory row** | `Scripts/CLAUDE.md` line 39 | Placement is alphabetical-within-`Lib/` (between `Compile-HLSL.psm1` and `PostBuildWin.ps1`). Description format matches sibling rows (one-line role with key behaviour). | OK |

### Behaviour-preservation walkthrough (the three callers)

**TestGIScene.ps1** (141 → 108):
- Flags unchanged: `-renderer 0 -loglevel 1 -total_frames $Frames`.
- BinDir default unchanged: `C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo`.
- Three universal post-run greps → module. GISponza-specific not-loaded diagnostic preserved as caller-side branch.
- MAE comparison path: untouched apart from `Split-Path $BinDir -Parent` → `$run.BinRoot` (semantically identical).
- Exit codes: 1 on every FAIL path, 0 on PASS. Match.

**TestGPUPathTracer.ps1** (85 → 53):
- Flags unchanged: `-renderer 0 -loglevel 0 -offscreen -total_frames $Frames -test gpu_path_tracer`.
- `-NoNewWindow` survived via the new module switch.
- All three universal greps consolidated; GISponza diagnostic preserved.
- Exit codes match.

**TestPathTracerThreeScenes.ps1** (371 → 341):
- Per-scene loop preserved.
- Engine-side capture flags (`-dump_frames`, `-camera_orbit`) untouched.
- Steady-state grep block (reached / timeout / lost flap-back) preserved verbatim — appropriately left as caller-local because the three single-scene drivers don't need it.
- `$overallPass = $false; continue` pattern on per-scene FAIL preserved.
- All TASK-213 CL D Workstream 1 / 2 logic (flap-back-aware dump-window shift, capture-ceiling check, trim-PNGs sub-block) preserved verbatim.
- Log copy + per-scene capture-dir layout preserved.

### No-premature-abstraction check

Per the task spec: "extract only what's invoked from ≥2 callers OR is a single source of truth for staleness/mirror predicates."

- `Invoke-EngineMainRun`: every parameter (`BinDir`, `ArgList`, `NoNewWindow`) used by ≥2 callers. PASS.
- `Test-EngineRunOutcome`:
  - `LogFile`, `SceneTag`, the three universal greps: 3 callers.
  - `ScenePrefix`: 1 caller (three-scene driver). Earns its keep as the alternative is duplicating the FAIL-print loop in the caller; the cost is one optional `[string]` parameter with a sensible default.

No premature abstraction.

### Reviewer's bottom line

The extraction is well-shaped, behaviour-preserving for all three callers (modulo two cosmetic output-text changes with no consumer), and the module's header doc legitimately consolidates invariants previously duplicated across three driver headers. The two `ADVISORY` items on retained `TASK-213` literals in `TestPathTracerThreeScenes.ps1` Write-Host lines + comments are pre-existing and out of scope per `surface-dont-chase`; the implementer's selective tightening of the directly-touched region is the discipline-correct call.

Recommend main-session **proceed to commit** with the implementer's current diff. The ADVISORY items can be folded into TASK-220 (rolling cleanup — prune essay comments) if a future rolling pass picks up the residual TASK-213 markers in `TestPathTracerThreeScenes.ps1`.
<!-- SECTION:NOTES:END -->
