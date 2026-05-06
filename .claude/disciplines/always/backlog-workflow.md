# Discipline: backlog-workflow

Tasks live under `.backlog/tasks/*.md` as the single AI-authored cross-session medium. **Every implementation requires a backlog task — no exceptions.**

## Sequence

1. File task (single AC fine; non-skippable).
2. Commit the task file.
3. Dispatch implementation against the task.

File-and-dispatch can happen in one turn.

## Status values

- **To Do** — filed, not picked up.
- **In Progress** — actively worked. Exactly one agent at a time.
- **Done** — closed with test-run evidence (closure-evidence gate). `paper-port`-labelled → also alignment artifact under `.alignments/` (paper-port gate).

## Ownership

- **Producer** owns task creation, decomposition, priority, cross-agent coordination. Writes Description, sets initial labels.
- **Other agents** read tasks in their scope, update Implementation Notes during work, write Final Summary at close, flip `status:`.

## Cross-session continuity

End of any landing CL on a multi-session task:

1. Update `## Implementation Notes`: what landed, what's deferred, what's next, priority order.
2. Update sub-slice statuses.
3. Commit task changes in the same CL or a `docs(backlog)` follow-up. Never leave backlog uncommitted across sessions.

Session start: producer reads in-progress tasks + Implementation Notes — that's the hand-off. Never infer "what's next" from commit subjects or prior-conversation memory.

## Closure-staleness gate

`gates/closure-staleness.js` parses `TASK-\d+` from commit messages. Blocks when any referenced task is `In Progress` / `To Do` AND staged files include non-docs paths. Bypass: `[task-stays-open]`.

## Closure-evidence is main-session-only

`test-run.js` parses the *main-session* bash transcript. Sub-agent transcripts are isolated. Before closing integration-test-relevant work (engine, editor, rendering, shaders), main-session pre-runs:

- Engine: `cmake --build Build --config RelWithDebInfo --target Main` then `Bin/RelWithDebInfo/Main.exe -total_frames N`.
- Editor: `cd Source/Editor-Next && npm test -- --workers=1 tests/<spec>.spec.js`.
- Rendering pipeline: `Bin/RelWithDebInfo/RenderTest.exe -test <name>`.

Docs-only path bypass (`DOCS_ONLY_PATH`) handles backlog/harness commits automatically. Bypass not firing for a legitimately exempt path → extend the regex.

Exemption for closure-only docs CLs: a `Closure-Reason: <value>` commit-message footer re-applies the docs-only bypass when a task is flipping to Done. Use only for genuinely-obsolete / non-reproducible / superseded closures where running an integration test would add no signal — not for "the test was a pain to set up". Code-bearing CLs still require a qualifying test run; the exemption only suspends the closure-as-evidence override on the docs-only path.

## Don't pile on backlog tasks

Before filing, ask: blocking the user's terminal goal? Would I do this work today if dispatched? Both no → don't file. Note inline (closure note, commit-message footer, or just move on).

Recurring noise: only file once it actually cost the user — a build failure, a real bisect, a wrong diagnosis.

Sequential beats parallel for the next session.

## Anti-patterns — explicitly NOT exempt

- "User asked directly" → file task → dispatch.
- "Bug is small / one-line fix" → file task → dispatch.
- "Part of the task already in flight" → genuinely in scope: append AC; not in scope: file new.

## Mechanical exemptions (peer-review only, not file-task)

- Backlog file edits.
- Pure mechanical refactors where the file IS the diff.
- Hook self-edits / harness-internal disciplines.

## Cross-references

- `on-commit/peer-review-required.md`, `always/persistence-venue.md`, `on-commit/commit-message-policy.md`, `on-session-start/session-start.md`, `always/surface-dont-chase.md`.
