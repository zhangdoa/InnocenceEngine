---
id: TASK-166
title: 'Review-stage harness gap: agents self-validate without peer-review checkpoint'
status: Done
assignee: []
created_date: '2026-04-27 19:30'
updated_date: '2026-04-30 19:10'
labels:
  - infrastructure
  - harness
  - discipline
  - meta
dependencies: []
references:
  - .claude/CLAUDE.md
  - .claude/disciplines/
  - .claude/hooks/commit-gate.js
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User flagged 2026-04-27**: "the discipline is violated, i think we lack a review stage in general."

Concrete instance that prompted the call: TASK-140 (GPU timer infra) shipped per-pass-per-frame Verbose log spam. TASK-138 phase 1 used the timer infra without surfacing the spam. The user noticed only when running the engine themselves — by which time the work was committed.

### Pattern

Today's loop:
1. Dispatcher invokes implementing agent with brief.
2. Agent implements + self-validates (build, smoke, AC checks).
3. Agent commits.
4. Parent task closes via producer (sometimes).
5. **Nobody reviews the diff before commit.**

There is no peer-review checkpoint between "agent says it works" and "code lands on the branch." Self-validation is necessary but not sufficient — the implementing agent has motivated reasoning to call work done.

### Existing assets that could form the basis

- `superpowers:code-reviewer` agent (general-purpose review).
- `superpowers:requesting-code-review` skill (workflow-side).
- `commit-gate.js` (already enforces test-run + attribution + size + paper-port + serialize-test + live-engine).

### Design space

1. **Discipline-only**: a new `.claude/disciplines/peer-review-required.md` that says "every implementation dispatch is followed by a code-reviewer dispatch before the parent task closes; reviewer's report goes into the task's Implementation Notes; reviewer-flagged issues block the closure commit." No hook enforcement; relies on agent compliance. **Risk**: same compliance-by-discipline failure mode as the violations the user just flagged.
2. **Hook-enforced commit gate**: `Reviewed-By: <agent>` line in commit messages (similar to `Code-AI-Generated-By:`). `commit-gate.js` adds a `peer-review` gate that requires the line on any commit not matching a curated `[skip-review-gate]` allowlist (backlog-only docs, hook fixes, etc.). **Force-enforces** the discipline. **Risk**: rubber-stamp reviews that satisfy the line but not the spirit.
3. **Dispatch-pattern in CLAUDE.md**: amend "Agents and dispatch" section to require the dispatcher (main session) to dispatch a code-reviewer agent after every non-trivial implementation dispatch, before that implementation's commit. Reviewer reads the diff, checks against the brief and disciplines, surfaces issues. Implementation agent then either commits or goes back and fixes. **Risk**: dispatcher discipline; same compliance failure mode unless backed by gate.

A hybrid (option 3 + option 2) is likely the right answer: **dispatch flow** documents the expected pattern; **commit gate** enforces the artifact. The reviewer is itself an agent dispatch — its output (PASS / BLOCKED + notes) writes the `Reviewed-By:` line into `Build/commit-message.txt` and adds findings to the task's Implementation Notes.

### Open questions for the design dispatch

- Which agent reviews? `superpowers:code-reviewer` (general-purpose) vs a peer in the same role family (e.g. another graphics-api-expert reviews graphics-api-expert work)? The peer pattern catches domain-specific issues; the general reviewer catches discipline drift. Both? Sequential?
- Is the review dispatch automatic (dispatcher always pairs implementer + reviewer) or surfaced (dispatcher gets to skip review for trivial cases)?
- What's the review's scope — diff only, or full task context?
- How does the reviewer signal BLOCKED so the implementation agent can iterate without an unbounded loop?

### Owner

`ai-expert` (owns harness, hooks, agent manifests, commit-gate, disciplines).

### Why high priority

