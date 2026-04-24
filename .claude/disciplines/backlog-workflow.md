# Discipline: backlog-workflow

Tasks live under `.backlog/tasks/*.md` as the single AI-authored cross-session medium for priorities, decomposition, state, and findings. No parallel docs under repo-root or other tracked directories.

## Status values

- **To Do** — filed, not yet picked up.
- **In Progress** — actively being worked on; exactly one agent at a time should own any in-progress task.
- **Done** — complete. Closing a task requires test-run evidence (closure-evidence gate) and, for tasks labelled `paper-port`, an alignment artifact under `.alignments/` (paper-port gate).

## Who does what

- **Producer** owns task creation, decomposition, priority ordering, and cross-agent coordination. Writes the Description section and sets initial labels.
- **Every other agent** reads the tasks in their scope, updates the Implementation Notes as they work, writes the Final Summary at close, and flips `status:`.

## Cross-session continuity

Conversation context doesn't survive session boundaries. Everything a future session needs to resume must live in a tracked file — specifically in the owning task's Implementation Notes.

At the end of any landing CL on a multi-session task:
1. Update the task's `## Implementation Notes`: what landed, what's deferred, what's next, priority order for remaining work.
2. Update sub-slice statuses.
3. Commit the task changes in the same CL or a dedicated `docs(backlog)` follow-up. Backlog changes must never be left uncommitted across sessions.

At the start of a session, the producer reads in-progress tasks and their Implementation Notes; that is the hand-off. Never infer "what's next" from commit subjects or prior-conversation memory.

## MCP reference (when available)

The Backlog.md MCP server, when connected, exposes detailed workflow guides at `backlog://workflow/overview` (or `backlog.get_backlog_instructions()`) covering the decision framework for task creation, search-first-to-avoid-duplicates, and finalisation checklists. Consult it for non-obvious operations — e.g. milestone organisation, archival criteria.

If the MCP is disconnected, the rules above still hold; task files are plain markdown and editable directly.
