---
id: TASK-200
title: Res/Shaders/ layout inconsistency — orphan dir tracked by old build scripts
status: To Do
assignee: []
created_date: '2026-04-29 07:19'
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

- [ ] `Res/` no longer appears as untracked in `git status` after a clean build
- [ ] Either `Scripts/HLSL2SPIR-V.ps1` and `Scripts/SPIR-V2HLSL.bat` are confirmed active and their output path is `.gitignore`'d, OR they are deleted as stale
- [ ] Stash-pop from old branches no longer surfaces `Res/Shaders/*` as conflicting paths in current HEAD
- [ ] `docs/superpowers/` references audited — outdated mentions removed or annotated

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
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
