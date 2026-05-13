---
id: TASK-187
title: Sub-agent discipline compliance — enforcement (hooks or manifest inlining)
status: In Progress
assignee: []
created_date: '2026-04-28 18:10'
updated_date: '2026-05-13 23:03'
labels:
  - harness
  - hooks
  - discipline
  - meta
  - enforcement
dependencies: []
references:
  - .claude/agents/
  - .claude/disciplines/
  - .claude/hooks/
  - CLAUDE.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"we don't simulate a real human, but employing an AI agent, so it must comply with all our rules set for them."*

Today, sub-agent discipline compliance is **by instruction, not by enforcement**:

- `CLAUDE.md` § "Agents and dispatch" lists universal disciplines every agent reads.
- Each `.claude/agents/<name>.md` manifest tells the subagent which disciplines apply.
- The dispatcher's brief names specific disciplines inline.

What we DON'T have: any mechanism that **verifies** the agent actually loaded the discipline content into its context window before acting. Compliance relies on the agent following its instructions to read the files.

Observed evidence agents do comply (they cite disciplines by name in findings, follow rules not in the brief, etc.) but no hook-level enforcement. An agent CAN technically skip the read and pattern-match on the brief alone.

### Required

A mechanism that turns "agent should read disciplines" into "agent has read disciplines" with verifiable evidence.

### Design space (ai-expert refines)

1. **Inline discipline content into agent manifests** — paste the universal disciplines verbatim into each `.claude/agents/<name>.md` body. The manifest IS the subagent's system prompt; inlining guarantees the content is present. Pros: simple, no runtime cost. Cons: discipline edits require manifest re-sync (a hook can audit drift). Bloats every dispatch's system prompt.

2. **Auto-include directives in manifests + session-start hook expansion** — manifests carry `<!-- AUTO-INCLUDE: ../disciplines/<name>.md -->` markers; a session-start hook (or agent-dispatch hook) expands them. Avoids stale content. More mechanism to maintain.

3. **Pre-tool-use hook on Agent invocations** — inspect the dispatcher's brief for required discipline references; reject if absent. Catches dispatcher-side noise but doesn't verify the agent itself read the files. Compliance is still partial.

4. **Agent-internal acknowledgment gate** — every agent's first response must include a structured "disciplines I internalized" block; a hook validates the block before any code/edit tool runs. Catches the agent skipping reads but adds latency + brittle-format risk.

5. **Hybrid (1 + 3)** — inline disciplines into manifests for guarantee; pre-tool-use hook on Agent for dispatcher-side rule references. Manifest-inlining covers the agent side, hook covers the dispatcher side.

### Acceptance shape

- A chosen mechanism + rationale (a/b/c framing per `tech-choice-vs-default.md`).
- Implementation: hook code, manifest changes, audit script for drift if applicable.
- Test: a dispatch + verification path that proves the agent has discipline content available (e.g. ask the agent to recall a specific discipline section as part of an audit dispatch).
- Documentation: cross-reference from `CLAUDE.md` and `peer-review-required.md` so future agents know the enforcement layer exists.

### Why high priority

- Foundational. Every other discipline (peer-review-required, no-rogue-fixes, tech-choice-vs-default, regression-fix-flow, etc.) compounds in value when compliance is verifiable.
- TASK-186 (disciplines refactor + template) compounds with this — a tightened, consistently-formatted discipline set is easier to enforce mechanically.

### Owner

`ai-expert` (owner of hooks + agent manifests + disciplines).

### Related tasks

- **TASK-186** (disciplines tighten + template) — pairs with this; refactor first or in parallel.
- **TASK-167** (peer-review commit gate) — same enforcement pattern, applied at commit time. This task extends the pattern to dispatch time.

### What this does NOT do

- Does not change discipline content. Pure mechanism work.
- Does not affect main-session-Claude (dispatcher) discipline reads — that's a separate concern (main session has CLAUDE.md auto-loaded; the gap is sub-agents).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Mechanism chosen with a/b/c rationale
- [ ] #2 Mechanism implemented; agents can no longer dispatch without discipline content reaching their context
- [ ] #3 Verification test demonstrates an agent has the discipline content available (read-back / cite-by-section)
- [ ] #4 Drift audit (if manifest-inlining is the mechanism) — hook or script catches manifest staleness vs. discipline source
- [ ] #5 CLAUDE.md + peer-review-required.md cross-reference the enforcement layer
- [ ] #6 Peer review per discipline
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-14 — Picked up autonomously. Two-stage approach: first dispatch design phase (AC #1) to choose mechanism with a/b/c rationale; subsequent dispatch implements chosen mechanism (ACs #2 – #6). Bounded scope per stage, peer review per `peer-review-required`.

2026-05-14 — AC #1 met: design saved at `.backlog/decisions/TASK-187-enforcement-mechanism-2026-05-14.md`. Picked mechanism: PreToolUse `skill-evidence` sub-gate in `session-gate.js`, scoped to sub-agent transcripts, blocking non-passive tool calls until the transcript shows `Skill(<name>)` for every skill on the manifest's always-apply line. Three-reference rationale (training-default / SOTA / project-precedent) names `task-mgmt-brief.js` + `peer-review.js` as the precedent pattern. MVP slice: `code-impl`, `shader-impl`, `harness-impl` first. Implementation (ACs #2 – #6) deferred to a follow-up dispatch.
<!-- SECTION:NOTES:END -->
