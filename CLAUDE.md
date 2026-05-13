# InnocenceEngine — Claude Instructions

Project-scoped orchestration. Meta / cross-project rules in user-scope `CLAUDE.md`.

## Project framing

- Single user (zhangdoa). No team, no other contributors, no CI fleet, no fresh-checkout onboarding for anyone else.
- Closure notes / backlog rationales / commit messages: cost is borne by zhangdoa alone. Do not write "blocks repro for someone else" or "misleads other developers."

## Working principles

- An agent is a stage of actions. Skills are the rules and procedures the agent applies inside a stage. No human-role simulation.
- Push back on scope that trades structural health for narrow completion.
- Surface structural observations.
- Never end a turn with "awaiting next instruction."

## Stages and skills

Stages live under `.claude/agents/`. Skills live under `.claude/skills/<name>/SKILL.md` and auto-load by description match; agents may also pin must-load skills in their manifest.

Generic, project-agnostic skills live at user level (`~/.claude/skills/`). Project-specific skills extend or specialise:

| Project skill | Use when |
|---|---|
| `backlog-workflow` | Filing, working, or closing a backlog task. |
| `persistence-venue` | Deciding where to record a rule, fact, or correction. |
| `workspace-hygiene` | Creating files, scratch output, or new docs. |
| `session-start` | Start of every new session. |
| `dispatch-briefs` | Main-session shaping a dispatch brief. |
| `commit-message-policy`, `peer-review-required` | Before `git commit`. |
| `file-splitting` | File-size gate hits, or before adding code that would push a file past the limit. |
| `cpp-style`, `safety-observability`, `threading-contracts` | Engine C++. |
| `shader-standards` | Engine HLSL. |
| `paper-port`, `paper-audit` | Paper-driven implementation. |
| `test-etiquette`, `visual-validation` | Engine / editor / Playwright runs; rendering output. |
| `perf-frame-budget`, `regression-build-chain` | Engine perf measurement; engine regression bisects. |
| `comment-discipline`, `fundamentals` | Editing harness files; engine-specific quality bar. |

Roster: `.claude/team.md`. Project-state snapshots: `.claude/state/*.md` (direction, remote-sync, engine invariants) — update in the same CL that lands a directional change.

## Session start

First action every new session: invoke `task-mgmt`. No substantive work before briefing + user direction. Procedure: skill `session-start`.

## Dispatch

Main-session = dispatcher. Cross-stage work routes through `task-mgmt`. Peer review on every non-trivial implementation dispatch — fresh dispatch, never main-session, never the implementer (skill `peer-review-required`).

## Harness enforcement

- `.claude/settings.json` wires two PreToolUse hook dispatchers: `session-gate.js`, `commit-gate.js`.
- Per-gate logic: `.claude/hooks/gates/<name>.js`. Shared helpers: `.claude/hooks/lib/common.js`.
- Both fail open on internal errors.
- Each gate's block message lists its escape sentinel.
- Commit-message drafts: `Build/commit-message.txt` (gitignored).
