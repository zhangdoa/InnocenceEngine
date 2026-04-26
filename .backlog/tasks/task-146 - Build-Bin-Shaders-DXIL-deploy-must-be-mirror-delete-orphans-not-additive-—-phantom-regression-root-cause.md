---
id: TASK-146
title: >-
  Build: Bin/Shaders/DXIL/ deploy must be mirror (delete-orphans), not additive
  — phantom-regression root cause
status: Done
assignee:
  - ci-build-expert
created_date: '2026-04-26 21:28'
updated_date: '2026-04-26 22:05'
labels:
  - build
  - infrastructure
  - bug
  - shaders
dependencies: []
references:
  - Scripts/Lib/Compile-HLSL.psm1
  - Scripts/HLSL2DXIL.ps1
  - Scripts/HLSL2DXIL_NoPause.ps1
  - CMake/DeployRuntimePayload.cmake
  - .claude/disciplines/regression-fix-flow.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Critical infrastructure bug discovered 2026-04-26 via bisect.**

The shader compile + deploy chain accumulates stale DXIL artifacts in `Bin/Shaders/DXIL/`. When source-tree HLSL files are deleted or renamed (e.g. revert of WIP work, branch switch, bisect), their previously-compiled `.dxil` files are NOT removed from the build cache. The deploy step then copies the entire (contaminated) cache to `Bin/RelWithDebInfo/Shaders/DXIL/`.

Worse: when the SAME shader name has different content across commits (e.g. `lightPass.comp.dxil` was the monolithic shader pre-TASK-135 and the post-split refactor post-TASK-135), incremental builds at an older commit may NOT recompile the shader (timestamp comparison says cached DXIL is newer), leaving the engine loading DXIL with bindings that don't match the engine binary's expectations → GPU device-hang or silent rendering corruption.

### How this caused a phantom regression chain

A multi-hour debug session (TASK-141 → TASK-142 → TASK-144 → TASK-145) chased a "sun shadow regression" on GISponza/UnitTest/GITestBox that turned out to be entirely build-cache contamination, NOT a source-code bug:

- TASK-138 WIP (later reverted) created new shader files (`SunShadowRT*.hlsl`); their `.dxil` linged in `Bin/Shaders/DXIL/` after revert
- Subsequent incremental builds at HEAD AND at older bisect commits (60c645a0, 723c94b4, d58b4fbf) inherited stale DXIL
- The d58b4fbf engine, attempting to load `lightPass.comp.dxil`, got the post-TASK-135 split version → binding mismatch → device-removed (`HRESULT=-2005270522`) → black screen
- Every "no shadow" / "dark image" / "fragmented shadow" user report during the session was contamination, not real

Once disciplined (nuke `Bin/Shaders/DXIL/` + `Bin/RelWithDebInfo/Shaders/DXIL/` + repopulate via `HLSL2DXIL_NoPause.ps1` + cmake build), every commit in the bisect chain showed shadows working correctly.

### Required fix

The shader compile/deploy chain must be **mirror semantics**: the deploy target (`Bin/RelWithDebInfo/Shaders/DXIL/`) should EXACTLY match the set of shaders compiled from current source, with orphans removed. Same for the upstream cache (`Bin/Shaders/DXIL/`).

Two implementation options:

1. **HLSL2DXIL_NoPause.ps1 nuke-on-start**: at the top of the script, delete `Bin/Shaders/DXIL/` before recompiling. Forces full rebuild every invocation; slow but bulletproof.
2. **Tracked-file delete pass**: script enumerates source HLSL files, computes expected DXIL filenames, deletes any DXIL in `Bin/Shaders/DXIL/` that doesn't have a corresponding source. Faster but more fragile.

Option 1 is the safest first cut. Option 2 is the optimization if rebuild time becomes a problem.

The CMake deploy step (currently `cmake -E copy_directory Bin/Shaders/DXIL Bin/RelWithDebInfo/Shaders/DXIL`) should similarly be replaced with a mirror operation: `cmake -E remove_directory <target>` before copy, OR use a sync-style command that removes orphans.

