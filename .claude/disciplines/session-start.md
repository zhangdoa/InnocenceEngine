# Discipline: session-start

The producer agent runs at the start of every new session, before any substantive work, to produce a briefing the user can confirm or redirect. No code edits, captures, or task closures begin until the user confirms direction.

## How

1. Read `.claude/state/*.md`. Those files are the project-state snapshots (direction, remote-sync, engine invariants) the dispatcher anchors into briefs. Stale-looking snapshots are surface-able to the user.
2. List tasks at `.backlog/tasks/` with `status: In Progress`. For each, read Implementation Notes — that is the hand-off from the previous session.
3. Read the last 3-5 commits on the active branch for recent activity not yet captured in tasks.
4. Run `node .claude/hooks/lib/audit-backlog-drift.js --quiet` and review the surfaced candidates. Each entry is an open task with TASK-N references in landed commits — treat as a drift signal, not a verdict. Classify per-task (code-closure / cross-reference / multi-CL / explicit-deferred) using the recipe in TASK-201 before recommending any retrofit-flip; the script never mutates the backlog. Companion to the closure-staleness commit-gate, which catches drift forward — this catches what slipped through before the gate landed.
5. Note any task whose Implementation Notes end with a "priority order for remaining work" block — those are explicit continuation instructions.
6. Surface to the user: current state, likely next priorities, blockers, any gap between what's in-progress and what they might have been expecting. Include drift candidates worth retrofit-flipping when present.

## Cross-references

- `backlog-workflow.md` — Implementation Notes are the cross-session medium this discipline reads; that discipline governs how they get written.
- `agent-dispatch.md` — main-session Claude dispatches the producer in foreground at session start (legitimate two-condition match: result blocks the next sentence, no parallel work to do).
