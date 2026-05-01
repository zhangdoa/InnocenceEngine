# Discipline: persistence-venue

Prescriptive routing table for cross-session corrections, facts, and conventions. The auto-memory venue (`~/.claude/projects/<slug>/memory/`) is **disabled** in this project — entries there are subagent-invisible and bypass review pressure. The `no-auto-memory` gate inside `.claude/hooks/session-gate.js` blocks writes to that directory and emits the same routing table the next Claude reads.

## Routing table

| Content shape | Venue | Reach | Reviewed |
|---|---|---|---|
| Rule the dispatcher (main-session) must follow | `.claude/disciplines/dispatcher/<topic>.md` | main-session only | yes (commit) |
| Rule every agent must follow | `.claude/disciplines/<topic>.md` (and add to root `CLAUDE.md` universal preamble) | every agent | yes (commit) |
| Rule one specific role must follow | add a section to that agent's manifest `.claude/agents/<role>.md` | only that role | yes (commit) |
| Project-state snapshot (direction, sync, engine invariants) | `.claude/state/<topic>.md` | main-session + producer at session start | yes (commit) |
| Cross-session continuity for an in-flight task | the task's `## Implementation Notes` in `.backlog/tasks/<task>.md` | every agent that reads the task | yes (commit) |
| Cost-of-one-slip-is-high enforcement | a new gate under `.claude/hooks/gates/<name>.js` with an explicit escape sentinel | every tool call | yes (commit) |
| Ephemeral conversation context | do not persist; let it scroll | none | n/a |

The venues above all live in tracked files, which means every change passes through the commit-gate (peer review, attribution, gates that apply). That review pressure is the whole point: stale advice gets challenged when it touches a CL, instead of accumulating unread.

## Decision rule

1. **Who needs to act on this?** That answers reach. Sub-agents do not see anything outside the repo, so anything multi-agent must live in the repo.
2. **What shape is the content?** A rule (steps, applicability, anti-patterns) goes in a discipline; a snapshot (point-in-time fact) goes in `.claude/state/`; cross-session task continuity goes in the task's Implementation Notes.
3. **Cost of one slip is high?** Promote to a gate with a clear escape sentinel.
4. **Conversation context that is not actionable next session?** Let it scroll. Re-deriving from source on the next session is cheaper than carrying stale advice forward.

## Register translation

When promoting from user prose into a repo venue, paraphrase into discipline-file register — operative rule first, structured sections, no first-person or session-specific narrative. Match the existing universal disciplines' tone. Translate the substance; drop the framing.

## Cross-references

- `workspace-hygiene.md` — *the backlog is the only AI-authored project-state medium for tasks* is the operational form of the no-rogue-docs rule. `.claude/state/` files are not project-state-as-tasks; they are point-in-time snapshots the dispatcher needs at session start.
- `backlog-workflow.md` — Implementation Notes are the cross-session medium subagents see; that discipline governs how they get written.
