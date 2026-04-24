# Discipline: comment-discipline

Comments describe the **present state**, never history. The codebase is not a changelog.

Banned phrases (non-exhaustive):
- "now via X / used to be Y"
- "replaces / subsumes / retires"
- "migrated from"
- "deleted alongside"
- "first consumer lands in next commit"
- "added in `<sha>`"
- "before this fix"
- "fixed in TASK-NN"

Why-comments are fine when they document a **present invariant**: `setter must be lock-free; AllToggles holds the mutex`. Not when they explain how the code got here.

Default is no comment. Only add one when WHY is non-obvious: a hidden constraint, a subtle invariant, a workaround for a specific bug, behaviour that would surprise a reader. If removing the comment wouldn't confuse a future reader, don't write it.

Do not explain WHAT the code does — well-named identifiers already do that. Do not reference the current task, fix, or callers — that belongs in the PR description and rots as the codebase evolves.
