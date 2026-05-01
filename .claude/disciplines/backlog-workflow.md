# Discipline: backlog-workflow

Tasks live under `.backlog/tasks/*.md` as the single AI-authored cross-session medium for priorities, decomposition, state, and findings. **Every implementation requires a backlog task — no exceptions.** No parallel docs under repo-root or other tracked directories.

A commit message is not a substitute for a task: it post-records the *what*, not the pre-recorded *boundary of work*. Without a task there is no AC list, no closure record, no reviewable boundary.

## How

The sequence is always:

1. File task (single AC is fine; the file step is non-skippable).
2. Commit the task file.
3. Dispatch implementation against the task.

The file-and-dispatch can happen in a single turn, but the file step is what makes the change auditable.

### Status values

- **To Do** — filed, not yet picked up.
- **In Progress** — actively being worked on; exactly one agent at a time should own any in-progress task.
- **Done** — complete. Closing a task requires test-run evidence (closure-evidence gate) and, for tasks labelled `paper-port`, an alignment artifact under `.alignments/` (paper-port gate).

### Who does what

- **Producer** owns task creation, decomposition, priority ordering, and cross-agent coordination. Writes the Description section and sets initial labels.
- **Every other agent** reads the tasks in their scope, updates the Implementation Notes as they work, writes the Final Summary at close, and flips `status:`.

### Cross-session continuity

Conversation context doesn't survive session boundaries. Everything a future session needs to resume must live in a tracked file — specifically in the owning task's Implementation Notes.

At the end of any landing CL on a multi-session task:

1. Update the task's `## Implementation Notes`: what landed, what's deferred, what's next, priority order for remaining work.
2. Update sub-slice statuses.
3. Commit the task changes in the same CL or a dedicated `docs(backlog)` follow-up. Backlog changes must never be left uncommitted across sessions.

At the start of a session, the producer reads in-progress tasks and their Implementation Notes; that is the hand-off. Never infer "what's next" from commit subjects or prior-conversation memory.

### Harness enforcement — closure-staleness gate

Compliance is structurally enforced by `.claude/hooks/gates/closure-staleness.js` (wired through `.claude/hooks/commit-gate.js`). The gate:

- Parses `TASK-\d+` references from the commit message.
- Resolves each referenced task file (staged-first so a same-CL flip is honored, on-disk fallback) and reads the effective `status:` frontmatter.
- Blocks the commit when any referenced task is `In Progress` or `To Do` AND at least one staged file is non-docs-only (a pure `docs(backlog)` flip CL passes through).
- Allows the commit when `[task-stays-open]` appears in the message — the audit-trail escape hatch for genuinely partial work.

Symmetric to the `closure-evidence` rule inside `test-run.js`: closure-evidence catches "claimed Done with no test"; closure-staleness catches the upstream half — "wrote code citing TASK-N without closing it."

#### Closure-evidence is main-session-only

The closure-evidence half of `test-run.js` parses the *main-session* bash transcript for qualifying integration-test invocations. Sub-agent transcripts are isolated; a test that ran inside an implementer or peer-reviewer dispatch is invisible to the gate. Before attempting a closing commit on any task whose scope is integration-test-relevant (engine, editor, rendering, shaders), the dispatcher pre-runs the relevant test command in main-session bash:

- `cmake --build Build --config RelWithDebInfo --target Main` then `Bin/RelWithDebInfo/Main.exe -total_frames N` — engine work.
- `cd Source/Editor-Next && npx playwright test tests/<spec>.spec.js` — editor work.
- `Bin/RelWithDebInfo/RenderTest.exe -test <name>` — rendering pipeline work.

If the work was already validated by the sub-agent and re-running is genuinely redundant (hook-internal change validated by unit tests; backlog-only docs commit), use `[skip-test-gate]` with explicit rationale. Under parallel dispatch a pre-run validates the *combined* worktree state, which is useful (catches drift across agents) but means the test exercises something the implementer did not see directly.

