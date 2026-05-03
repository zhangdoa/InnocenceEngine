# Discipline: persistence-venue

Routing table for cross-session corrections, facts, and conventions. The auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** in this project — entries there are subagent-invisible and bypass review pressure. The `no-auto-memory` gate inside `.claude/hooks/session-gate.js` blocks writes there and emits this routing table.

## Routing table

| Content shape | Venue | Reach | Reviewed |
|---|---|---|---|
| Rule the dispatcher (main-session) must follow | `.claude/disciplines/dispatcher/<topic>.md` | main-session only | yes (commit) |
| Rule every agent must follow | `.claude/disciplines/<topic>.md` (and add to root `CLAUDE.md` universal preamble) | every agent | yes (commit) |
| Rule one specific role must follow | section in `.claude/agents/<role>.md` | only that role | yes (commit) |
| Project-state snapshot (direction, sync, engine invariants) | `.claude/state/<topic>.md` | main-session + producer at session start | yes (commit) |
| Cross-session continuity for an in-flight task | `## Implementation Notes` in `.backlog/tasks/<task>.md` | every agent that reads the task | yes (commit) |
| Cost-of-one-slip-is-high enforcement | new gate under `.claude/hooks/gates/<name>.js` with explicit escape sentinel | every tool call | yes (commit) |
| Ephemeral conversation context | do not persist; let it scroll | none | n/a |

## Decision rule

1. **Who needs to act on this?** Sub-agents do not see anything outside the repo, so anything multi-agent must live in the repo.
2. **What shape is the content?** A rule (steps, applicability, anti-patterns) → discipline. A snapshot (point-in-time fact) → `.claude/state/`. Cross-session task continuity → task's Implementation Notes.
3. **Cost of one slip is high?** Promote to a gate with a clear escape sentinel.
4. **Conversation context not actionable next session?** Let it scroll.

## Register translation

When promoting from user prose into a repo venue: paraphrase into discipline-file register — operative rule first, structured sections, no first-person, no session-specific narrative. Match the existing universal disciplines' tone. Translate the substance; drop the framing.

## Cross-references

- `always/workspace-hygiene.md` — backlog is the only AI-authored project-state medium for tasks. `.claude/state/` files are point-in-time snapshots, not tasks.
- `always/backlog-workflow.md` — Implementation Notes are the cross-session medium subagents see.
