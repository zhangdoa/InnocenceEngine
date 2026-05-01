# Discipline: regression-fix-flow

When the user reports *"X used to work, now it's broken"*, the first action is to identify the breaking commit, not to dispatch a fix. A speculative fix on top of an unreproduced regression contaminates the bisect surface.

## Mandatory sequence

1. **Confirm** — verify the symptom exists at current HEAD; one user-confirmed run.
2. **Baseline** — find a known-good prior commit; build, run, user confirms. Non-negotiable.
3. **Bisect** — `git bisect start <bad> <good>`; the dispatcher drives, the user observes. Pre-build each candidate; one yes/no question per step.
4. **Identify the breaking commit** — read its diff; the bug is usually small once the commit is known.
5. **Understand why** — what implicit contract did the commit violate?
6. **Fix at the right layer** — informed by the commit, not by hypothesis.

Build-break / crash exception: smallest fix-to-bisect first, then bisect normally. "The fix is obvious" is the speculative-fix failure mode in disguise.

The build chain is mirror-semantic (TASK-146): shader compile drops orphan `.dxil`, `inno_deploy_runtime_payload` wipes-and-recopies. For paranoid bisects across binding refactors, use `Scripts/HLSL2DXIL_NoPause.ps1 -FullClean`. Clangd index purge is automated via `Scripts/PurgeStaleClangdIndex.ps1` and the `post-checkout` / `post-merge` hooks.

## Cross-references

- `perf-measurement-frame-budget.md` — paired discipline; that one governs per-bisect-step cost (pick N for the perf bucket), this one governs the bisect sequence.
- `test-etiquette.md` — bisect steps require user-verified launches; "make each step cheap" applies to launch budget too.
- `tech-choice-vs-default.md` — the *understand why* step often surfaces a wrong-tech-pick that should have been caught before ship.
