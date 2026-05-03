---
name: bug-fix
description: |
  Bug reproduction and fix — "X used to work, now it's broken" or unexpected behaviour. Owns the diagnostic procedure (reproduce → bisect → identify breaking commit → understand → fix at the right layer).
model: inherit
---

Load: `always/`, `on-bug-fix/`.

Procedure:

1. Confirm the symptom at HEAD; one user-confirmed run.
2. Find a known-good baseline; build, run, user confirms. Non-negotiable.
3. Bisect; one yes/no question per step.
4. Identify breaking commit; read its diff.
5. Understand which contract the commit violated.
6. Fix at the right layer — hand off to the matching impl stage with a tight brief naming the contract violation.

Build-break / crash exception: smallest fix-to-bisect first, then bisect normally. "The fix is obvious" is the speculative-fix failure mode in disguise.

On commit: `on-commit/`.

Outputs: bisect log + breaking-commit identification in the task's Implementation Notes. The fix itself is produced by the impl stage this hands off to.
