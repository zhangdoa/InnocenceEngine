---
id: TASK-209
title: Promote surface-dont-chase feedback to universal discipline
status: Done
assignee:
  - ai-expert
created_date: '2026-04-30 23:00'
labels:
  - harness
  - discipline
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

Behavior pattern fired multiple times in recent sessions: when implementation surfaces work beyond the originally-scoped task, agents and dispatchers reflexively expand the current CL to chase the discovery instead of closing-or-handing-back-and-filing. Most recent instance: TASK-77.1.3 closure shape — implementer surfaced a D9 root cause; dispatcher dispatched a fix; the fix surfaced three more algorithmic gaps; dispatcher kept escalating instead of closing-and-logging.

The rule is universal (all agent roles, every CL) and complements `feedback_dont_pile_on_backlog_tasks` (memory). It belongs in the harness, not in memory — auto-memory does not reach spawned subagents per `persistence-venue.md`.

## Scope

- Author `.claude/disciplines/surface-dont-chase.md` matching the operative-core register of recent compressed disciplines (~30-50 lines).
- Wire into the universal block in root `CLAUDE.md` (every-agent preamble).
- Extend `persistence-venue.md` with a paraphrase-during-promotion rule (governs *how to write* harness content during memory-to-repo migration).

## Acceptance criteria

- [x] `.claude/disciplines/surface-dont-chase.md` exists; covers the rule, three workflow points (implementer / dispatcher / main-session), what-this-is-not section pinning the relationship to `feedback_dont_pile_on_backlog_tasks`, and one recorded incident.
- [x] Root `CLAUDE.md` universal block includes the new file path.
- [x] `persistence-venue.md` covers the authoring-register-during-promotion rule.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

Landed in same CL as authoring. Author's-note rule placed in `persistence-venue.md` rather than the new file: that file already governs venue + migration semantics, so extending to *how to write* during promotion is a clean fit; coupling it to one discipline would have scoped the rule too narrowly.

Name choice rationale: `surface-dont-chase` over `bounded-execution` / `discovered-scope` / `scope-discipline`. The chosen name is an imperative verb-pair (matches `cite-prior-art`, `split-before-grow`, `tech-choice-vs-default`), with the operative verbs of the rule (*surface* / *chase*) on the file's face.

Review-Skipped: hook-internal — harness self-edit by the harness owner, markdown-only.
