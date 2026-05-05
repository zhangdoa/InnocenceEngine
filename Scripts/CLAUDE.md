Owned by the `ci-build-expert` agent (build, shader-compile, test-runner, automation scripts). See `.claude/agents/ci-build-expert.md`.

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

## BuildWin.ps1 — clangd index refresh policy (TASK-199)

`BuildWin.ps1` invokes `RegenClangdIndex.ps1` *after* `msbuild` succeeds.
The Visual Studio generator that drives msbuild does not emit
`compile_commands.json`; the regen script runs a parallel Ninja configure
(no compile) to produce it, then chains `PurgeStaleClangdIndex.ps1` to
drop `.idx` cache entries for sources deleted from the tree.

### Why auto-trigger (option a), not a manual SOP (option c)

The task offered three paths:

- **(a) Build-time auto-trigger** — `BuildWin.ps1` calls
  `RegenClangdIndex.ps1` post-build; the regen script reconfigures CMake
  and chains the orphan-purge.
- **(b) `.clangd` config tweak** — set an "index-purge-on-mismatch" or
  similar directive.
- **(c) Manual reset SOP** — document a sequence the developer runs
  after deletions / branch switches.

Option (b) was rejected because no such `.clangd` directive exists.
clangd's `Index:` block has `Background: Build`, `External:`, and
`StandardLibrary:` knobs but nothing that purges cached translation
units when the underlying source disappears or when CDB compile flags
change. The actual root cause was a stale `compile_commands.json` at
the repo root (dated 2026-04-19; deletions in TASK-138 phase 2 and
TASK-177 happened later in May), so the CDB still listed
`SunShadowGeometryProcessPass.cpp` and `PointShadowGeometryProcessPass.cpp`
as TUs to compile. No `.clangd` directive can fix a stale CDB.

Option (a) was chosen over (c) for the same reason auto-trigger beat
loud-fail in the DXIL section: the existing scripts are idempotent,
the cost on a no-change cycle is bounded (~10 s end-to-end on this
machine: `VsDevCmd.bat`'s MSVC-environment probe runs unconditionally
at ~6 s, plus a ~4 s CMake reconfigure that stamps unchanged build
files — the configure phase is fast because CMake skips regeneration
when the input tree is unchanged, but the VsDevCmd probe is not
short-circuitable), and a manual SOP rots — the team learns to
ignore the diagnostic noise instead of running the command.
`disciplines/always/feedback_no_dismissing_tool_noise.md`
operationalises the rule. Option (b) has no working knob to compare
against in the first place; option (c)'s SOP rots regardless of how
fast the underlying command would run, so the cost differential
between (a) and (c) on a no-change cycle is not the deciding axis.

### What gets refreshed

`RegenClangdIndex.ps1`:

1. Runs `cmake -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` against
   `Build/clangd/`. CMake walks the live `CMakeLists.txt` tree and emits
   a `compile_commands.json` matching the on-disk source set.
2. Copies `Build/clangd/compile_commands.json` to the repo root where
   clangd's upward search picks it up.
3. Invokes `PurgeStaleClangdIndex.ps1` (no flags) — drops `.idx` files
   whose primary source URI no longer exists on disk.

### Failure handling

The regen step runs only when msbuild succeeded. If regen itself exits
non-zero (rare: CMake configure failure on a transient state), the
script emits a `Write-Warning` and lets the build's exit code stand —
the engine binary is fine; the clangd index is just stale until the
next refresh.

### Escape hatches

- `BuildWin.ps1 -SkipClangdIndexRefresh` skips the post-step. Reserved
  for working without a clangd-aware editor or bisecting a CMake
  configure issue.
- `Scripts/PurgeStaleClangdIndex.ps1 -PurgeAll` nukes every `.idx` file.
  The orphan-source heuristic in the no-flag mode cannot detect a TU
  whose source still exists but whose cached preprocessor / token state
  is corrupt (observed once on `Engine.cpp` —
  "Pasting formed '<HIDService' invalid preprocessing token" on lines
  that compile cleanly). `-PurgeAll` is the deterministic recovery: drop
  every cached TU and let clangd rebuild from scratch in the background.
  Costs minutes of CPU off the user's interactive path.

### Related work

- **TASK-146** — orphan-DXIL mirror semantics; the precedent for
  "delete the artefact when the source is gone."
- **TASK-151** — original wiring of `PurgeStaleClangdIndex.ps1` into
  `RegenClangdIndex.ps1` (the orphan-source heuristic itself).
- **TASK-202** — same hazard on the `cmake --build` invocation path;
  not addressed by this change. `BuildWin.ps1` is the PowerShell driver;
  the cmake-direct path bypasses both this auto-trigger and the DXIL
  one (parallel surface), and remains a separate fix surface.
- **TASK-214** — DXIL pre-step auto-trigger; same auto-trigger
  rationale, different artefact.

## PowerShell 5.1 string conventions

Files in this directory are read by Windows PowerShell 5.1
(`powershell.exe`), which decodes `.ps1` source as Windows-1252 unless
a UTF-8 BOM is present. None of the scripts here carry a BOM.

Practical rule: **double-quoted strings in `.ps1` files must be
pure ASCII.** A non-ASCII byte (em-dash, smart quotes, etc.) inside a
double-quoted string gets re-decoded through CP1252 and can mangle the
string-terminator byte, producing cascading parser errors that look
unrelated to the actual line.

Em-dashes, smart quotes, and other non-ASCII glyphs are safe in:

- `# comment lines`
- `'single-quoted strings'` (the parser doesn't process content)
- here-docs and other non-evaluated literals

Failure mode: `Write-Warning "engine build is fine — index may be stale"`
in PS 5.1 + UTF-8-without-BOM source produces a "string is missing the
terminator" parse error with bracket-mismatch cascades. Replace the
em-dash with `.` (period), `:`, or a single-quoted concatenation
(`"... " + 'fine — stale.'`).

This applies to every `.ps1` file in this tree until the project moves
to PowerShell 7+ or commits to UTF-8-with-BOM source encoding.

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
</content>
</invoke>