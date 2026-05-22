## Subtree map

- `agents/` — stage manifests (one file per stage).
- `commands/` — project-local slash-command prompts. One markdown file per command; body is the prompt injected when invoked.
- `skills/<name>/SKILL.md` — gate-error-message-supporting content only. Skills are deferred-loaded markdown; rules that don't fire 100% live as gates, not skills.
- `state/` — point-in-time fact snapshots (direction, remote-sync, engine invariants). Read by `task-mgmt` at session start. Update in the same CL that lands a directional change.
- `hooks/` — `session-gate.js`, `commit-gate.js`, per-gate logic under `hooks/gates/`, shared helpers under `hooks/lib/`, tests under `hooks/tests/`.

Claude default auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** for this project. The `no-auto-memory` gate blocks writes there.
