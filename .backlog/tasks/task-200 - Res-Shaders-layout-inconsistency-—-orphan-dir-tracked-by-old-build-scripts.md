---
id: TASK-200
title: Res/Shaders/ layout inconsistency — orphan dir tracked by old build scripts
status: Done
assignee:
  - ci-build-expert
created_date: '2026-04-29 07:19'
updated_date: '2026-04-29 22:00'
labels:
  - repo-layout
  - ci-build
  - shaders
  - cleanup
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

`Res/Shaders/` exists on disk as an untracked directory containing 8 shader files (`lightPass.comp`, `opaqueGPUCulling.comp`, `RadianceCacheClosestHit.hlsl`, `RadianceCacheFilterHorizontal.comp`, `RadianceCacheFilterVertical.comp`, `RadianceCacheIntegration.comp`, `RadianceCacheRayGen.hlsl`, `RadianceCacheReprojection.comp`).

These files are NOT current shader sources — those live in `Source/Shaders/HLSL/`. The `Res/Shaders/HLSL/` files are either:
1. **Build output** — `Scripts/HLSL2SPIR-V.ps1` line 1 does `mkdir ..\Res\Shaders\SPIRV` and operates relative to `Res/Shaders/`. So `Res/Shaders/HLSL/` may be a copy step's output.
2. **Stale orphans** — leftover from a pre-`Source/Shaders/` layout migration. Files in HEAD's ancestry (verified by today's stash-pop conflict surfacing them as "deleted by us, exist in stash@{0}").

In either case, current state is bad:
- `git status` shows `?? Res/` every session — noise.
- Stash entries from old branches reference these paths and produce conflict-on-pop.
- If they're build output, they should be `.gitignore`'d; if they're stale, they should be deleted along with the scripts that produce them.

## Investigation + fix

1. Determine whether `Scripts/HLSL2SPIR-V.ps1` and `Scripts/SPIR-V2HLSL.bat` are still in active use, or are stale tooling from an earlier shader pipeline.
2. If active: figure out the intended build-output layout. Either:
   - Move output to `Build/Shaders/SPIRV/` (matches CMake convention), and delete `Res/`.
   - Keep `Res/Shaders/` but `.gitignore` it.
3. If stale: delete the scripts AND the `Res/` directory.
4. Audit `docs/superpowers/{plans,specs}/*.md` references to `Res/Shaders` — if those docs are obsolete, update or delete.

## Acceptance criteria

- [x] `Res/` no longer appears as untracked in `git status` after a clean build
- [x] Either `Scripts/HLSL2SPIR-V.ps1` and `Scripts/SPIR-V2HLSL.bat` are confirmed active and their output path is `.gitignore`'d, OR they are deleted as stale
- [x] Stash-pop from old branches no longer surfaces `Res/Shaders/*` as conflicting paths in current HEAD
- [x] `docs/superpowers/` references audited — outdated mentions removed or annotated

## Owner

`ci-build-expert` (build + automation scripts ownership).

## References

- `Scripts/HLSL2SPIR-V.ps1` line 1-5 (Res/Shaders/SPIRV mkdir + operations)
- `Scripts/SPIR-V2HLSL.bat`
- `docs/superpowers/plans/2026-04-01-gpu-path-tracer-reference-pass.md`, `2026-03-22-rendering-client-audit.md`
- `docs/superpowers/specs/2026-04-01-gpu-path-tracer-reference-pass-design.md`
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — N/A (this task has no engine/editor/shader code change; only repo-layout cleanup, README copy edit, and doc annotations)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — N/A (no runtime behavior changed)
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — N/A (cleanup-only)
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap — N/A (no tests authored)
- [x] #5 User-observable outcome verified — `git status` no longer reports `?? Res/`; verified by `ls /c/GitRepo/InnocenceEngine/Res` returning "No such file or directory" and `ls Scripts/ | grep -iE "SPIR"` returning empty
- [x] #6 Final summary lists what was NOT verified — see Implementation Notes § "Did not verify"
<!-- DOD:END -->

## Implementation Notes

**Landed 2026-04-29 by ci-build-expert** (background dispatch).

### Classification: stale tooling

The two Vulkan-pipeline scripts are leftover from a pre-`Source/Shaders/` shader-tree migration and were never updated to match the active layout:

