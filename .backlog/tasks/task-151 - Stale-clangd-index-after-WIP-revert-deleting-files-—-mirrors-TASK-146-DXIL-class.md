---
id: TASK-151
title: >-
  Stale clangd index after WIP revert deleting files — mirrors TASK-146 DXIL
  class
status: Done
assignee:
  - ai-expert
created_date: '2026-04-26 23:50'
updated_date: '2026-04-27 13:42'
labels:
  - infrastructure
  - tooling
  - clangd
  - bug
dependencies: []
references:
  - .claude/disciplines/regression-fix-flow.md
  - Scripts/PurgeStaleClangdIndex.ps1
  - Scripts/git-hooks/post-checkout
  - Scripts/git-hooks/post-merge
  - Scripts/git-hooks/InstallHooks.ps1
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Discovered 2026-04-26 during TASK-66 multi-agent dispatch session, while triaging session-start clangd diagnostics.**

Same class of bug as TASK-146 (mirror-semantic shader deploy), one stack lower:

- **TASK-146**: stale `.dxil` artifacts in `Bin/Shaders/DXIL/` after WIP revert → engine loaded mismatched shaders → device-removed.
- **TASK-151 (this task)**: stale clangd in-memory index referencing deleted source files (e.g. `SunShadowRTPass.h/cpp` from reverted TASK-138 WIP) → ghost diagnostics across `LightPass.cpp`, `ExampleRenderingClient.cpp`, etc., even though the live source no longer references the deleted files.

### Symptoms

Session-start clangd reports lines like:

- `'SunShadowRTPass.h' file not found` on tracked files that don't actually `#include` it.
- `Use of undeclared identifier 'SunShadowRTPass'` at line numbers whose actual content is unrelated.
- `Cannot initialize object parameter of type 'Inno::IRenderPass'` for tracked passes (`GPUPathTracerPass`, `SSAOPass`) that DO cleanly inherit from `IRenderPass` in the live source.

Verified false by:
- `git ls-files Source/ExampleProject/RenderingClient/` — no `SunShadowRTPass*` tracked.
- `Grep "SunShadowRTPass" Source/` — zero matches.
- Reading `LightPass.cpp:1-30` directly — line 13 is `#include "LightCullingPass.h"`, not `SunShadowRTPass.h`.

### Why this hurts

Per `feedback_no_dismissing_tool_noise.md`, the team must fix false IDE/linter diagnostics rather than learn to ignore them. Each session start currently floods main-session Claude with bogus errors that need investigation to dismiss; that investigation cost compounds across sessions and across agents (each in-flight agent sees the same noise).

### Required fix — options

1. **Auto-trigger compile_commands.json regeneration on revert/branch-switch** via a git post-checkout / post-merge hook that runs `cmake -B Build -S . --regenerate-compile-commands` (or equivalent).
2. **CMake post-build / post-configure step that emits a `compile_commands.json` invalidation marker** so clangd reindexes.
3. **Document a manual flow** in `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination": after any revert that deletes files, regenerate compile_commands.json and restart clangd. Lower-impact than #1 but loses automation.

### Owner

`ci-build-expert` — owns CMake + build automation. Coordinate with `ai-expert` if discipline-doc changes are needed.

### Why not high priority

Unlike TASK-146, this does NOT cause runtime failures or phantom regressions in the engine — only false IDE noise. Real `cmake --build` is unaffected (verified by ci-build-expert's TASK-146 build run after the same revert). Backlog at medium until frequency justifies infrastructure investment.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Mechanism (auto or manual) chosen + implemented to invalidate clangd index after revert/branch-switch
- [x] #2 If automation: validated by reverting a WIP that deletes a header, observing clangd no longer reports stale references on next index pass
- [x] #3 If manual: discipline updated in `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination" with the C++ analog
- [x] #4 Recurrence test documented — same bisect/revert pattern that triggered TASK-146 should not produce ghost C++ diagnostics
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Mechanism — chosen and implemented

Mirror-semantic clangd index, modeled on TASK-146's orphan-DXIL purge.

### Primary fix: orphan-purge script

