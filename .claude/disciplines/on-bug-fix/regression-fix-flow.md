# Discipline: regression-fix-flow

When the user reports *"X used to work, now it's broken"* → identify the breaking commit first. A speculative fix on top of an unreproduced regression contaminates the bisect surface.

## Mandatory sequence

1. **Confirm** — verify the symptom exists at current HEAD; one user-confirmed run.
2. **Baseline** — find a known-good prior commit; build, run, user confirms. Non-negotiable.
3. **Bisect** — `git bisect start <bad> <good>`; dispatcher drives, user observes. Pre-build each candidate; one yes/no question per step.
4. **Identify the breaking commit** — read its diff.
5. **Understand why** — what implicit contract did the commit violate?
6. **Fix at the right layer** — informed by the commit, not by hypothesis.

Build-break / crash exception: smallest fix-to-bisect first, then bisect normally. "The fix is obvious" is the speculative-fix failure mode in disguise.

## Build chain notes

- Mirror-semantic: shader compile drops orphan `.dxil`; `inno_deploy_runtime_payload` wipes-and-recopies.
- Paranoid bisects across binding refactors → `Scripts/HLSL2DXIL_NoPause.ps1 -FullClean`.
- Clangd index purge automated via `Scripts/PurgeStaleClangdIndex.ps1` and the `post-checkout` / `post-merge` hooks.

## Cross-references

- `on-bug/perf-measurement-frame-budget.md` — paired: governs per-bisect-step cost.
- `on-test/test-etiquette.md` — bisect steps require user-verified launches.
- `on-design/tech-choice-vs-default.md` — *understand why* often surfaces a wrong-tech-pick.
