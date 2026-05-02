---
id: TASK-212
title: 'Scripts/ folder fragmentation — dead scripts, naming drift, overlapping test drivers'
status: To Do
assignee:
  - ci-build-expert
created_date: '2026-05-02 00:00'
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
| AC | Bar |
|---|---|
| AC-1 | Naming convention picked + documented (`Scripts/README.md` or extension to `Scripts/CLAUDE.md`). All scripts conform; one rule for Windows scripts (suffix or no-suffix), one rule for shared modules under `Lib/`, one rule for Python helpers. |
| AC-2 | Every remaining script either has at least one known caller (or is a documented manual-invocation entry point named in the README) OR is deleted. The caller cross-reference table above is the audit baseline; the closure CL re-runs grep to confirm. |
| AC-3 | No two scripts duplicate functionality. `TestEditorIPC.py` vs `VerifyEditorService.py` resolved (one deleted). `HLSL2DXIL.ps1` + `HLSL2DXIL_NoPause.ps1` either consolidated to a single file with `-NoPause` switch (preferred per TASK-146 Implementation Notes) or the duplication is justified inline. |
| AC-4 | README, `.vscode/tasks.json`, `CMakeLists.txt`, `.clangd`, `GEMINI.md`, `.claude/disciplines/*` updated in lockstep with any moves / deletes / renames. The caller cross-reference table in this task's Description is the lockstep checklist. |
| AC-5 | Mac / Linux dead-script question resolved: either restored to working state (engine has not been Win-only-by-policy, just by-practice) or deleted with a one-line note in `README.md` clarifying the engine is currently Windows-only. README Linux/Mac sections updated to match. |
| AC-6 | `Scripts/README.md` (or extended `CLAUDE.md`) documents: (a) the naming rule from AC-1, (b) the partition rule for `Lib/` (when a script earns a shared-module extraction), (c) the test-driver taxonomy (which `Test*.ps1` does what; the `Verify*` vs `Test*` distinction or the merger), (d) the git-hooks subtree's kebab-case exception. |
<!-- AC:END -->

## Approach options

- **A — One CL, all six ACs.** Rename + delete + consolidate + docs in a single ci-build-expert dispatch. Lower review overhead, but a long-changelist CL with cross-cutting renames is brittle if any caller-update is missed.
- **B — Phased.** Phase 1: delete dead scripts (`StartEngineLinux.sh`, `StartEngineMac.sh`, `PostBuildMac.sh`, `CleanRepo.sh`, `TestEditorIPC.py`, `VerifyEditorStack.ps1` if confirmed orphan, `ConvertModels.ps1` if confirmed orphan, `GenerateMetadataWin.ps1`) + README cleanup. Phase 2: HLSL pair consolidation. Phase 3: Test-driver shared module + naming sweep. Phase 4: `README.md` / `CLAUDE.md` rule documentation. Three smaller CLs land cleanly; orthogonality risk is bounded per phase.

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
<!-- SECTION:NOTES:END -->
