# Discipline: regression-fix-flow

When the user reports a regression — *"X used to work, now it's broken"* — the first action is to identify the breaking commit. Not to dispatch a fix.

A fix dispatched on top of an unreproduced regression piles new variables onto an already-uncertain baseline, contaminates the bisect surface, and accumulates changes that make the actual root cause harder to find. Every speculative fix is technical debt against the eventual real fix.

## Mandatory sequence

1. **Confirm the regression** — verify the symptom exists at the current state. One run, user-confirmed.
2. **Establish a known-good baseline** — find a previous commit where the symptom is absent. Build it, run it, get user confirmation that the symptom is gone there. **Non-negotiable; without this anchor there is no bisect.** If the user can name a commit / version / "yesterday's build" that worked, start from there. If not, ask "when did you last see it work?" and narrow.
3. **Bisect** — `git bisect start <bad> <good>`, build + run + user-verifies at each step. Step size is one commit at the end; the dispatcher's job is to make each step cheap.
4. **Identify the breaking commit** — read its diff. The bug is usually small once the commit is known.
5. **Understand why** — what implicit contract did the commit violate?
6. **Fix at the right layer** — targeted, not speculative. The fix is informed by the commit, not by hypothesis.

## Anti-patterns

- **Diagnostic dispatch against the regressed state with no baseline anchor.** The agent confirms the symptom but cannot identify the breaking commit because there is no comparison point. Its conclusions are perforce hypothetical.
- **Shipping a "fix" before the breaking commit is identified.** Each speculative fix introduces new variables (matrices, constants, code paths) that mask the original bug and corrupt the bisect surface.
- **Parallel agents touching adjacent files during a regression.** Each parallel CL adds another commit to the bisect range and another set of changes to the working tree. Serialize the work or pause it.
- **Treating user reports as bug-fix tickets.** "The brightness is off, ship a brightness fix" is wrong. "The brightness is off, find when it changed" is right.

## Exception: build-break or crash regressions

If the regression itself prevents bisect (the engine won't start, the build won't compile), fix-to-bisect first: the smallest possible change that restores enough function to run the symptom check. Then bisect from there. Do not skip bisect because "the fix is obvious" — that is the speculative-fix failure mode wearing a different hat.

## The user's role

This is a single-developer project. The user is the only one who can run windowed visual tests and confirm whether a symptom is present at a given commit. Bisect therefore requires their participation at every step.

The dispatcher's obligation: make each step **cheap** for the user. One binary, one launch, one yes/no question ("does the symptom exist at this commit?"). Pre-build the candidate commit before asking. Surface the question with the commit hash and what changed. Do not ask the user to bisect themselves; the dispatcher drives, the user observes.

## How this discipline interacts with dispatch

When the dispatch trigger is a user-reported regression, the dispatcher's first dispatch is **not** a fix. It is one of:

- A bisect plan (read the recent commit log, propose the candidate good commit, get user confirmation).
- A build of the candidate good commit, followed by the user-verification request.
- A read of the breaking commit's diff, once identified, before any fix dispatch.

Any agent receiving a fix dispatch on a user-reported regression should check: *did the dispatcher confirm a known-good baseline and identify the breaking commit?* If not, the dispatch is premature; surface that back to the dispatcher rather than starting a speculative fix.

## Recorded incident

TASK-122 → TASK-141 → TASK-142 → TASK-145 (2026-04-26). User reported "GISponza pastel" after the ACES → AGX swap (TASK-122). The dispatcher shipped TASK-141 (AGX matrix + encoding), then TASK-142 (K retune), then TASK-145 (diagnostic — against the regressed state, no baseline). When the user reported shadows totally gone, the bisect surface had been contaminated by ~5 commits' worth of accumulated changes; the diagnostic could not isolate root cause because there was no comparison anchor. User: *"we have been violating regression fix flows."*
