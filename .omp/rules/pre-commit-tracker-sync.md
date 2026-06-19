---
name: pre-commit-tracker-sync
description: "Before git add / commit, audit diff and tracker for backlog/memory drift"
condition: "\\bgit\\s+(add|commit)\\b"
scope: "tool:bash"
---

Before `git add` / `git commit`, tracker state must match codebase state:

1. **Tasks in scope** (umbrella + sub-tasks matching diff files): status / AC-ticks / notes not updated → fix in the same commit or flag the gap in the body.
2. **Cross-references**: a sub-task subsumed by a sibling phase gets its status flipped and a pointer. No separate 'paperwork commit'.
3. **New work surfaced in the diff**: file as separate tasks or log as surface-don't-chase. Decide before committing.
4. **basic-memory resume note** (InnocenceEngine): on a state transition (milestone, new direction, blocker), write/update it this session.
5. **Dead-letter tasks**: closed-by-duplicate / subsumed → Done with a pointer, not left In Progress.