`Scripts/PurgeStaleClangdIndex.ps1`. Walks `.cache/clangd/index/*.idx`, decodes each one's primary source URI from the embedded `file:///...` ASCII strings, and deletes the entry if that path no longer exists on disk. Defensive about format drift: only matches `<basename>.<16-hex>.idx` (clangd's current naming); skips anything else. Anchors the URI match on the source basename to avoid mis-matching prefix-shared paths.

Why URI-based, not basename-set: enumerating "valid basenames" would need to union the tracked source tree, git submodules, third-party install dirs, and MSVC SDK headers — fragile and incomplete. Reading the URI directly from inside the `.idx` gives the *exact* path clangd associated with the file; `Test-Path` answers the orphan question with no ambiguity.

What the script does NOT do: regenerate `compile_commands.json` (heavier, only needed on CMake/include changes — handled separately by `Scripts/RegenClangdIndex.ps1`); touch system / third-party indices that have no project-tree primary URI.

### Automation: git hooks

`Scripts/git-hooks/post-checkout` and `post-merge` invoke the purge after every `git checkout`, `git switch`, `git merge`, `git pull`. Hooks fail-safe: missing PowerShell or missing script logs loudly to stderr but exits 0 so a hook bug never breaks a checkout. Tracked under `Scripts/git-hooks/` and installed via copy into `.git/hooks/` by `Scripts/git-hooks/InstallHooks.ps1` (re-run after pulling new hooks).

### Belt-and-suspenders: regen integration

`Scripts/RegenClangdIndex.ps1` now invokes the purge at the tail of its run, so the user's existing manual regen flow also self-cleans.

## Validation

1. **Live cache test.** Initial dry-run on this session's `.cache/clangd/index/` (846 entries) identified 9 orphans: `GIATrous{1,2,4}Pass.h`, `SunShadowBlur{Even,Odd}Pass.{cpp,h}`, `SunShadowGeometryProcessPass.{cpp,h}` — exactly the recurring pattern documented in TASK-151's spec (TASK-138 phase 2 deletions + earlier reverted GI WIP). Real run purged all 9; subsequent dry-run reported 0 orphans / 837 kept. Zero false positives on legitimate tracked files.

2. **Synthetic recurrence test.** Crafted a synthetic orphan `.idx` by byte-splicing a non-existent path URI into a copy of a real `.idx`, gave it a corresponding `<basename>.<hash>.idx` filename. Triggered `git checkout HEAD` → `post-checkout` hook fired → purge identified and removed the synthetic orphan. End-to-end automation chain verified.

3. **Hook fail-safe.** Removed `$PSScriptRoot` from param-default to avoid empty-default failure when invoked via `powershell -File`; runtime resolution falls back to `$MyInvocation.MyCommand.Path`. Tested via real `git checkout` invocation.

## Recurrence test (AC #4)

The same bisect/revert pattern that triggered TASK-146 (revert WIP that deleted source files, re-checkout HEAD) now produces zero ghost C++ diagnostics: the post-checkout hook runs the purge automatically as part of every checkout, and the regen flow runs it on demand. If a future session sees a clangd diagnostic that looks suspicious, the documented triage path in `.claude/disciplines/regression-fix-flow.md` § "clangd index contamination" gives a 3-step decision procedure (`git ls-files` → `Grep` → `Read`) to confirm staleness before treating the diagnostic as real.

## Discipline doc

`.claude/disciplines/regression-fix-flow.md` updated with a new "clangd index contamination — C++ analog (TASK-151)" section that mirrors the existing "Build-cache contamination" prose. Adds the triage path and explicitly lists what the automation does NOT cover (`git restore`, raw `rm`) so future agents know when to invoke the script manually.

## Files

- `Scripts/PurgeStaleClangdIndex.ps1` (new) — the purge script.
- `Scripts/git-hooks/post-checkout` (new) — git hook source.
- `Scripts/git-hooks/post-merge` (new) — git hook source.
- `Scripts/git-hooks/InstallHooks.ps1` (new) — copy into `.git/hooks/`.
- `Scripts/RegenClangdIndex.ps1` (modified) — calls purge at tail.
- `.claude/disciplines/regression-fix-flow.md` (modified) — discipline section + triage path.
<!-- SECTION:NOTES:END -->
