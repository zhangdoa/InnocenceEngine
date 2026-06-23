## BuildWin.ps1

Orchestrates the engine build.

- **Pre-step**: invokes `HLSL2DXIL.ps1 -NoPause` before `msbuild`. Single source of truth for shader staleness is `Lib/Compile-HLSL.psm1` (per-shader + per-`#include` `LastWriteTimeUtc` compare). Do not duplicate that predicate.
- **Pre-step failure** aborts before `msbuild`. Partial DXIL set = silent-stale failures at runtime.
- **Post-step**: invokes `RegenClangdIndex.ps1` after `msbuild` succeeds. Ninja-configures `compile_commands.json` (VS generator does not emit it), then `PurgeStaleClangdIndex.ps1` drops orphan `.idx` entries.
- **Post-step failure** emits Warning and lets build's exit code stand. Engine fine; clangd index just stale until next refresh.

Escape flags: `-SkipShaderCompile`, `-SkipClangdIndexRefresh`. `Scripts/PurgeStaleClangdIndex.ps1 -PurgeAll` drops every `.idx` for corrupt-cache recovery.

`cmake --build` invocation bypasses both pre- and post-step. Separate fix surface.

## PowerShell 5.1 string encoding

`.ps1` files have no UTF-8 BOM. PS 5.1 decodes them as Windows-1252.

**Double-quoted strings in `.ps1` must be pure ASCII.** Non-ASCII byte inside a double-quoted string mangles the string-terminator and cascades parser errors. OK in `# comments`, `'single-quoted strings'`, here-docs.

## Engine verification: offscreen runs have NO usable text sink

`LogService` mirrors every line to a timestamped `[…].Log` in the engine CWD, but that file
is empty in offscreen / headless / redirected runs (LogService flushes it only on a graceful
*interactive* dtor). `Main.exe` is a `/SUBSYSTEM:WINDOWS` (WinMain) binary, so it also writes
nothing to a redirected or inherited **stdout**. A fully healthy audit run can leave BOTH the
`.Log` and captured stdout at 0 bytes (observed 2026-06-23 — earlier "stdout is the reliable
sink" guidance was wrong).

**Reliable signals for an offscreen run: the process EXIT CODE and the PRODUCED ARTIFACTS** —
not text. Gate on:
- **exit code 0** (graceful auto-terminate). `StartEngineWin.ps1` does `-Wait` + `exit
  $proc.ExitCode`; `Invoke-EngineBounded` returns `.ExitCode` / `.TimedOut`.
- **expected artifacts at non-degenerate size** — `Test-EngineArtifacts` (point it at the
  GBuffer / LightPass / FinalBlend HDRs that can't be black; NOT LUTs/masks like BRDFLUTMS
  ~22KB or SSRC ProbeMask ~2KB, which are legitimately small).

`Test-EngineRunOutcome` greps the `.Log`; on an empty log it returns `LogEmpty=$true` and its
load/terminate predicates are INCONCLUSIVE (not a FAIL) — advisory only. Windowed runs DO
populate the `.Log` (readable while the process is alive); there the predicates are meaningful.

Use `Invoke-EngineBounded` (`Lib/Test-Engine.psm1`) for programmatic verification: it kills
stragglers + settles (a run killed mid-frame can hang the next launch — TASK-241) and bounds
the hang via `WaitForExit` + `Kill` (never orphaning the child, which a bash `timeout` would).
Its stdout/stderr captures are a best-effort crash hint only.

## Script inventory

| Script | Role |
|---|---|
| `BuildWin.ps1` | Build Main + RenderTest. HLSL pre-step + clangd post-step. |
| `HLSL2DXIL.ps1` | HLSL → DXIL compile. Use `-NoPause` for non-interactive. |
| `Lib/Compile-HLSL.psm1` | Shared shader-compile module. Single source of truth. |
| `Lib/Test-Engine.psm1` | Shared engine-driver. `Invoke-EngineMainRun` (unbounded), `Invoke-EngineBounded` (kills+settles+bounds+captures stdout), `Test-EngineRunOutcome` (post-run checks; greps the empty `.Log` — prefer stdout). |
| `regression-rebuild.ps1` | CMake regen + build + clangd purge. |
| `sweep-orphan-engines.ps1` | Kill stray Main/RenderTest/InteractiveTest processes. |
| `pick-frame-count.ps1 <budget_ms> <frame_ms>` | Emit `-total_frames N` for a budget. |
| `PostBuildWin.ps1` | Post-build deploy / cleanup. |
| `StartEngineWin.ps1` | Launch engine binary. |
| `StartEditorWin.ps1` | Launch editor binary. |
| `TestEditorComplete.ps1` | Editor smoke driver (`VerifyEditorService.py` + `npm test`). |
| `TestGIScene.ps1` | GI scene regression. |
| `TestPT.ps1` | PT single-scene capture. |
| `TestPTThreeScenes.ps1` | PT three-scene capture. |
| `InteractiveTest.ps1` | Windowed + keystroke automation. Invoked from `.vscode/tasks.json`. |
| `VerifyEditorService.py` | Engine IPC WS round-trip pre-flight. |
| `frame_variance.py` | Ad-hoc analysis. Manual invocation. |
| `DownloadAssets.ps1` | Asset download. |
| `RegenClangdIndex.ps1` | Regenerate clangd compile_commands.json. |
| `PurgeStaleClangdIndex.ps1` | Purge stale `.idx` entries. |
| `CleanRepo.bat`, `CleanGitSubmodules.bat` | Double-click cleanup. |
| `Tests/Test-BuildWinDxilPrestep.ps1` | Pester-shaped HLSL pre-step harness. |
| `git-hooks/InstallHooks.ps1` | Install repo-local git hooks. |
| `git-hooks/post-{checkout,merge}` | Drop orphan clangd `.idx` after checkout/merge. |

## Naming

- `.ps1` PascalCase, Windows-only by default. Platform suffix only on genuine forks (`BuildWin.ps1`).
- `Lib/*.psm1` PascalCase verb-noun. Earn extraction with a real second caller.
- `.py` PascalCase if loaded by another script; snake_case if manual.
- `.bat` only for legacy double-click entry points.
- `git-hooks/` kebab-case (git mandate).
- `Tests/Test-<Subject>.ps1` (Pester-style).

## Test-driver taxonomy

- `Test*.ps1` — engine-driver tests launching `Main.exe`.
- `Verify*` — engine-stack readiness pre-flights.
- `InteractiveTest.ps1` — windowed + keystroke (user-driven smoke).
