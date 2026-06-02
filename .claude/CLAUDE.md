## Subtree map

Harness spans two roots after the omp migration: `.claude/` (Claude-provider content omp
still discovers) and `.omp/` (omp-native: skills + the commit-guard extension).

- `.claude/agents/` — stage manifests (one file per stage). Impl agents carry `spawns: ""`
  (cannot self-dispatch review); `task-mgmt` carries `spawns: "*"` (dispatcher).
- `.claude/commands/` — reserved venue for project-local slash commands (omp discovers
  `.claude/commands/*.md`; allowlisted by commit-guard's no-new-md). No dir created yet.
- `.claude/state/` — point-in-time fact snapshots (direction, remote-sync, engine
  invariants). Read by `task-mgmt` at session start. Update in the same CL as a directional
  change.
- `.omp/skills/<name>/SKILL.md` — project skills, omp-native (priority 100, so they win over
  same-named user `~/.claude/skills`). Enforcement-supporting content only; deterministic
  rules live as commit-guard gates, not skills.
- `.omp/extensions/commit-guard/` — the commit gates (TypeScript omp extension; ports the
  removed Claude-era `.claude/hooks`). Intercepts `git commit` via the `tool_call` event.
  `bun test` under `tests/` must pass for changes.

omp does not run Claude Code's hook/settings wiring: `.claude/settings.json` and
`.claude/hooks/` are gone. Tool permissioning is omp's approval-mode. The old `no-auto-memory`
gate was dropped — `~/.claude/projects/<slug>/memory/` is not a venue omp writes to.
