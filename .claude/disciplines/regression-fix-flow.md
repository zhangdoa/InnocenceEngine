# Discipline: regression-fix-flow

When the user reports a regression — *"X used to work, now it's broken"* — the first action is to identify the breaking commit. Not to dispatch a fix. A speculative fix on top of an unreproduced regression contaminates the bisect surface and makes the real root cause harder to find. Long-form rationale + recorded incidents in `regression-fix-flow-extras.md`.

## Mandatory sequence

1. **Confirm the regression** — verify the symptom exists at the current state. One run, user-confirmed.
2. **Establish a known-good baseline** — find a previous commit where the symptom is absent. Build it, run it, get user confirmation. **Non-negotiable; without this anchor there is no bisect.** If the user can name a commit / version / "yesterday's build" that worked, start from there. If not, ask "when did you last see it work?" and narrow.
3. **Bisect** — `git bisect start <bad> <good>`, build + run + user-verifies at each step. Step size = one commit at the end; the dispatcher's job is to make each step cheap.
4. **Identify the breaking commit** — read its diff. The bug is usually small once the commit is known.
5. **Understand why** — what implicit contract did the commit violate?
6. **Fix at the right layer** — targeted, not speculative. The fix is informed by the commit, not by hypothesis.

### Exception: build-break or crash regressions

If the regression itself prevents bisect (engine won't start, build won't compile), fix-to-bisect first: smallest change that restores enough function to run the symptom check. Then bisect from there. Do not skip bisect because "the fix is obvious" — that is the speculative-fix failure mode wearing a different hat.

## The user is the only verifier

This is a single-developer project. The user is the only one who can run windowed visual tests and confirm whether a symptom is present at a given commit. Bisect therefore requires their participation at every step.

The dispatcher's obligation: make each step **cheap** for the user. One binary, one launch, one yes/no question ("does the symptom exist at this commit?"). Pre-build the candidate commit before asking. Surface the question with the commit hash and what changed. Do not ask the user to bisect themselves; the dispatcher drives, the user observes.

## Dispatch interaction

When the dispatch trigger is a user-reported regression, the dispatcher's first dispatch is **not** a fix. It is one of:

- A bisect plan (read the recent commit log, propose the candidate good commit, get user confirmation).
- A build of the candidate good commit, followed by the user-verification request.
- A read of the breaking commit's diff, once identified, before any fix dispatch.

Any agent receiving a fix dispatch on a user-reported regression should check: *did the dispatcher confirm a known-good baseline and identify the breaking commit?* If not, the dispatch is premature; surface that back to the dispatcher rather than starting a speculative fix.

## Build-cache contamination triage

**As of TASK-146 (landed 2026-04-26), the build chain is mirror-semantic.** `Scripts/Lib/Compile-HLSL.psm1` removes orphan `.dxil` artifacts at the start of every shader-compile invocation; the CMake `inno_deploy_runtime_payload` POST_BUILD step wipes-and-recopies the per-Config deploy target. Source-tree shader/asset removal propagates automatically. The mandatory bisect-step nuke dance (formerly required as a workaround) is no longer necessary.

For paranoid bisects (e.g. crossing a major shader-binding refactor), the shader-compile script accepts `-FullClean`:

```
powershell -ExecutionPolicy Bypass -File Scripts/HLSL2DXIL_NoPause.ps1 -FullClean
cmake --build Build --config RelWithDebInfo --target Main
```

For the C++ analog (clangd ghost diagnostics from stale `.idx` entries), see `regression-fix-flow-extras.md` § clangd index contamination — full triage path.

## Long-form

`regression-fix-flow-extras.md` — full rationale, clangd ghost-diagnostic triage, recorded incident (TASK-122 → TASK-141 → TASK-142 → TASK-145 → TASK-146), full anti-pattern catalogue.

## Cross-references

- `perf-measurement-frame-budget.md` — governs per-bisect-step cost.
- `test-etiquette.md` — "make each step cheap" applies to launch budget too.
- `tech-choice-vs-default.md` — the *understand why* step often surfaces a wrong-tech-pick.
