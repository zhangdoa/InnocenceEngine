---
name: persistence-venue
description: Use when deciding where to record a rule, fact, snapshot, or correction in this project. Project-specific routing table mapping content shapes to repo venues; auto-memory venue is disabled in this project.
---

# Skill: persistence-venue (project extension)

Generic decision rule (who needs to act, what shape, cost of slip, ephemeral or not) lives in user-level `persistence-venue`. This skill carries the project-specific routing table.

## Auto-memory disabled

The Claude default auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** for this project — entries there are subagent-invisible and bypass review pressure. The `no-auto-memory` gate inside `.claude/hooks/session-gate.js` blocks writes there and emits this routing table.

## Routing table

| Content shape | Venue | Reach | Reviewed |
|---|---|---|---|
| Rule the dispatcher (main-session) must follow | `.claude/skills/dispatch-briefs/SKILL.md` | main-session only | yes (commit) |
| Rule every agent must follow | `.claude/skills/<topic>/SKILL.md` (and add to root `CLAUDE.md` universal preamble) | every agent | yes (commit) |
| Rule one specific role must follow | section in `.claude/agents/<role>.md` | only that role | yes (commit) |
| Generic rule applicable across all your projects | `~/.claude/skills/<topic>/SKILL.md` | every agent on every project | not project-reviewed |
| Project-state snapshot (direction, sync, engine invariants) | `.claude/state/<topic>.md` | main-session + task-mgmt at session start | yes (commit) |
| Cross-session continuity for an in-flight task | `## Implementation Notes` in `.backlog/tasks/<task>.md` | every agent that reads the task | yes (commit) |
| Cost-of-one-slip-is-high enforcement | new gate under `.claude/hooks/gates/<name>.js` with explicit escape sentinel | every tool call | yes (commit) |
| Ephemeral conversation context | do not persist; let it scroll | none | n/a |

## Cross-references

- User-level `persistence-venue` — generic decision rule, register translation, auto-memory considerations.
- `workspace-hygiene` — backlog is the only AI-authored project-state medium for tasks. `.claude/state/` files are point-in-time snapshots, not tasks.
- `backlog-workflow` — Implementation Notes are the cross-session medium subagents see.
