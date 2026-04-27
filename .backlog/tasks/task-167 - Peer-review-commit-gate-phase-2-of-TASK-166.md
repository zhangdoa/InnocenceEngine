---
id: TASK-167
title: 'Peer-review commit-gate (phase 2 of TASK-166)'
status: To Do
assignee: []
created_date: '2026-04-26 23:00'
labels:
  - infrastructure
  - harness
  - commit-gate
dependencies:
  - TASK-166
priority: high
references:
  - .claude/disciplines/peer-review-required.md
  - .claude/hooks/commit-gate.js
  - .claude/hooks/gates/attribution.js
  - .claude/hooks/gates/paper-port.js
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->

Phase 2 of TASK-166 — the harness side of `peer-review-required.md`. TASK-166 phase 1 landed the discipline doc and the dispatcher pattern in `CLAUDE.md`. This task adds the commit-gate that enforces the artefact (`Reviewed-By:` / `Review-Skipped:` line in the commit message), so the discipline is durable against compliance drift the way the attribution and paper-port gates are.

### What lands here

1. **`.claude/hooks/gates/peer-review.js`** — new gate, transcript-independent, attribution-shaped: blocks any `git commit` whose message lacks one of:
   - `Reviewed-By: <reviewer-agent>` (one or more)
   - `Review-Skipped: <reason>`
   The reasons enumerated in `peer-review-required.md` § "When required" are the only legitimate skip categories; the gate does not enumerate them itself (loose match — the reviewer of THIS gate's CL would call out a fake reason in review). Loud failure shape modelled on `attribution.js`.
2. **Wire-up in `.claude/hooks/commit-gate.js`** — add the gate to `GATES`, transcript-independent phase, before `attribution` (peer review is a richer claim; if review was skipped, attribution can still fail and the user gets the more informative error first).
3. **Test coverage** — extend the harness test suite to cover: missing line → block; `Reviewed-By:` present → pass; `Review-Skipped:` present → pass; multiple `Reviewed-By:` → pass; line in `-F` file → pass; line missing while attribution present → block.
4. **Wire the artefact into `commit-message-policy.md`** — append a section listing `Reviewed-By:` / `Review-Skipped:` alongside the existing `Code-AI-Generated-By:` line as standard commit-message footer fields.

### Skip-sentinel question

Unlike `[skip-test-gate]`, peer-review does not get a sentinel. The artefact IS the audit trail — `Review-Skipped: <reason>` is the explicit-skip path, surfaced in `git log` after the fact, the same shape attribution uses (no escape sentinel because attribution is the audit trail). Putting `[skip-review-gate]` in addition would dilute the artefact.

### Acceptance criteria for the gate test fixtures

The gate's test cases cover the four skip categories from `peer-review-required.md`:

- Backlog/docs-only non-closing CL — `Review-Skipped: backlog-only` passes; closing CL with same reason fails (closing claim warrants review).
- Harness self-edit where the diff IS gate logic — `Review-Skipped: hook-internal` passes.
- Mechanical refactor (rename, file move) — `Review-Skipped: mechanical-rename` passes.
- Bootstrap (the discipline-introducing CL itself) — `Review-Skipped: bootstrap` passes (one-shot category, mostly for completeness).

The gate does not validate the *truthfulness* of the reason — that is reviewer / dispatcher discipline. The gate enforces that a reason is explicitly stated.

### Why high priority

`peer-review-required.md` is universal-list as of TASK-166 phase 1, but phase 1 is discipline-only. The user has already flagged `regression-fix-flow.md` (also discipline-only at the time) for repeated violations. Without the gate, peer-review is on the same compliance trajectory; the user will rightfully flag it again. Land before the next big feature lane.

### Owner

`ai-expert` (harness, hooks, commit-gate, disciplines).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `.claude/hooks/gates/peer-review.js` lands; loud-failure shape mirrors `attribution.js`
- [ ] #2 `commit-gate.js` `GATES` array includes the new gate, transcript-independent phase, before `attribution`
- [ ] #3 Harness test suite covers: missing line block, both line forms pass, multiple `Reviewed-By:` pass, file-mode (`-F`) pass, attribution-without-review block
- [ ] #4 `commit-message-policy.md` § "Standard Format" lists `Reviewed-By:` / `Review-Skipped:` alongside `Code-AI-Generated-By:`
- [ ] #5 The gate's own landing CL goes through peer review (dogfood — first hard pass of the discipline backed by the gate)
<!-- AC:END -->
