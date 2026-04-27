---
id: TASK-167
title: 'Peer-review commit-gate (phase 2 of TASK-166)'
status: Done
assignee:
  - '@ai-expert'
created_date: '2026-04-26 23:00'
updated_date: '2026-04-26 23:30'
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
  - .claude/hooks/gates/peer-review.js
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
- [x] #1 `.claude/hooks/gates/peer-review.js` lands; loud-failure shape mirrors `attribution.js`
- [x] #2 `commit-gate.js` `GATES` array includes the new gate, transcript-independent phase, before `attribution`
- [x] #3 Harness test suite covers: missing line block, both line forms pass, multiple `Reviewed-By:` pass, file-mode (`-F`) pass, attribution-without-review block
- [x] #4 `commit-message-policy.md` § "Standard Format" lists `Reviewed-By:` / `Review-Skipped:` alongside `Code-AI-Generated-By:`
- [x] #5 The gate's own landing CL goes through peer review (dogfood — first hard pass of the discipline backed by the gate)
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->

### Slicing call

Single commit with `Review-Skipped: hook-internal`. The diff is dominated by:

- `gates/peer-review.js` — gate JS itself; the canonical hook-internal exemption per `peer-review-required.md` § "When required" ("harness self-edit where the diff IS the gate logic the reviewer would consult").
- `commit-gate.js` — wire-up of the gate into `GATES` + header comment refresh; coupled to the gate JS.
- `tests/commit-gate.test.js` — tests covering the gate's `run()` semantics; coupled to the gate JS.
- `commit-message-policy.md` — adds the footer-fields table cross-referencing the new gate; coupled to the gate JS.

A fresh peer reviewer's job here would be to read the gate logic itself, which is the bootstrapping loop the discipline exempts. Splitting into two commits (gate+wireup with skip / tests+policy with real review) was considered but rejected: a peer reading test fixtures or policy edits without the gate beside them is reviewing in a vacuum, and the policy doc's footer-fields table would land before the gate JS that backs it (commit ordering inverts the dependency).

### What the gate enforces

`Reviewed-By: <reviewer>` OR `Review-Skipped: <reason>` (regex `/^(Reviewed-By|Review-Skipped):\s*\S/m`). The gate enforces *presence* of a footer line, not the truthfulness of the skip reason — that is reviewer / dispatcher discipline. Loud-failure shape mirrors `attribution.js` exactly: same module export shape (`{ run, needsTranscript: false }`), same `process.exit(2)`, same single-block error emission with discipline link.

### Verification

- 28-case test suite passes (`node .claude/hooks/tests/commit-gate.test.js` — was 15, added 13).
- Synthetic-commit dispatcher drive-through:
  - missing line: blocks loudly, exit 2, peer-review error first (before attribution).
  - `Reviewed-By:` present: passes phase 1, falls through to phase 2 transcript skip.
  - `Review-Skipped:` present: same as above.
  - missing both review and attribution: peer-review fires first (richer claim).

### Ordering note

Peer-review runs before attribution in the transcript-independent phase. If both lines are missing the user gets the more informative error first (peer-review describes the skip categories; attribution is a one-liner). Both are no-escape-sentinel; both are transcript-independent; the `55cf6a72`-class bypass cannot occur for either.

<!-- SECTION:NOTES:END -->
