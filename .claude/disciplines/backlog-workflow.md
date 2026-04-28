# Discipline: backlog-workflow

Tasks live under `.backlog/tasks/*.md` as the single AI-authored cross-session medium for priorities, decomposition, state, and findings. No parallel docs under repo-root or other tracked directories.

## No rogue implementations

**Every implementation requires a backlog task — no exceptions.** That includes user-driven asks, dispatcher-driven asks, and emergent fixes that surface mid-dispatch. The sequence is always:

1. File task (single AC is fine; the file step is non-skippable).
2. Commit the task file.
3. Dispatch implementation against the task.

The file-and-dispatch can happen in a single turn, but the file step is what makes the change auditable: it carries the AC list, the rationale, the labels, and the eventual closure evidence. A commit message is not a substitute — it post-records the *what*, not the pre-recorded *boundary of work*.

### Categories that are explicitly NOT exempt

- **"User asked directly."** User asks → file task → commit task → dispatch. The user setting direction is not the same as a task existing. The triggering precedent is the RasterizedGI dev-toggle CL (`a8cbda10`, 2026-04-28): user asked for the toggle, dispatcher went straight to implementation, no backlog entry exists for it. The commit message reads coherently but there is no AC list, no closure record, and no reviewable boundary. User flagged the gap explicitly: *"make sure our discipline well covers that no rogue fixes or features, even from me must be tasked to the back log."*
- **"Bug just surfaced and is small."** File task → dispatch. Small bugs are still bugs and benefit from an AC list, closure evidence, and a discoverable record.
- **"It's a one-line fix."** File task → dispatch. Line count is orthogonal to whether traceability matters.
- **"It's part of the task already in flight."** If the new work is genuinely within scope of an existing in-progress task, append the AC to that task; if it is not, file a new one. Silently expanding scope is a backlog-discipline violation either way.

### Mechanical exemptions that DO exist

These are exemptions on *peer review*, not on the backlog-task requirement. The task still gets filed:

- Backlog file edits themselves — filing a task is not implementation, so does not recursively require its own task.
- Pure mechanical refactors (file moves, renames) where the file IS the diff — see `peer-review-required.md` § mechanical exemption. The task still exists, but review may be skipped.
- Hook self-edits / harness-internal disciplines — see `peer-review-required.md` § hook-internal exemption. Same pattern: task exists, review may be skipped.

### Cross-reference

The no-rogue-implementations rule is one layer up from peer review: tasks must exist *before* dispatch. Peer review is the layer where dispatched work is reviewed *before* commit. The two disciplines compose — task-then-dispatch-then-review-then-commit is the full sequence.

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

### Harness enforcement — closure-staleness gate

Compliance with the rule above is structurally enforced by `.claude/hooks/gates/closure-staleness.js` (wired through `.claude/hooks/commit-gate.js`). The gate:

- Parses `TASK-\d+` references from the commit message.
- Resolves each referenced task file (staged-first so a same-CL flip is honored, on-disk fallback) and reads the effective `status:` frontmatter.
- Blocks the commit when any referenced task is `In Progress` or `To Do` AND at least one staged file is non-docs-only (a pure `docs(backlog)` flip CL passes through).
- Allows the commit when `[task-stays-open]` appears in the message — the audit-trail escape hatch for genuinely partial work.

Symmetric to the `closure-evidence` rule inside `test-run.js` (closure-evidence catches "claimed Done with no test"; closure-staleness catches the upstream half — "wrote code citing TASK-N without closing it"). Fail-open on internal errors, same as every other gate.

## MCP reference (when available)

The Backlog.md MCP server, when connected, exposes detailed workflow guides at `backlog://workflow/overview` (or `backlog.get_backlog_instructions()`) covering the decision framework for task creation, search-first-to-avoid-duplicates, and finalisation checklists. Consult it for non-obvious operations — e.g. milestone organisation, archival criteria.

If the MCP is disconnected, the rules above still hold; task files are plain markdown and editable directly.