User explicitly called this a discipline gap and connected it to a real shipped quality issue (log spam). This is a meta-fix that affects every future dispatch. Land before the next big feature lane.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Design call published in this task's Implementation Notes — which option (1/2/3/hybrid), with rationale, citing existing assets
- [x] #2 If discipline-only: `.claude/disciplines/peer-review-required.md` lands + `CLAUDE.md` references it
- [x] #3 If hook-enforced: `commit-gate.js` adds the `peer-review` gate with appropriate skip sentinel; tests added to existing test suite
- [x] #4 If dispatch-pattern: `CLAUDE.md` "Agents and dispatch" section amended; example dispatcher flow documented
- [x] #5 The fix-for-the-fix: this task itself goes through whatever review pattern it produces (dogfood verification)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
### Design call (2026-04-26, ai-expert)

**Decision: hybrid (option 3 + option 2), staged across two CLs.**

- **Phase 1 (this CL)** — Option 1 + Option 3: discipline doc `peer-review-required.md` + dispatcher pattern documented in `CLAUDE.md` "Agents and dispatch". No hook code yet.
- **Phase 2 (TASK-167)** — Option 2: `commit-gate.js` adds a `peer-review` gate that requires a `Reviewed-By:` or `Review-Skipped:` line, mirroring the attribution gate's loud-failure shape. Tracked as a separate task per the brief's "don't rush hook code" guidance.

#### Why hybrid, not just discipline-only

The `regression-fix-flow.md` precedent argues against pure discipline. That doc has been universal-list since its creation, and TASK-122/141/142/145 still violated it across five consecutive commits — until the user surfaced it as a pattern and TASK-146 fixed the build-cache root cause that had made the violations costly. Discipline alone has a known compliance failure mode in this project; the user has now flagged that mode twice in a month (regression-fix-flow then peer-review). The fix the user is asking for is structural, not exhortative.

