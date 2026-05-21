## BuildWin.ps1

Orchestrates the engine build. Invariants:

- Pre-step: invokes `HLSL2DXIL.ps1 -NoPause` before `msbuild`. The shader compile module (`Lib/Compile-HLSL.psm1`) is the single source of truth for staleness — it does per-shader and per-`#include` `LastWriteTimeUtc` comparisons. Do not duplicate that predicate in `BuildWin.ps1`; query the module.
- Pre-step failure aborts before `msbuild`. A partial DXIL set produces silent-stale failures at runtime.
- Post-step: invokes `RegenClangdIndex.ps1` after `msbuild` succeeds. Runs a parallel Ninja configure to emit `compile_commands.json` (the VS generator does not emit it), then chains `PurgeStaleClangdIndex.ps1` to drop orphan `.idx` cache entries.
- Post-step failure emits `Write-Warning` and lets the build's exit code stand. The engine binary is fine; the clangd index is just stale until next refresh.

Escape hatches:

- `-SkipShaderCompile` — skip the HLSL pre-step. C++-only bisects.
- `-SkipClangdIndexRefresh` — skip the clangd post-step. No clangd-aware editor / debugging a CMake configure issue.
- `Scripts/PurgeStaleClangdIndex.ps1 -PurgeAll` — drop every `.idx`. Recovery for corrupt-but-live cached translation units (the orphan-source heuristic only catches deleted sources).

The `cmake --build` invocation path bypasses both pre- and post-step. Separate fix surface; not addressed by `BuildWin.ps1`.

## PowerShell 5.1 string conventions

`.ps1` files in this tree have no UTF-8 BOM. PowerShell 5.1 decodes them as Windows-1252.

Rule: **double-quoted strings in `.ps1` files must be pure ASCII.** A non-ASCII byte inside a double-quoted string mangles the string-terminator and produces cascading parser errors.

Acceptable for non-ASCII glyphs (em-dash, smart quotes, etc.):

- `# comment lines`
- `'single-quoted strings'`
- here-docs and other non-evaluated literals

Applies until the project moves to PowerShell 7+ or commits to UTF-8-with-BOM source encoding.

## Script inventory

| Script                       | Role                                                                  |
|------------------------------|-----------------------------------------------------------------------|
| `BuildWin.ps1`               | Build Main + RenderTest. HLSL pre-step + clangd post-step.            |
| `HLSL2DXIL.ps1`              | Interactive HLSL → DXIL compile (use `-NoPause` for CI / automation invocations). |
| `Lib/Compile-HLSL.psm1`      | Shared compile module — single source of truth for shader build policy. |
| `Lib/Test-Engine.psm1`       | Shared engine-driver module — `Main.exe` launch + log discovery + standard D3D12 / scene-load / auto-terminate post-run checks for the three `Test*` smoke drivers. |
| `PostBuildWin.ps1`           | Post-build deploy / cleanup hooks.                                    |
| `StartEngineWin.ps1`         | Launch the runtime engine binary.                                     |
| `StartEditorWin.ps1`         | Launch the editor binary.                                             |
| `TestEditorComplete.ps1`     | Editor smoke test driver (orchestrates `VerifyEditorService.py` + `npm test`). |
| `TestGIScene.ps1`            | GI scene capture / regression driver.                                 |
| `TestPT.ps1`      | GPU path tracer single-scene capture.                                 |
| `TestPTThreeScenes.ps1` | Three-scene PT capture.                                            |
| `InteractiveTest.ps1`        | Interactive engine smoke test (windowed + keystroke automation). Invoked from `.vscode/tasks.json`. |
| `VerifyEditorService.py`     | Engine IPC websocket round-trip pre-flight. Invoked by `TestEditorComplete.ps1`. |
| `frame_variance.py`          | Ad-hoc frame-variance analysis (manual invocation; no live caller).   |
| `DownloadAssets.ps1`         | Asset download / sync.                                                |
| `RegenClangdIndex.ps1`       | Regenerate clangd compile_commands.json index.                        |
| `PurgeStaleClangdIndex.ps1`  | Purge stale clangd index entries.                                     |
| `CleanRepo.bat`              | Manual double-click cleanup of build artefacts.                       |
| `CleanGitSubmodules.bat`     | Manual double-click submodule reset.                                  |
| `Tests/Test-BuildWinDxilPrestep.ps1` | Pester-shaped harness for the HLSL pre-step staleness contract. |
| `git-hooks/InstallHooks.ps1` | Install repo-local git hooks.                                         |
| `git-hooks/post-checkout`    | Drops orphan clangd `.idx` entries after a checkout (kebab-case mandated by git). |
| `git-hooks/post-merge`       | Same as `post-checkout`, fires after merges.                          |

