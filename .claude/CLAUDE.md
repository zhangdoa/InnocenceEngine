Owned by the `ai-expert` agent (hooks, agent manifests, shared disciplines, dispatcher disciplines, project-state snapshots, commit-gate, team roster, references map). See `.claude/agents/ai-expert.md` and `.claude/team.md`.

## Subtree map

- `agents/` — agent manifests (one file per role).
- `disciplines/` — universal disciplines every agent reads before acting.
- `disciplines/dispatcher/` — main-session-only rules; sub-agents do not need to read these.
- `state/` — project-state snapshots (direction, remote-sync, engine invariants). Read by the producer at session start. Update in the same CL that lands a directional change.
- `hooks/` — PreToolUse / SessionStart hooks. Per-gate logic under `hooks/gates/`, shared helpers under `hooks/lib/`, tests under `hooks/tests/`.
- `team.md` — agent roster.
- `collaboration.md` — cross-agent collaboration protocol.
- `references.json` — paper / reference-implementation map for the citation gate.

The Claude default auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** for this project. Writes there are blocked by the `no-auto-memory` gate (`hooks/gates/no-auto-memory.js`); the routing table lives in `disciplines/persistence-venue.md` and is duplicated in the gate's block message.
