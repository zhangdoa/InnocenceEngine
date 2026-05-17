---
name: session-start
description: Use at the start of every new session before substantive work. task-mgmt reads state snapshots, in-progress tasks, recent commits, runs the drift audit, surfaces priorities to the user.
---

# Skill: session-start

The task-mgmt agent runs at the start of every new session, before any substantive work. No code edits, captures, or task closures begin until the user confirms direction.

## Procedure

1. Read `.claude/state/*.md` (project-state snapshots: direction, remote-sync, engine invariants). Surface stale-looking snapshots to the user.
2. List `.backlog/tasks/` with `status: In Progress`. For each, read Implementation Notes — that is the previous session's hand-off.
3. Read the last 3-5 commits on the active branch.
4. Run `node .claude/hooks/lib/audit-backlog-drift.js --quiet`. Each surfaced entry is an open task with TASK-N references in landed commits — drift signal, not a verdict. Classify per-task (code-closure / cross-reference / multi-CL / explicit-deferred) before recommending any retrofit-flip. The script never mutates the backlog.
5. Note any task whose Implementation Notes end with a "priority order for remaining work" block — those are explicit continuation instructions.
6. Surface to the user: current state, likely next priorities, blockers, gap between in-progress and what they might expect. Include drift candidates worth retrofit-flipping when present.

## Cross-references

- `backlog-workflow` — Implementation Notes are the cross-session medium this discipline reads.
- `agent-dispatch` (user level) — main-session dispatches task-mgmt in foreground at session start (legitimate two-condition match).