## Script naming and partition rules

Rationale: TASK-212. Picked after Phases 1+2 settled the de-facto pattern; codified here so a future session applies the same rule before adding a new entry point.

- **PowerShell scripts (`.ps1`): PascalCase, Windows-only by default.** No platform suffix unless a script genuinely has platform-divergent behaviour (`BuildWin.ps1`, `PostBuildWin.ps1`). The engine is Windows-only in practice; non-`Win` siblings were dead and were removed in Phase 1. WHY: a suffix on every script is noise when one platform is the only target; reserve it for genuine forks.
- **Shared modules under `Lib/`: PascalCase verb-noun `.psm1`** (`Compile-HLSL.psm1` — the embedded kebab is PowerShell module convention, not a violation). Earned when behaviour must be invoked from ≥2 callers, OR when a staleness / mirror predicate is the single source of truth and ad-hoc duplication risks drift. WHY: `Lib/` is reserved for code that has paid for its extraction with a real second caller — otherwise it's premature abstraction.
- **Python helpers (`.py`): PascalCase when loaded by another script, snake_case when invoked manually.** `VerifyEditorService.py` (called from `TestEditorComplete.ps1`) is PascalCase; `frame_variance.py` (ad-hoc analysis) is snake_case. WHY: the case telegraphs harness-loaded vs hand-invoked at a glance.
- **Batch holdouts (`.bat`):** `CleanRepo.bat`, `CleanGitSubmodules.bat` are grandfathered manual-invocation entry points retained for double-click convenience. Not a rule to apply to new scripts. WHY: `.ps1` is the standard everywhere else; the `.bat` survivors exist only because clicking through Explorer is the actual use case.
- **`git-hooks/` subtree: kebab-case (`post-checkout`, `post-merge`).** Mandated by git's hook-naming requirement, not a violation of the PowerShell rule. The `InstallHooks.ps1` installer inside follows the parent PascalCase rule. WHY: git looks for these exact filenames; rename and the hook stops firing.
- **`Tests/` subtree: `Test-<Subject>.ps1`** (precedent: `Test-BuildWinDxilPrestep.ps1`). Mirrors the Pester / pwsh-test community convention. WHY: distinct from the top-level `Test*.ps1` engine-driver pattern, which has a different purpose (driving `Main.exe`, not asserting against a unit).

### Test-driver taxonomy (top level)

- `Test*.ps1` — engine-driver tests that launch `Main.exe` and assert on its output (`TestGIScene.ps1`, `TestPT.ps1`, `TestPTThreeScenes.ps1`, `TestEditorComplete.ps1`).
- `Verify*.{ps1,py}` — engine-stack readiness pre-flights (currently `VerifyEditorService.py` only; the `.ps1` sibling was removed in Phase 1). `Verify` = "the stack is healthy enough to run a test." `Test` = "the feature behaves correctly."
- `InteractiveTest.ps1` — user-driven smoke (windowed + keystroke automation). Distinct from headless `Test*` because the entry point is human eyes on a window, not an exit code.

**Open question (Phase 3, not picked here):** whether the `Test*` / `Verify*` split should merge into a single `Test-<Subject>.{ps1,py}` convention matching the `Tests/` subtree, or be formalised as a distinct pre-flight tier. Phase 3 also owns extraction of a shared `Lib/Test-Engine.psm1` module for the `Main.exe` boilerplate (`-loglevel` / `-total_frames` sync warnings, `Auto-test: ... terminating` log grep) shared across the three engine drivers.
</content>
</invoke>