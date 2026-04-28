---
id: TASK-186
title: 'Disciplines: tighten + refactor + common template (fragmented + inconsistent)'
status: To Do
assignee: []
created_date: '2026-04-28 17:50'
labels:
  - harness
  - documentation
  - discipline
  - meta
dependencies: []
priority: high
references:
  - .claude/disciplines/
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"the disciplines are fragmented, some are concise and others are verbose, format are messed up — tighten and refactor them and get a common template for future disciplines."*

The `.claude/disciplines/` set has accumulated organically across many sessions. Each new discipline (recent: `peer-review-required.md`, `tech-choice-vs-default.md`, `perf-measurement-frame-budget.md`) was written by whichever ai-expert dispatch happened to land it, with no shared template. Result:

- Depth varies wildly — some disciplines are 30-line concise rules; others are 100+ lines with multiple subsections.
- Section headings drift — "Why / When / How / Anti-patterns / Cross-references" exist in some but not all; some use `## Rule`, others `## Required fix`, others bare prose.
- Anti-pattern lists vary from "explicit bullet list" to "implicit prose."
- Cross-reference style varies — some bracket-link to other disciplines, others footnote, others mention by name only.
- Cite-prior-art coverage varies — some disciplines name the triggering incident with task IDs (TASK-176/177 for `peer-review-required.md`), others don't.

Every agent reads these on every dispatch (per `CLAUDE.md` § "Agents and dispatch"). Inconsistent format inflates parsing cost and produces uneven enforcement.

### What this delivers

1. **A canonical template** for new disciplines, documented at `.claude/disciplines/_TEMPLATE.md` (or similar). Sections, ordering, depth guidance, citation style, anti-pattern format. The template itself sets the tone.
2. **Refactor existing disciplines** to match the template. Tighten verbose ones; expand concise ones if a section is missing; normalize headings and cross-references.
3. **Cross-reference index** — every discipline lists its paired disciplines (cite-prior-art ↔ tech-choice-vs-default; peer-review-required ↔ backlog-workflow; etc.).
4. **Audit incident citations** — each discipline names the incident(s) that triggered it (ai-expert can grep backlog for related task IDs).

### Template sketch (ai-expert refines)

Suggested sections for the canonical template:

```md
# Discipline: <name>

## Rule

(One-paragraph load-bearing rule.)

## Why

(Failure mode this prevents. Cite triggering incident with task ID(s) if known.)

## When

(Triggers / scope — when does this apply, when does it not.)

## How

(Concrete steps / format / artifact requirements.)

## Anti-patterns

- (Bullet list of patterns the rule explicitly disallows, each with one-line rationale.)

## Cross-references

- Paired disciplines (e.g. `peer-review-required.md` for review, `tech-choice-vs-default.md` for picking among alternatives).
```

### What this does NOT do

- Does NOT change the substantive rules in any discipline. Pure refactor: same content, normalized form.
- Does NOT remove any discipline. Even small / single-rule disciplines stay; they just match the template's minimum-viable shape.
- Does NOT touch the `feedback_*.md` auto-memory files (those are user-scope, separate ownership).

### Why high priority for next session

User's stated principle: agents need consistent, fast-to-parse rules. The recent material-bug thrash + my dispatcher-side temp-patch reflex revealed how much rule-internalization matters under pressure. Tightening these BEFORE the next big lane lands is leverage.

### Owner

`ai-expert` (owner of `.claude/disciplines/`).

### Coordination

This task is a refactor; mechanical-with-design-surface (the template choices have lasting consequences). Per `peer-review-required.md` — peer reviewer is another `ai-expert` invocation reading the template + sample-refactored disciplines. Not `Review-Skipped: mechanical-rename` — there IS a design surface (the template).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Canonical template lands at `.claude/disciplines/_TEMPLATE.md` (or equivalent) with section ordering + depth guidance documented
- [ ] #2 All existing `.claude/disciplines/*.md` refactored to the template; substantive content unchanged
- [ ] #3 Cross-reference index is bidirectional — every discipline's "paired" entries match the inverse paired entries on the other doc
- [ ] #4 Triggering incident cited (with task ID) on every discipline that has one
- [ ] #5 Diff visibly tightens: total line count across `.claude/disciplines/` drops or stays flat (no per-discipline length doubled)
- [ ] #6 Peer review per discipline (ai-expert peer reviewer)
<!-- AC:END -->
