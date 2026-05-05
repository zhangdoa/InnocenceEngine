# Scripts/

Build, shader-compile, test-runner, and engine-launcher scripts. Owned by the
`ci-build-expert` stage (see `.claude/agents/ci-build-expert.md`).

## BuildWin.ps1 — DXIL staleness policy (TASK-214)

`BuildWin.ps1` invokes `HLSL2DXIL_NoPause.ps1` *before* `msbuild` on every run.
A C++-only build is preceded by a no-op shader pass; a build that follows an
HLSL edit recompiles the affected shaders before the engine binary is built.

### Why auto-trigger (option a), not loud-fail (option b)

The task offered two paths:

- **(a) Auto-trigger** — `BuildWin.ps1` calls `HLSL2DXIL_NoPause.ps1`; the
  underlying module decides per-shader whether to recompile.
- **(b) Loud-fail** — `BuildWin.ps1` walks `Source/Shaders/HLSL/` vs
  `Bin/Shaders/DXIL/`, errors with "DXIL stale, run HLSL2DXIL_NoPause.ps1"
  if any source is newer than its DXIL output, and exits without building.

Auto-trigger was chosen for three reasons:

1. **The compile module is already idempotent.** `Scripts/Lib/Compile-HLSL.psm1`
   does per-shader `LastWriteTimeUtc` comparison against the source *and* every
   `#include` dependency (`shaderDependencies` map). A no-edit invocation
   compiles zero shaders and pays only a directory enumeration plus a few
   stat calls — sub-second on a warm SSD.

2. **Loud-fail would be a shadow check.** To produce the diagnostic, option (b)
   would have to re-implement the staleness predicate in `BuildWin.ps1`. That
   predicate would drift from `Compile-HLSL.psm1`'s real policy — most obviously
   on the `#include` graph. The first time someone added an `#include` to a
   shared header without updating the duplicate check, builds would proceed
   with stale DXIL and the failure mode TASK-214 was filed to eliminate would
   reappear. `disciplines/on-implement/no-shadow-state.md` operationalises
   this rule: state arises from the data structures that already represent it;
   query the source of truth, do not mirror it.

3. **Auto-trigger eliminates the failure mode; loud-fail merely flags it.**
   The user-observable symptom (PSO `E_INVALIDARG` from stale DXIL) does not
   re-occur after a clean build under option (a). Under option (b) the symptom
   is replaced by an error message that requires the user to run a second
   command and rerun the build. The cost differential between (a) and (b) on a
   no-edit build is in the millisecond range; the cost of (b) on an edit build
   is one extra manual command. Option (a) is strictly better on both axes.

### Failure handling

If the HLSL pre-step exits non-zero (DXC compile error, missing source dir,
missing `dxc.exe`), `BuildWin.ps1` aborts before invoking `msbuild`.
Proceeding to `msbuild` with a partial DXIL set would just reproduce the
silent-stale failure shape in a new disguise — some PSOs would build cleanly
against the freshly-compiled subset while others fail `E_INVALIDARG` at
runtime against unmodified-but-stale neighbours.

### Escape hatch

`BuildWin.ps1 -SkipShaderCompile` skips the pre-step. Reserved for the rare
bisect of a C++-only change against a known-good DXIL set; never the default.

### Related work

- **TASK-146** — orphan-DXIL mirror semantics in `Compile-HLSL.psm1` (the
  *delete* case for stale DXIL).
- **TASK-202** — same hazard on the `cmake --build` invocation path; not
  addressed by this change. `BuildWin.ps1` is the PowerShell driver; the
  cmake-direct path remains a separate fix surface.
- **TASK-213 CL C** — surfacing incident: HLSL edited 20:08, DXIL last
  compiled 20:03, PSO `E_INVALIDARG` at runtime until manual
  `HLSL2DXIL_NoPause.ps1` run.

## Script inventory

| Script                       | Role                                                                  |
|------------------------------|-----------------------------------------------------------------------|
| `BuildWin.ps1`               | Build Main + RenderTest. Compiles HLSL pre-step (this doc).           |
| `HLSL2DXIL.ps1`              | Interactive HLSL -> DXIL compile (ends with `Pause`).                 |
| `HLSL2DXIL_NoPause.ps1`      | CI / automation HLSL -> DXIL compile (no `Pause`).                    |
| `Lib/Compile-HLSL.psm1`      | Shared compile module — single source of truth for shader build policy. |
| `GenerateMetadataWin.ps1`    | Generate component metadata (parser pass).                            |
| `PostBuildWin.ps1`           | Post-build deploy / cleanup hooks.                                    |
| `StartEngineWin.ps1`         | Launch the runtime engine binary.                                     |
| `StartEditorWin.ps1`         | Launch the editor binary.                                             |
| `TestEditorComplete.ps1`     | Editor smoke test driver.                                             |
| `TestGIScene.ps1`            | GI scene capture / regression driver.                                 |
| `TestGPUPathTracer.ps1`      | GPU path tracer single-scene capture.                                 |
| `TestPathTracerThreeScenes.ps1` | Three-scene PT capture (TASK-213 family).                          |
| `VerifyEditorStack.ps1`      | Editor stack verification.                                            |
| `InteractiveTest.ps1`        | Interactive engine smoke test.                                        |
| `ConvertModels.ps1`          | Model converter wrapper.                                              |
| `DownloadAssets.ps1`         | Asset download / sync.                                                |
| `RegenClangdIndex.ps1`       | Regenerate clangd compile_commands.json index.                        |
| `PurgeStaleClangdIndex.ps1`  | Purge stale clangd index entries.                                     |
| `git-hooks/InstallHooks.ps1` | Install repo-local git hooks.                                         |
