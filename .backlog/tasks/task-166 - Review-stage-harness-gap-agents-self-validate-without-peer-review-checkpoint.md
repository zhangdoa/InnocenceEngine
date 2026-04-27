---
id: TASK-166
title: 'Review-stage harness gap: agents self-validate without peer-review checkpoint'
status: To Do
assignee: []
created_date: '2026-04-27 19:30'
labels:
  - infrastructure
  - harness
  - discipline
  - meta
dependencies: []
priority: high
references:
  - .claude/CLAUDE.md
  - .claude/disciplines/
  - .claude/hooks/commit-gate.js
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
- [ ] #1 Design call published in this task's Implementation Notes — which option (1/2/3/hybrid), with rationale, citing existing assets
- [ ] #2 If discipline-only: `.claude/disciplines/peer-review-required.md` lands + `CLAUDE.md` references it
- [ ] #3 If hook-enforced: `commit-gate.js` adds the `peer-review` gate with appropriate skip sentinel; tests added to existing test suite
- [ ] #4 If dispatch-pattern: `CLAUDE.md` "Agents and dispatch" section amended; example dispatcher flow documented
- [ ] #5 The fix-for-the-fix: this task itself goes through whatever review pattern it produces (dogfood verification)
<!-- AC:END -->
