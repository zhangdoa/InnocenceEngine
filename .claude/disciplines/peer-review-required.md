# Discipline: peer-review-required

Every non-trivial implementation dispatch is followed by a peer-review dispatch before the implementing agent's work commits. Self-validation is necessary but not sufficient — the implementing agent has motivated reasoning to call work done.

## Why

A dispatcher's loop today: brief → implementer → implementer self-validates → implementer commits. The implementer is the only set of eyes on the diff. Self-validation catches build-breaks and the AC checks the brief listed; it routinely misses log-spam, dead branches, contract drift, and duplicated logic — categories the implementer is least primed to see because they are the author of what produced them.

The user flagged this concretely after TASK-140 shipped per-pass-per-frame Verbose log spam (now TASK-165) and TASK-138 phase 1 used the timer infra without surfacing it. Two consecutive dispatches missed the same defect because both were inside the implementer's frame. *"the discipline is violated, i think we lack a review stage in general."*

The fix is structural: a fresh agent reads the diff against the brief and the disciplines, with no exposure to the implementer's reasoning trace. The reviewer is itself an agent dispatch, so its findings are surfaced where the dispatcher can route them rather than living in main-session conversation.

## When required

A peer-review dispatch is required for any implementation dispatch whose result will be committed, **except**:

- **Backlog/docs-only CLs** matching the same exemption the test-run gate uses (`.backlog/`, `Documents/`, `.md`, `.claude/`, `.alignments/`, `.gitignore`) — *unless* the CL is closing a task, in which case the closure claim is itself the substantive change and warrants a review pass.
- **Hook fixes / harness self-edits where the diff is purely the gate logic the reviewer would consult.** Avoid the bootstrapping loop. Surface in the commit message which gate's logic was edited; this is the equivalent of the test-run gate's `[skip-test-gate]` for harness-internal CLs.
- **Mechanical refactors with a single deterministic transformation** (rename, file move, generated-data sweep) where there is no design surface for a reviewer to evaluate. The dispatcher must be able to articulate why the transformation has no design surface; if it can, no review.

If the dispatcher is unsure whether a dispatch qualifies for an exception, the default is to require review. Skipping review is opted into in writing, not by omission.

## Who reviews

The reviewer is an agent dispatch, never the dispatcher (main-session Claude) and never the implementing agent. The dispatcher and implementer share context and motivated reasoning; only a fresh dispatch breaks that frame.

Choose the reviewer in this order:

1. **Peer in the same role family** — another `graphics-api-expert` reviewing `graphics-api-expert` work, etc. Catches domain-specific issues (DX12 barrier invariants, paper-port spec drift, threading contracts) that a general reviewer misses. This is the default.
2. **`software-architect`** — when the implementer's role does not have a meaningful peer (sole owner of a subtree) or when the CL crosses two role-family boundaries and a structural read is the right lens.
3. **`superpowers:code-reviewer`** — fallback for CLs that do not map to a project role family at all (rare; most CLs in this repo do).

The reviewer is dispatched *fresh* — same model, but a new sub-agent invocation with no carryover from the implementer's session. The brief gives them: the diff, the original implementation brief, the relevant disciplines, and any anchored invariants. They do not see the implementer's reasoning trace.

## What the reviewer checks

The reviewer is biased to find issues, not to confirm correctness — same property as `paper-audit.md`. PASS is asserted only with line-grounded evidence; absence of obvious issues is not evidence.

The review pass covers, in order:

1. **The brief's acceptance criteria** — every AC actually met by the diff, not by the implementer's assertion that it is met.
2. **The anchored invariants** — every invariant the brief called out, checked against the diff.
3. **The universal disciplines** — `coding-principles.md` (no workarounds, fix at the right layer), `comment-discipline.md` (no explanatory comments), `target-qualities.md` (orthogonality, explicit contracts, fail loudly, reload-safe), `commit-message-policy.md` (attribution).
4. **Role-specific disciplines** — whichever apply to the implementer's role family (`paper-port.md`, `shader-standards.md`, `threading-contracts.md`, etc.).
5. **Structural fit** — does any added logic belong one layer up or one layer down? Are there explanatory comments masking unclear code instead of clarifying it? Is failure loud where invalid input arrives? Could the change have been smaller?
6. **Defects the implementer is least primed to see** — log-spam (per-frame channels), dead branches, copy-paste duplication, magic numbers, silent guards, hardcoded paths. The historical pattern from TASK-140/165.

