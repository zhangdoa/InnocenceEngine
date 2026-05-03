# InnocenceEngine — Claude Instructions

Project-scoped orchestration. Meta / cross-project rules in user-scope `CLAUDE.md`.

## Project framing

- Single user (zhangdoa). No team, no other contributors, no CI fleet, no fresh-checkout onboarding for anyone else.
- Closure notes / backlog rationales / commit messages: cost is borne by zhangdoa alone. Do not write "blocks repro for someone else" or "misleads other developers."

## Working principles

- An agent is a stage of actions. Disciplines are the rules guiding actions inside a stage. No human-role simulation.
- Push back on scope that trades structural health for narrow completion.
- Surface structural observations.
- Never end a turn with "awaiting next instruction."

## Stages and disciplines

Stages live under `.claude/agents/`. Each stage declares which disciplines it loads, when. Disciplines are loaded **per stage**, not preloaded.

| Discipline directory | Loaded when |
|---|---|
| `disciplines/always/` | At session start. |
| `disciplines/on-session-start/` | Session begins. |
| `disciplines/on-design/` | Choosing structure / naming / tech. |
| `disciplines/on-implement/` | Writing source diffs (C++, HLSL, TS, harness, build). |
| `disciplines/on-bug-fix/` | Reproducing / bisecting / fixing a regression. |
| `disciplines/on-commit/` | Before `git commit`. |
| `disciplines/on-dispatch/` | Calling the `Agent` tool. |
| `disciplines/dispatcher/` | Main-session-only sub-rules. Sub-stages skip. |

Roster: `.claude/team.md`. Project-state snapshots: `.claude/state/*.md` (direction, remote-sync, engine invariants) — update in the same CL that lands a directional change.

## Session start

First action every new session: invoke `task-mgmt`. No substantive work before briefing + user direction. Procedure: `.claude/disciplines/on-session-start/session-start.md`.

## Dispatch

Main-session = dispatcher. Cross-stage work routes through `task-mgmt`. Peer review on every non-trivial implementation dispatch — fresh dispatch, never main-session, never the implementer (`.claude/disciplines/on-commit/peer-review-required.md`).

## Harness enforcement

- `.claude/settings.json` wires two PreToolUse hook dispatchers: `session-gate.js`, `commit-gate.js`.
- Per-gate logic: `.claude/hooks/gates/<name>.js`. Shared helpers: `.claude/hooks/lib/common.js`.
- Both fail open on internal errors.
- Each gate's block message lists its escape sentinel.
- Commit-message drafts: `Build/commit-message.txt` (gitignored).
