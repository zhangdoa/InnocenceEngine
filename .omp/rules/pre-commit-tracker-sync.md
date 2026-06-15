---
name: pre-commit-tracker-sync
description: "Before any code-related git operation, audit diff and tracker for backlog/memory drift"
condition: "\\bgit\\s+(status|diff|log|add|commit|stash)\\b"
scope: "tool:bash"
---

Before any code-related `git commit` (or pre-cursor `git status` / `git diff` / `git log` / `git add` / `git stash`): tracker state MUST match codebase state at every commit boundary.

Pre-commit audit checklist:

1. **Tasks in scope** (umbrella + sub-tasks matching diff files): if a phase/milestone landed but status / AC-ticks / implementation-notes weren't updated, fix in the SAME commit OR flag the gap in the commit body.
2. **Cross-references**: a sub-task subsumed by work in a sibling phase (e.g. TASK-227.3 work done in TASK-227.2's P4) gets its status flipped and a pointer note. Don't wait for a separate 'paperwork commit' that never comes.
3. **New work surfaced during the diff** (root causes, blockers, follow-ups): file as separate tasks OR log as surface-don't-chase. Decide before committing — never silently drop.
4. **basic-memory resume note** (InnocenceEngine project): if a state transition occurred (milestone landed, new direction chosen, blocker discovered), write/update the note in the same session. Resume notes are how the next session finds its bearings.
5. **Dead-letter tasks**: a task closed by duplicate or subsumed by adjacent work should be flipped to Done with a pointer, not left In Progress to mislead the next session.