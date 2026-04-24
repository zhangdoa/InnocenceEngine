# Discipline: session-start

Runs at the start of every new session, before any substantive work, to produce a briefing the user can confirm or redirect.

1. List tasks at `.backlog/tasks/` with `status: In Progress`. For each, read Implementation Notes — that is the hand-off from the previous session.
2. Read the last 3–5 commits on the active branch for recent activity not yet captured in tasks.
3. Note any task whose Implementation Notes end with a "priority order for remaining work" block — those are explicit continuation instructions.
4. Surface to the user: current state, likely next priorities, blockers, any gap between what's in-progress and what they might have been expecting.

No substantive work (code edits, captures, task closures) begins until the user confirms direction.