Same concern likely applies to `Bin/Data/` deploy chain — check whether deleted source-tree component / scene files persist in `Bin/Data/`.

### Why high priority

This bug has been latent since the build system was set up. Every shader-touching CL has been at risk of this contamination silently for unknown duration. The phantom regression chain that surfaced it cost hours of dispatch time and ~5 commits of speculative fixes before being caught. Without this fix, ANY future bisect on shader work has the same contamination risk.

### Owner

`ci-build-expert` — owns CMake + automation scripts.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 HLSL2DXIL_NoPause.ps1 (and Scripts/HLSL2DXIL.ps1) clean Bin/Shaders/DXIL/ at start, OR enumerate orphans and delete
- [x] #2 CMake deploy step uses mirror semantics (remove + copy, OR sync-with-delete) for Bin/RelWithDebInfo/Shaders/DXIL/
- [x] #3 Same audit done for Bin/Data/ deploy chain — orphans removed if any
- [x] #4 Validation: revert a known-modifying CL (e.g. delete a test .hlsl file from source), build, confirm Bin/Shaders/DXIL/ no longer contains the orphan .dxil
- [x] #5 Document the mirror semantics in the build script comments so a future regression of this rule is self-explanatory
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Audit (2026-04-26)**

Sources of orphan accumulation:

1. `Build/HLSL2DXIL_NoPause.ps1` — compiles HLSL → DXIL into `Bin/Shaders/DXIL/`. No orphan deletion. Source HLSL deleted/renamed → stale `.dxil` lingers.
2. `Scripts/HLSL2DXIL.ps1` — exact duplicate of #1, plus `Pause` at end. Same flaw.
3. `CMake/DeployRuntimePayload.cmake` — POST_BUILD copies `Bin/Shaders/DXIL` → `$<TARGET_FILE_DIR>/Shaders/DXIL` via `cmake -E copy_directory`. Additive copy. Orphans deleted upstream are NOT removed downstream.
4. Same `CMake/DeployRuntimePayload.cmake` copies `Data/<sub>/` → `Bin/Data/<sub>/` via `copy_directory`. Additive — stale assets accumulate when source-tree files renamed/deleted.

Current state on this checkout (HEAD, ecs-overhaul):
- `Bin/Shaders/DXIL/` 46 files vs source `Source/Shaders/HLSL/` 48 files — slight mismatch, likely benign extension-filter delta but worth verifying.
- `Bin/Data/UnitTest/` is missing despite `Data/UnitTest/` existing — confirms additive deploy is also "first-build-wins" (UnitTest must have appeared in source after the last full clean).

**Plan**

