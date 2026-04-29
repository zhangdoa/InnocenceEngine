# Discipline: comment-discipline

Comments describe the **present state**, never history. The codebase is not a changelog. Default is no comment; only add one when WHY is non-obvious — a hidden constraint, a subtle invariant, a workaround for a specific bug, behaviour that would surprise a reader.

## How

Why-comments are fine when they document a **present invariant**:

```cpp
// setter must be lock-free; AllToggles holds the mutex
```

Not when they explain how the code got here. Do not explain WHAT the code does — well-named identifiers already do that. Do not reference the current task, fix, or callers — that belongs in the PR description and rots as the codebase evolves.

If removing the comment wouldn't confuse a future reader, don't write it.

## Anti-patterns

Banned phrases (non-exhaustive):

- "now via X / used to be Y"
- "replaces / subsumes / retires"
- "migrated from"
- "deleted alongside"
- "first consumer lands in next commit"
- "added in `<sha>`"
- "before this fix"
- "fixed in TASK-NN"

Each one anchors the comment to a moment in history; six months later that moment is invisible to the reader and the comment is misleading.

## Cross-references

- `coding-principles.md` — *be explicit in code* runs first; comments are the fallback when naming alone cannot carry the meaning.
- `commit-message-policy.md` — historical context (what changed, why, when) belongs in the commit message, not in source comments.