The reviewer's verdict is one of:

- **PASS** — the diff meets the brief and the disciplines, with line-grounded evidence cited for every checked item. PASS is not "no findings"; it is "checked all of the above and they hold."
- **BLOCKED** — at least one finding the reviewer believes the implementer should address before commit. Each finding cites file:line and the discipline / invariant violated.
- **ADVISORY** — findings the reviewer surfaces for the implementer's awareness but that do not block commit (style nudges, follow-up backlog seeds, paths the implementer should consider for a later CL).

A review that returns PASS without specifics is itself a defect — the implementer / dispatcher should re-dispatch with a tighter brief.

## How findings flow back

The reviewer writes findings into the owning task's Implementation Notes section, in a `## Review (<reviewer-agent>, <date>)` block. PASS / BLOCKED / ADVISORY appears in the block header; findings are itemised below with file:line and discipline citations.

The implementing agent then either commits (on PASS or PASS-with-ADVISORY) or addresses the BLOCKED findings and re-dispatches review.

## Loop bound

Maximum two review iterations before escalation. The sequence is:

1. Implementer dispatches → review #1.
2. If BLOCKED, implementer addresses findings → review #2.
3. If review #2 is also BLOCKED, the dispatcher surfaces both reviews to the user with the implementer's response and asks for direction.

This prevents an unbounded reviewer-vs-implementer disagreement loop. The user resolves anything that survives two passes; both reviews are kept on the task for context.

## Commit-message artifact

The implementing agent records the review in the commit message with:

```
Reviewed-By: <reviewer-agent>
```

Multiple `Reviewed-By:` lines are valid (e.g. peer + architect on a cross-cutting CL). Skip cases (per "When required" above) record the reason instead, on a single line:

```
Review-Skipped: <reason — backlog-only / hook-internal / mechanical-rename / etc>
```

The line is what makes the discipline auditable in `git log` after the fact, the same way `Code-AI-Generated-By:` makes attribution auditable.

## Harness enforcement

Phase 1 (this CL) is discipline-only. Compliance is dispatcher behaviour, surfaced in the commit-message artifact above.

Phase 2 — tracked under TASK-167 — adds a `peer-review` gate to `commit-gate.js` that requires a `Reviewed-By:` or `Review-Skipped:` line on every commit, with the same loud-failure shape as the attribution gate. Until that lands, the discipline relies on dispatcher compliance and reviewer existence on the task — same compliance posture as `regression-fix-flow.md` between its prose-only era and any future hook backing.

## Anti-patterns

- **Dispatcher self-review.** Main-session Claude reading its own implementer's diff and writing PASS is the failure this discipline targets. The reviewer is always a fresh agent dispatch.
- **Implementer self-review under a different label.** Same agent re-running with a "review hat" prompt is not a peer review; it is the same context with a new instruction. The dispatcher must invoke a separate `Agent` tool call, with the reviewer role distinct from the implementer role on the brief.
- **Rubber-stamp PASS.** A review that returns PASS with no line-grounded checks satisfies the artifact (`Reviewed-By:` line) but defeats the discipline. The reviewer's manifest section on this discipline is the corrective; if PASS-without-evidence becomes a pattern, tighten reviewer briefs and route the failure mode back here.
- **"It's a small change" as a skip rationale.** Smallness is orthogonal to whether a fresh read catches a defect. The user's triggering case (TASK-140 log spam) was a small CL. Skip categories are listed above; "small" is not one of them.
- **Treating the review block as the commit message.** Findings live in the task's Implementation Notes, not the commit body. The commit records the *fact* of review (`Reviewed-By:`) and the dispatcher's response to BLOCKED findings, not the review verbatim.