The intent: zero-trust evidence. Sub-agent claims of "tests passed" are not auditable in `git log`; main-session transcript invocations are.

### Don't pile on backlog tasks

The backlog is the cross-session AC and closure ledger, not a dump for every observed rough edge. Every entry has a cost — read, prioritise, dispatch, sometimes re-read across sessions. A 200-task backlog where 50 are hygiene noise is worse than a 150-task backlog of real work.

When tempted to file a task for an observed issue, ask:

- Is this blocking the user's stated terminal goal?
- Would I do this work today if dispatched?

If "no" to both, do not file. Note it inline (closure note, commit message, or just keep working) and move on. For the same noise hitting multiple sessions: only escalate to a task once it actually *cost* the user something — a build failure, a real bisect, a wrong diagnosis. Surface-level annoyance is not escalation-grade.

The bar for filing: would the user, looking at this task name in a backlog list six weeks from now, want it there or want it culled? If the answer is "cull," do not file.

What to do instead with reflexive task ideas:

1. Commit-message footer: "Noted during this work: clangd shows N unused-includes; not addressed."
2. Closure note in the task being closed: "Adjacent rough edge: X. Skipping unless it bites."
3. Verbal mention to the user; let them decide whether it is task-worthy.

Sequential beats parallel for the next session: pick one task, finish it, then pick the next. Three-agents-in-parallel on independent tasks creates orthogonality risk (cross-subtree stash collisions, divergent worktree state) and exceeds the user's preferred working cadence.

This is the complement of `surface-dont-chase.md`: that one says "do not fold discoveries into the current CL"; this one says "do not file every discovery as a separate task either." Worthy discoveries get filed; reflexive ones get noted inline.

### MCP reference (when available)

The Backlog.md MCP server, when connected, exposes detailed workflow guides at `backlog://workflow/overview` (or `backlog.get_backlog_instructions()`) covering the decision framework for task creation, search-first-to-avoid-duplicates, and finalisation checklists. If the MCP is disconnected, the rules above still hold; task files are plain markdown and editable directly.

## Anti-patterns

The following are **explicitly NOT exempt** from the file-task-first rule:

- **"User asked directly."** User asks → file task → commit task → dispatch. The user setting direction is not the same as a task existing.
- **"Bug just surfaced and is small."** File task → dispatch. Small bugs are still bugs and benefit from an AC list, closure evidence, and a discoverable record.
- **"It's a one-line fix."** File task → dispatch. Line count is orthogonal to whether traceability matters.
- **"It's part of the task already in flight."** If the new work is genuinely within scope of an existing in-progress task, append the AC to that task; if it is not, file a new one. Silently expanding scope is a backlog-discipline violation either way.

### Mechanical exemptions that DO exist

These exempt **peer review**, not the backlog-task requirement. The task still gets filed:

- Backlog file edits themselves — filing a task is not implementation, so does not recursively require its own task.
- Pure mechanical refactors (file moves, renames) where the file IS the diff — see `peer-review-required.md` § mechanical exemption.
- Hook self-edits / harness-internal disciplines — see `peer-review-required.md` § hook-internal exemption.

## Cross-references

- `peer-review-required.md` — paired layer-up: tasks must exist before dispatch (this discipline); review must happen before commit (that one). Together: task → dispatch → review → commit.
- `persistence-venue.md` — explains why backlog Implementation Notes are the only AI-authored cross-session medium that reaches subagents.
- `commit-message-policy.md` — `TASK-NN` references in commit messages are what the closure-staleness gate parses.
- `session-start.md` — the producer reads In-Progress tasks at session start; that is the hand-off this discipline produces.

After every non-trivial CL, file structural findings (what implicit contract was violated, what weakness allowed it, what improvement moves toward the four target qualities) as their own backlog tasks. Conversation memory does not survive turn / session boundaries; tasks do.
