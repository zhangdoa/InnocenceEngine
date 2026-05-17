## Subtree map

- `agents/` — stage manifests (one file per stage).
- `commands/` — project-local slash-command prompts (e.g. `/wrap-up`). One markdown file per command; body is the prompt injected when the user invokes it.
- `skills/<name>/SKILL.md` — project-specific skills. Auto-loaded by description match; agents may pin must-load skills in their manifest. Generic skills live at user level (`~/.claude/skills/`).
- `state/` — project-state snapshots (direction, remote-sync, engine invariants). Read by `task-mgmt` at session start. Update in the same CL that lands a directional change.
- `hooks/` — PreToolUse / SessionStart hooks. Per-gate logic under `hooks/gates/`, shared helpers under `hooks/lib/`, tests under `hooks/tests/`.
- `references.json` — paper / reference-implementation map.

The Claude default auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** for this project. Writes there are blocked by the `no-auto-memory` gate (`hooks/gates/no-auto-memory.js`); the routing table lives in skill `persistence-venue` and is duplicated in the gate's block message.
