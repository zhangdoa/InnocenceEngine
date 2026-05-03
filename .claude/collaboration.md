# Collaboration protocol

Stages coordinate by scope boundaries declared in owned subtrees' `CLAUDE.md` files.

## Out-of-stage work

- Name the relevant stage in your turn summary.
- Describe the hand-off scope concretely — files, concern.
- Do not fix it yourself.

Cross-stage work that is genuinely unavoidable (e.g., a `code-impl` change needs a paired `shader-impl` change) → route through `task-mgmt`.

## Dispatch

Main-session = dispatcher. Read staged scope → match it to a stage from `.claude/team.md` → invoke via `Agent` tool. Universal commit-gate rules fire on every CL.

`Agent` invocations default to `run_in_background: true`. Foreground dispatch must satisfy the two-condition test in `.claude/disciplines/on-dispatch/agent-dispatch.md`. Any agent that delegates to a sub-agent is itself a dispatcher.

## Escalation: agent teams

Default = subagents dispatched via `task-mgmt` (or any owning dispatcher).

Escalate to experimental agent-teams (`CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS=1`) **only** for adversarial review or multi-hypothesis debugging — when teammates need to challenge each other directly via `SendMessage` rather than serializing through a dispatcher.

Not the default — experimental, no project-level config (per-machine state under `~/.claude/teams/`), session-ephemeral (mailboxes vanish), breaks `/resume`, unsupported in Windows Terminal split-pane. Flip on for a specific debate session, then off.
