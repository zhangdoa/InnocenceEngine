Owned by the `ci-build-impl` stage (build, shader-compile, test-runner, automation scripts). See `.claude/agents/ci-build-impl.md` and `.claude/team.md`.

## BuildWin.ps1

Orchestrates the engine build. Invariants:

- Pre-step: invokes `HLSL2DXIL_NoPause.ps1` before `msbuild`. The shader compile module (`Lib/Compile-HLSL.psm1`) is the single source of truth for staleness — it does per-shader and per-`#include` `LastWriteTimeUtc` comparisons. Do not duplicate that predicate in `BuildWin.ps1`; query the module.
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
| `HLSL2DXIL.ps1`              | Interactive HLSL → DXIL compile (ends with `Pause`).                  |
| `HLSL2DXIL_NoPause.ps1`      | CI / automation HLSL → DXIL compile (no `Pause`).                     |
| `Lib/Compile-HLSL.psm1`      | Shared compile module — single source of truth for shader build policy. |
| `GenerateMetadataWin.ps1`    | Generate component metadata (parser pass).                            |
| `PostBuildWin.ps1`           | Post-build deploy / cleanup hooks.                                    |
| `StartEngineWin.ps1`         | Launch the runtime engine binary.                                     |
| `StartEditorWin.ps1`         | Launch the editor binary.                                             |
| `TestEditorComplete.ps1`     | Editor smoke test driver.                                             |
| `TestGIScene.ps1`            | GI scene capture / regression driver.                                 |
| `TestGPUPathTracer.ps1`      | GPU path tracer single-scene capture.                                 |
| `TestPathTracerThreeScenes.ps1` | Three-scene PT capture.                                            |
| `VerifyEditorStack.ps1`      | Editor stack verification.                                            |
| `InteractiveTest.ps1`        | Interactive engine smoke test.                                        |
| `ConvertModels.ps1`          | Model converter wrapper.                                              |
| `DownloadAssets.ps1`         | Asset download / sync.                                                |
| `RegenClangdIndex.ps1`       | Regenerate clangd compile_commands.json index.                        |
| `PurgeStaleClangdIndex.ps1`  | Purge stale clangd index entries.                                     |
| `git-hooks/InstallHooks.ps1` | Install repo-local git hooks.                                         |
</content>
</invoke>