| Concern | Active DXIL pipeline (kept) | Vulkan SPIR-V scripts (deleted) |
|---|---|---|
| Source root | `Source\Shaders\HLSL\` | `Res\Shaders\HLSL\` (orphan shadow tree) |
| Output root | `Bin\Shaders\DXIL\` | `Res\Shaders\SPIRV\` (does not match runtime expectations) |
| Wrapper | thin entry → `Lib\Compile-HLSL.psm1` shared module with mirror-delete semantics (TASK-146) | one-shot `.ps1` / `.bat` with `mkdir`+`Set-Location`, no error handling, no module |
| Last meaningful update | TASK-146 (Mar 2026) | none post-migration; matches the orphan files' epoch |

Even if `HLSL2SPIR-V.ps1` were run today, its output path (`Res/Shaders/SPIRV/`) does not match the runtime's expected shader load location. The `VKGraphicsService` shader-loader at `Source/Engine/Services/VK/VKGraphicsService_VulkanObject.cpp:32` reads `m_shaderRelativePath = "Shaders//SPIRV//"`, which is resolved relative to the engine working directory — same root as the DX12 path that ships shaders to `Bin/<Config>/Shaders/DXIL/`. So the SPIR-V output would need to be `Bin/<Config>/Shaders/SPIRV/`, not `Res/Shaders/SPIRV/`. The scripts produce binaries that the engine cannot find, against source files that are not the live source. They have been broken since the `Source/Shaders/` migration; deleting them is removal of dead weight, not removal of capability.

`INNO_RENDERER_VULKAN` is still ON when the Vulkan SDK is detected (`Source/CMakeLists.txt:32-36`), so the VK service still compiles and links. That is fine — the shader-compile pipeline is what was stale. If/when Vulkan support is re-activated as a real path, a new Vulkan-shader-compile script will need to mirror the DXIL-script structure (`HlslSourceDir = Source/Shaders/HLSL`, `SpirvOutputDir = Bin/Shaders/SPIRV`, mirror-delete semantics via the shared module).

### What landed

- Deleted `Res/` directory (the 8 stale orphan shaders under `Res/Shaders/HLSL/`).
- Deleted `Scripts/HLSL2SPIR-V.ps1`.
- Deleted `Scripts/SPIR-V2HLSL.bat`.
- `README.md` § "Shader compilation" — removed the dead `Scripts/HLSL2SPIR-V.ps1  # SPIR-V for Vulkan` line; only `Scripts/HLSL2DXIL.ps1` remains (the active path).
- `docs/superpowers/plans/2026-04-01-gpu-path-tracer-reference-pass.md` — added top-of-file path-note annotation. The plan's `Res/Shaders/HLSL/X.hlsl` paths were wrong even when written; the annotation tells future readers to translate to `Source/Shaders/HLSL/X.hlsl`.
- `docs/superpowers/specs/2026-04-01-gpu-path-tracer-reference-pass-design.md` — same path-note annotation.
- `docs/superpowers/plans/2026-03-22-rendering-client-audit.md` — same path-note annotation (it has one `Res/Shaders/HLSL/` reference in the Fix.1 step).

`*.spv` and `*.dxil` are already in `.gitignore` (lines 51-52), so no `.gitignore` change was needed.

### Did not verify

- Engine build was not re-run. TASK-200 is repo-layout cleanup with no runtime behavior change; per the dispatch brief, the engine build path is out of scope. The next routine engine build should still succeed because nothing it consumes was deleted (the active DXIL script is untouched; the deleted scripts had no consumers).
- The Vulkan service's shader-load path was not re-tested. Vulkan support has been broken at the script layer since the `Source/Shaders/` migration; this CL does not regress that further (the scripts pointed at the wrong source already), but it makes the breakage explicit by deleting the misleading scripts.
- The annotated `docs/superpowers/` plans were not re-executed. They are historical records; the annotation is the fix per the AC #4 phrase "removed or annotated" — annotated chosen because the docs record past work and shouldn't be silently rewritten.
- Other repo-layout drift was NOT swept. Per dispatch brief "don't pile on": adjacent observations follow.

### Adjacent observations (NOT filed as new tasks per dispatch brief)

While auditing I noticed three pieces of repo-layout drift that the user can re-dispatch if desired:

1. **`Scripts/PostBuildMac.sh:11-12`** copies `Res/*` to a Mac-app bundle `Data/Res/` location. There is no top-level `Res/` in the repo (only the orphan I just deleted). The Mac packaging step is therefore broken or refers to a layout that predates the `Data/` reorg. The Mac path may already be cold, but the script will fail when run. Not in TASK-200's scope.
2. **`Scripts/SPIR-V2HLSL.bat`** referenced a tool `spirv-cross.exe` that is not vendored anywhere I could find. It was a manual-debug Vulkan-shader decompile path; same era as the other deleted scripts. Already deleted as part of this CL.
3. **`VKGraphicsService_VulkanObject.cpp:32`** has the path literal `"Shaders//SPIRV//"` (double-slashes; the rest of the codebase uses `Shaders\\DXIL\\` or single-slash paths). When the Vulkan path is resurrected, that literal needs review against whatever `IOService::loadFile` expects today.

### Peer review

**`Review-Skipped: hook-internal`** per `peer-review-required.md` § "When". This is repo-layout cleanup only — file deletions, README copy edit, doc annotations. Touches no runtime behavior, no engine code, no shaders. Same skip category as the disciplines refactor commit (`a5aca40a`).