1. Add an "orphan delete pass" to BOTH PowerShell scripts (DRY: extract shared helper, since they are duplicates today). After enumerating source HLSL files, compute the expected set of `.dxil` filenames. Enumerate `.dxil` files in target dir; delete any not in the expected set. This is Option 2 in the task spec — precise, fast, preserves incremental rebuild for unchanged shaders. Add a `-Clean` switch for the nuke-everything path (Option 1) for paranoid bisects.
2. CMake deploy: replace each `copy_directory` site with mirror semantics. Use `cmake -E rm -rRf <dst>` followed by `cmake -E copy_directory`, OR — cleaner — `cmake -E copy_directory_if_different` is NOT mirror semantics (still additive). Mirror requires the explicit remove. So: pre-step `cmake -E rm -rRf <dst>`, then `cmake -E copy_directory`. POST_BUILD runs every build, which is fine — it's a copy of small files (DXIL ~few MB) plus the Engine/UnitTest/etc. data trees. Generated/ may be larger.
3. **Generated/ deserves special handling** — it's gitignored runtime-derived output (~1.2 GB); a full mirror that wipes-then-copies on every build is wasteful AND could race with engine writes. Audit whether Generated/ should be excluded from the mirror entirely (engine could read directly from `Data/Generated/` via path resolution rather than via deploy). Out of scope for AC#3 unless it shows up as a regression.
4. Validation (AC#4): create `Source/Shaders/HLSL/__Task146Test.comp` (a trivial compute shader), build, confirm `__Task146Test.comp.dxil` appears in `Bin/Shaders/DXIL/`. Then delete the source file, run shader compile script, confirm orphan is removed. Restore final state by deleting the test file before commit.
5. Document mirror semantics in script + CMake comments (AC#5).

**Layering note**: PowerShell helper consolidation belongs in a shared module (`Scripts/Lib/ShaderCompile.psm1` or similar) so there's one source of truth. The duplicate file `Build/HLSL2DXIL_NoPause.ps1` should ideally be deleted entirely and `Scripts/HLSL2DXIL.ps1` parameterized for the no-pause case (`-NoPause` switch). However, that touches BuildWin.ps1's invocation surface and may be out of scope for this task — defer to backlog if it surfaces.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## What landed

Mirror semantics enforced at every layer of the shader + asset deploy chain. Stale `.dxil` and `Bin/Data/<sub>` artifacts no longer survive a source-tree deletion / branch switch / bisect step.

### Files

- **NEW** `Scripts/Lib/Compile-HLSL.psm1` — single source of truth for HLSL → DXIL compile. Enumerates source HLSL files, computes expected `.dxil` set, deletes orphans BEFORE compile (Option 2 from task spec). `-FullClean` switch wipes the whole dir for paranoid bisects (Option 1).
- **REWRITTEN** `Scripts/HLSL2DXIL.ps1` — 24-line wrapper delegating to module; preserves trailing `Pause` for interactive use.
- **NEW (tracked)** `Scripts/HLSL2DXIL_NoPause.ps1` — 22-line wrapper, no `Pause`, for CI/automation. Replaces the previously-untracked `Build/HLSL2DXIL_NoPause.ps1` (Build/ is gitignored, so the file never survived a fresh checkout — a latent bug discovered during AC#1 validation).
- **REMOVED** `Build/HLSL2DXIL_NoPause.ps1` from local checkout (was untracked, unsurvivable on fresh clone).
- **REWRITTEN** `CMake/DeployRuntimePayload.cmake` — POST_BUILD steps now do `cmake -E rm -rRf <dst>` before `copy_directory <src> <dst>` for both DXIL and Data subdirs. Three handling tiers: required (Engine — fatal if absent), optional (ExampleProject/UnitTest/Components — wiped if source absent), Generated/ (gitignored runtime-derived ~1 GB+ — kept additive, never auto-wiped).
- **UPDATED** `.claude/disciplines/regression-fix-flow.md` — replaced the "manual nuke required" workaround with a "TASK-146 has landed; build is mirror-semantic" section. `-FullClean` documented as paranoia opt-in.
- **UPDATED** `.backlog/tasks/task-147` and `task-148` — replaced the in-flight "manual nuke" notes with the new state.

### Validation transcript (AC#4 — end-to-end mirror semantics)

Created `Source/Shaders/HLSL/__Task146Test.comp` (trivial no-op CS), then:

1. Compile via `Scripts/HLSL2DXIL_NoPause.ps1` → `Bin/Shaders/DXIL/__Task146Test.comp.dxil` PRESENT.
2. `cmake --build Build --config RelWithDebInfo --target Main` → `Bin/RelWithDebInfo/Shaders/DXIL/__Task146Test.comp.dxil` PRESENT (mirror-deploy fired: `Mirror-deploying DXIL shaders -> .../Bin/RelWithDebInfo/Shaders/DXIL (wipe + copy)`).
3. Delete `Source/Shaders/HLSL/__Task146Test.comp`.
4. Re-run `Scripts/HLSL2DXIL_NoPause.ps1` → script logs `Removing orphan DXIL (no matching source): __Task146Test.comp.dxil` / `Removed 1 orphan DXIL file(s).` Upstream cache CLEAN.
5. `cmake --build Build --config RelWithDebInfo --target Main` → deploy target CLEAN.
6. Final state: `Bin/Shaders/DXIL/` 46 files, `Bin/RelWithDebInfo/Shaders/DXIL/` 46 files, `diff` = empty (byte-identical sets).

`-FullClean` validated separately: 46-file dir wiped, 46 fresh DXIL emitted (full rebuild from clean state).

### Build / runtime green (DoD #1, #2, #5)

- `cmake --build Build --config RelWithDebInfo --target Main` → `Main.vcxproj -> Bin/RelWithDebInfo/Main.exe`, exit 0.
- `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0 (engine boots, runs 30 frames, terminates cleanly with all worker threads released).
- `RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30 -test bareboot` exit 0.
- `RenderTest.exe ... -test draw_instanced` exit 0 (loads compiled shaders + executes a draw — actual rendering path exercises the deploy chain).

### What was NOT verified (DoD #6)

- **Vulkan / `Scripts/HLSL2SPIR-V.ps1`** — out of scope. Same orphan-accumulation class-of-bug likely exists in the SPIR-V compile path, but DX12 was the cause-of-record for TASK-141..145 and this task's ACs scope to DXIL. Filed as follow-up consideration; not committed here.
- **`Scripts/HLSL2DXIL.ps1` interactive variant** — only the no-pause path was validated end-to-end (compile + delete orphan + reverify) because Pause requires an interactive session. The interactive variant is a 24-line wrapper around the same module and was smoke-run (it compiled the test shader and reported expected output) but the orphan-delete validation only ran via the no-pause path.
- **Multi-config builds** — only `RelWithDebInfo` was exercised. The CMake POST_BUILD uses `$<TARGET_FILE_DIR:${target}>` which is generator-expression-correct for Debug/Release/RelWithDebInfo, but only one config was built and validated this session.
- **Visual A/B / RenderDoc capture** — no rendering-output regression test was performed because the change is build-system-only and has no shader-content effect. The acceptance signal is "engine boots and runs the same shaders to the same exit code with the new deploy machinery"; that was confirmed.
- **First-time fresh-checkout flow** — not validated end-to-end. The fix REMOVES a fresh-checkout hazard (the untracked `Build/HLSL2DXIL_NoPause.ps1`) but a literal `git clone` + build smoke test was not re-run — the existing checkout was used.
- **Generated/ growth** — kept additive intentionally. This means orphan files in `Data/Generated/<sub>/` (e.g., a removed component's serialized texture) will still survive a deploy. This is a deliberate deferral, not a bug; if it surfaces as a contamination class later, the fix is to add `Generated/` to the wiped tier with a separate `inno_clean_generated` target users can opt into.

### Structural retrospective

- **The duplicate scripts were a smell, not just a duplication issue** — they masked the deeper problem that `Build/HLSL2DXIL_NoPause.ps1` was untracked. The shared module dissolves both at once.
- **Mirror semantics is a layering principle, not just a script feature** — Option 2 (orphan-delete pass) and CMake `rm -rRf <dst> ; copy_directory` are the same pattern at two layers. Naming and comments at each layer explicitly call out "mirror semantics" so the rule is self-documenting and cannot regress without an obvious diff.
- **`Generated/` opt-out documents the cost ceiling** — wiping ~1 GB on every build would be wasteful and would race with engine writes. Encoding the policy in the CMake module (with a comment explaining why) is more discoverable than a TODO buried in a backlog task.
- **The whole class of bug was avoidable from day one** — `cmake -E copy_directory` is documented as additive; the original choice trusted the source tree to be append-only. As soon as a revert / branch switch / bisect happened, the assumption broke. New build infrastructure should default to mirror semantics unless there's a specific reason (size, race-safety) to be additive — and that reason should be in the comment.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