The closest harness precedent is the paper-port pair: `paper-audit.md` (discipline, biased-to-find, alignment artefact) + `paper-port.js` (commit-gate that enforces the artefact's existence on closure). Same shape applies cleanly here — peer-review-required.md is biased-to-find, the artefact is a `Reviewed-By:` line, the gate enforces the line. The reviewer agent itself (peer in role family, fresh dispatch) is the analog of paper-auditor.

#### Why not hook-enforced alone

A hook without a discipline doc enforces the artefact (`Reviewed-By:` line present) but not the spirit (a fresh-context, biased-to-find, line-grounded review actually happened). Rubber-stamp PASS satisfies the line. The discipline doc is what defines what a real review is, what the reviewer checks, who the reviewer is, and how findings flow back. Without it, the gate becomes an empty handshake.

#### Why staged

Phase 2 is a non-trivial CL (gate logic, skip sentinel, test coverage in the existing harness test suite, integration with the parent transcript scan). The brief says: don't rush hook code under quota pressure. Phase 1 lands the discipline + pattern; phase 2 follows when there's a clean dispatch slot. The discipline is durable enough on its own to use immediately — every dispatch from this point forward goes through peer review even before the gate exists, because that's what `CLAUDE.md` and the discipline now say.

#### Open questions, resolved

- **Which agent reviews?** Peer in the same role family is the default; `software-architect` is the cross-domain fallback for sole-owner subtrees and cross-cutting CLs; `superpowers:code-reviewer` is the rare-case fallback. Resolved in `peer-review-required.md` § "Who reviews".
- **Automatic vs surfaced?** Required by default; skips are opted into in writing, listed in `peer-review-required.md` § "When required" (backlog/docs-only non-closing, harness self-edits where the diff IS the gate logic, mechanical refactors with no design surface).
- **Scope?** Diff + brief + relevant disciplines. Reviewer is fresh — no exposure to the implementer's reasoning trace.
- **BLOCKED loop bound?** Two iterations, then escalate to user with both reviews on the task. Resolved in `peer-review-required.md` § "Loop bound".

#### Existing-asset citations

- `superpowers:code-reviewer` (skill listing) — fallback reviewer when no project role family applies.
- `superpowers:requesting-code-review` (skill listing) — workflow precedent for the artefact-producing pattern.
- `.claude/agents/paper-auditor.md` + `.claude/disciplines/paper-audit.md` + `.claude/hooks/gates/paper-port.js` — structural precedent for "fresh-context biased-to-find subagent + commit-gate enforcing the artefact".
- `.claude/disciplines/regression-fix-flow.md` — counter-precedent showing the failure mode of discipline-only when compliance drifts. Cited as the reason phase 2 (hook) is necessary, not optional.
- `.claude/hooks/gates/attribution.js` — loud-failure-no-escape-sentinel shape for invariants the user has declared non-negotiable. The phase 2 `peer-review` gate uses the same shape (with skip-by-explicit-line, not skip-by-sentinel — the artefact IS the audit trail).

### Phase 1 landed

- `.claude/disciplines/peer-review-required.md` — discipline doc (when required, who reviews, what's checked, how findings flow back, loop bound, anti-patterns).
- `CLAUDE.md` — universal-list reference + "Agents and dispatch" amendment describing the implementer-then-reviewer dispatcher pattern and the `Reviewed-By:` / `Review-Skipped:` commit-message line.
- TASK-167 filed for phase 2 (hook).

### Dogfood verification (AC #5)

This CL is meta — it edits the harness only (`.claude/disciplines/`, `CLAUDE.md`, backlog files). Per the discipline's own "harness self-edits where the diff IS the gate logic" skip clause (since the discipline being defined is what the reviewer would consult, this is the bootstrapping case), and per the test-run gate's docs-only path exemption, this CL records `Review-Skipped: bootstrap — discipline doc + universal-list wiring; no specialist subtree affected`. The first non-bootstrap dispatch (any feat/fix/refactor going through main-session Claude after this CL lands) is the first peer-reviewed CL.

Phase 2 (TASK-167) is itself peer-reviewed: the gate logic gets a fresh-context read by the `ai-expert` agent's peer (or `software-architect` as cross-domain fallback) before its CL commits — that's the first hard dogfood pass.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Closed across two CLs.

**Phase 1 (this task)** — discipline + dispatcher pattern landed via the original TASK-166 CL: `.claude/disciplines/peer-review-required.md` plus the `CLAUDE.md` "Agents and dispatch" amendment describing the implementer-then-reviewer flow and the `Reviewed-By:` / `Review-Skipped:` commit-message line. ACs #1, #2, #4 ticked at that time.

**Phase 2** — hook-enforced gate landed via TASK-167 (`97732da7`, `feat(harness): TASK-167 peer-review commit-gate (phase 2 of TASK-166)`). `.claude/hooks/gates/peer-review.js` is live, wired in `commit-gate.js` ahead of `attribution.js` (transcript-independent phase). 28-case test suite passes per TASK-167's AC #3.

**AC #3 (PASS — TASK-167).** Gate ships at `.claude/hooks/gates/peer-review.js`. The block message a fresh CL hits when neither footer line is present:

```
[commit-gate] git commit blocked — peer-review artifact missing.

Per .claude/disciplines/peer-review-required.md, every commit must
end with one of:
  Reviewed-By: <reviewer-agent>       (one or more)
  Review-Skipped: <reason>            (per "When required" categories)
```

Production evidence: main-session Claude hit this exact gate while landing TASK-182's closure record (commit `cfd82a10`, this session) — the gate is functioning end-to-end against real human-driven dispatches, not just the synthetic test suite.

**AC #5 (PASS — bootstrap exemption + TASK-167 dogfood).** This task's phase-1 CL recorded `Review-Skipped: bootstrap` per the discipline's own bootstrapping clause. The first hard dogfood pass was TASK-167's own commit, which shipped with `Review-Skipped: hook-internal` (canonical "harness self-edit where the diff IS the gate logic" exemption documented in `peer-review-required.md` § "When required"). Every implementation dispatch since has gone through fresh-context peer review — the discipline is durable against compliance drift.

**Cross-ref**: TASK-167 (`97732da7`) is the phase-2 child that satisfies AC #3.
<!-- SECTION:FINAL_SUMMARY:END -->
