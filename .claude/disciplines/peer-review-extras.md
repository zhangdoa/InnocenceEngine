# Peer-review extras

Long-form companion to `peer-review-required.md`. Read on demand — not in the universal preamble. The operative core is in the short file; this is the historical narrative + the full anti-pattern catalogue.

## Why (full prose)

A dispatcher's loop today: brief → implementer → implementer self-validates → implementer commits. The implementer is the only set of eyes on the diff. Self-validation catches build-breaks and the AC checks the brief listed; it routinely misses log-spam, dead branches, contract drift, and duplicated logic — categories the implementer is least primed to see because they are the author of what produced them.

The user flagged this concretely after TASK-140 shipped per-pass-per-frame Verbose log spam (now TASK-165) and TASK-138 phase 1 used the timer infra without surfacing it. Two consecutive dispatches missed the same defect because both were inside the implementer's frame. *"the discipline is violated, i think we lack a review stage in general."*

The fix is structural: a fresh agent reads the diff against the brief and the disciplines, with no exposure to the implementer's reasoning trace. The reviewer is itself an agent dispatch, so its findings are surfaced where the dispatcher can route them rather than living in main-session conversation.

## Reviewer obligations — full check ordering

The review pass covers, in order:

1. **The brief's acceptance criteria** — every AC actually met by the diff, not by the implementer's assertion that it is met.
2. **The anchored invariants** — every invariant the brief called out, checked against the diff.
3. **The universal disciplines** — `coding-principles.md`, `comment-discipline.md`, `target-qualities.md`, `commit-message-policy.md`.
4. **Role-specific disciplines** — whichever apply to the implementer's role family (`paper-port.md`, `shader-standards.md`, `threading-contracts.md`, etc.).
5. **Structural fit** — does any added logic belong one layer up or one layer down? Are there explanatory comments masking unclear code instead of clarifying it? Is failure loud where invalid input arrives? Could the change have been smaller?
6. **Defects the implementer is least primed to see** — log-spam (per-frame channels), dead branches, copy-paste duplication, magic numbers, silent guards, hardcoded paths. The historical pattern from TASK-140/165.

A review that returns PASS without specifics is itself a defect — the implementer / dispatcher should re-dispatch with a tighter brief. PASS is biased toward issue-finding (same property as `paper-audit.md`); absence of obvious issues is not evidence.

## UNVERIFIED tier — full guidance

### When to use UNVERIFIED vs ADVISORY

The distinction is whether the AC was *verified-but-imperfect* (ADVISORY) or *not-verifiable-with-current-infra* (UNVERIFIED). Reviewer findings of the shape "I cannot actually exercise this AC because <test-infra gap>" are UNVERIFIED, not ADVISORY. Style nudges, perf cleanup, and follow-up seeds remain ADVISORY.

### Dispatcher obligation on UNVERIFIED

UNVERIFIED is the only verdict tier the dispatcher MUST surface to the user before commit. PASS commits, BLOCKED loops back to the implementer, ADVISORY records and commits. UNVERIFIED stops at the dispatcher and is presented to the user with:

- Which AC is unverified.
- What test-infra gap blocked verification.
- The two options: (a) extend test infra now, or (b) ship unverified with explicit user acknowledgement.

The user picks. Commit message records the path taken: a `Review-Unverified: <AC ref> — <gap> — <a|b>` line, alongside the existing `Reviewed-By:` line. This is the audit hook so the chain is reconstructible from `git log` if a later regression traces back to an unverified AC.

## Anti-pattern catalogue (full)

- **Dispatcher self-review.** Main-session Claude reading its own implementer's diff and writing PASS is the failure this discipline targets. The reviewer is always a fresh agent dispatch.
- **Implementer self-review under a different label.** Same agent re-running with a "review hat" prompt is not a peer review; it is the same context with a new instruction. The dispatcher must invoke a separate `Agent` tool call, with the reviewer role distinct from the implementer role on the brief.
- **Rubber-stamp PASS.** A review that returns PASS with no line-grounded checks satisfies the artifact (`Reviewed-By:` line) but defeats the discipline. The reviewer's manifest section on this discipline is the corrective; if PASS-without-evidence becomes a pattern, tighten reviewer briefs and route the failure mode back here.
- **"It's a small change" as a skip rationale.** Smallness is orthogonal to whether a fresh read catches a defect. The user's triggering case (TASK-140 log spam) was a small CL. Skip categories are listed in the operative file; "small" is not one of them.
- **Treating the review block as the commit message.** Findings live in the task's Implementation Notes, not the commit body. The commit records the *fact* of review (`Reviewed-By:`) and the dispatcher's response to BLOCKED findings, not the review verbatim.
- **Classifying test-infra gaps as ADVISORY.** "AC met but I cannot actually verify it because <test-infra gap>" is UNVERIFIED, not ADVISORY. The TASK-176 → TASK-177 chain is the precedent: auto-capture tile-culling hid AC #7 verification, the dispatcher absorbed it as advisory, and the deferred fallback deletion exposed the gap downstream. Reviewers must promote test-infra-blocked AC findings to UNVERIFIED; dispatchers must not silently downgrade UNVERIFIED to ADVISORY at the commit boundary.

## Recorded incidents

**TASK-140 → TASK-165** (2026-04-25-ish). TASK-140 shipped per-pass-per-frame Verbose log spam; TASK-138 phase 1 used the same timer infra without surfacing it. Both dispatches missed the same defect because both were inside the implementer's frame. User flagged the gap.

**TASK-166** (2026-04, commit `ab67dd16`) added the discipline (phase 1, prose-only). **TASK-167** (commit `97732da7`) added the harness gate (phase 2).

**TASK-176 → TASK-177** (2026-04-28) — UNVERIFIED tier precedent. Review #2 of TASK-176 reported AC #7 (visual A/B cross-check) as PASS+ADVISORY because the auto-capture camera tile-culled all PointLights, so the 0-pixel-diff did not actually exercise the inline-RT path on visible pixels. The dispatcher treated PASS+ADVISORY as commit-eligible. TASK-177 then deleted the cube-shadow fallback that had been masking whatever AC #7 didn't verify, and the resulting regression had to be debugged downstream. Had the verdict been UNVERIFIED, the dispatcher would have surfaced it to the user.

## Cross-references

- `backlog-workflow.md` — paired layer-up: tasks must exist before dispatch (that one); review must happen before commit (this one). Together: task → dispatch → review → commit.
- `commit-message-policy.md` — owns the `Reviewed-By:` / `Review-Skipped:` line format and the gate that enforces presence.
- `paper-audit.md` — same property: biased to find issues, not confirm correctness; PASS asserted only with line-grounded evidence.
- `visual-validation.md` — UNVERIFIED tier exists for visual ACs that pass numerically but cannot be exercised by current test infra.
- `agent-dispatch.md` — peer-review dispatches default to background like all `Agent` calls; reviewer dispatches are not covered by the producer override.
