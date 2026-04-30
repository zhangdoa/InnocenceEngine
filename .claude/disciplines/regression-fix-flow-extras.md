# Regression-fix-flow extras

Long-form companion to `regression-fix-flow.md`. Read on demand. Operative core (mandatory bisect, user-as-only-verifier, build-cache triage) is in the short file.

## Why (full prose)

Every speculative fix is technical debt against the eventual real fix. The user surfaced this concretely: *"we have been violating regression fix flows."*

A fix dispatched on top of an unreproduced regression piles new variables onto an already-uncertain baseline, contaminates the bisect surface, and makes the actual root cause harder to find.

## clangd index contamination — full triage

**As of TASK-151 (landed 2026-04-26), the clangd index is mirror-semantic.** `Scripts/PurgeStaleClangdIndex.ps1` deletes any `.cache/clangd/index/<basename>.<hash>.idx` whose primary source path no longer exists on disk. The script runs automatically via `Scripts/git-hooks/post-checkout` and `post-merge` (installed by `Scripts/git-hooks/InstallHooks.ps1`), and is also invoked at the tail of `Scripts/RegenClangdIndex.ps1`.

The analog to TASK-146's stale `.dxil` failure mode is "ghost diagnostics": clangd reports errors against deleted files because their `.idx` entries linger and cross-reference live source. Symptoms:

- `'<file>.h' file not found` on tracked files that don't `#include` it.
- `Use of undeclared identifier 'X'` at lines whose actual content does not reference X.
- False inheritance / template errors on classes that are clean in the live source.

**Real `cmake --build` is unaffected** — this is purely IDE noise. But the noise wastes triage time and can mask real diagnostics; the source is fixed rather than learned-around.

### Triage path when you see a clangd diagnostic that looks suspicious

Each step is cheap; do all of them, in order, before reading source:

1. `git ls-files <file>` — if the file is not tracked, the diagnostic is stale.
2. `Grep <symbol>` over `Source/` — if zero hits in tracked source, the diagnostic is stale.
3. `Read <file>:<line>` — if the actual line content is unrelated to the reported error (e.g. line 13 is `#include "OtherFile.h"`, not `#include "GhostFile.h"`), the diagnostic is stale.

If any of those say "stale," run `Scripts/PurgeStaleClangdIndex.ps1` (or trigger any `git checkout` to fire the post-checkout hook) and re-check. Restart clangd (or reload the IDE window) to evict any in-memory state the persistent purge can't reach.

### When the automation is not enough

The hooks fire on `git checkout`, `git switch`, `git merge`, `git pull`. They do NOT fire on:

- `git restore <file>` (no checkout-event in some git versions).
- `rm <file>` outside of git (deleting a file directly from the working tree).
- Branch-switch via tools other than git (rare).

In those cases, run `Scripts/PurgeStaleClangdIndex.ps1` manually. The script is fast (<1s for ~850 idx entries) and idempotent.

## Recorded incident (full)

**TASK-122 → TASK-141 → TASK-142 → TASK-145** (2026-04-26). User reported "GISponza pastel" after the ACES → AGX swap (TASK-122). The dispatcher shipped TASK-141 (AGX matrix + encoding), then TASK-142 (K retune), then TASK-145 (diagnostic — against the regressed state, no baseline). When the user reported shadows totally gone, the bisect surface had been contaminated by ~5 commits' worth of accumulated changes; the diagnostic could not isolate root cause because there was no comparison anchor. User: *"we have been violating regression fix flows."*

**The deeper finding:** once the discipline was applied and a proper bisect was driven, the "shadows gone" regression turned out to be **entirely build-cache contamination** (TASK-146) — not a source-code bug at all. Stale `Bin/Shaders/DXIL/` artifacts from a TASK-138 WIP that had been reverted earlier in the session were polluting every incremental build. Once the cache was nuked + repopulated at HEAD, shadows worked. The phantom regression had cost ~5 speculative-fix commits + multiple agent dispatches before being caught.

The double lesson: (a) bisect first or chase phantoms; (b) when bisect is followed, infrastructure failures (build-cache contamination, etc.) become tractable — they reveal themselves quickly because every step has a known anchor.

## Anti-patterns (full)

- **Diagnostic dispatch against the regressed state with no baseline anchor.** The agent confirms the symptom but cannot identify the breaking commit because there is no comparison point. Its conclusions are perforce hypothetical.
- **Shipping a "fix" before the breaking commit is identified.** Each speculative fix introduces new variables (matrices, constants, code paths) that mask the original bug and corrupt the bisect surface.
- **Parallel agents touching adjacent files during a regression.** Each parallel CL adds another commit to the bisect range and another set of changes to the working tree. Serialize the work or pause it.
- **Treating user reports as bug-fix tickets.** "The brightness is off, ship a brightness fix" is wrong. "The brightness is off, find when it changed" is right.

## Cross-references

- `perf-measurement-frame-budget.md` — paired discipline: that one governs per-bisect-step cost (pick N for the perf bucket); together they make bisect cheap enough to actually run.
- `test-etiquette.md` — bisect steps require many user-verified launches; "make each step cheap" applies to launch budget too.
- `tech-choice-vs-default.md` — the *understand why* step often surfaces a wrong-tech-pick that should have been caught before ship; that discipline is the design-time form.
