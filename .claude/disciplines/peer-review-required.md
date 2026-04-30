# Discipline: peer-review-required

Every non-trivial implementation dispatch is followed by a peer-review dispatch before commit. The implementer has motivated reasoning; only a fresh agent breaks the frame. Long-form rationale + recorded incidents in `peer-review-extras.md`.

## When required

Required for every implementation dispatch whose result will be committed, **except**:

- **Backlog/docs-only CLs** — same exemption as the test-run gate (`.backlog/`, `Documents/`, `.md`, `.claude/`, `.alignments/`, `.gitignore`). Exception: if the CL is closing a task, the closure claim is itself the substantive change → review required.
- **Hook fixes / harness self-edits** where the diff is purely the gate logic the reviewer would consult (avoids bootstrapping loop). Surface in the commit message which gate's logic was edited.
- **Mechanical refactors with a single deterministic transformation** (rename, file move, generated-data sweep) — the dispatcher must articulate why there is no design surface.

If unsure, default to required. Skips are opted into in writing.

## Reviewer selection

The reviewer is always a fresh agent dispatch — never main-session Claude, never the implementer. Choose in this order:

1. **Peer in the same role family** (default) — another `graphics-api-expert` reviewing `graphics-api-expert` work, etc. Catches domain-specific issues a generalist misses.
2. **`software-architect`** — when the implementer's role has no peer (sole subtree owner) or the CL crosses two role-family boundaries.
3. **`superpowers:code-reviewer`** — fallback for CLs that map to no role family (rare).

The reviewer brief contains: the diff, the original implementation brief, relevant disciplines, anchored invariants. It does NOT contain the implementer's reasoning trace.

## Verdict tiers

- **PASS** — diff meets brief and disciplines, with line-grounded evidence cited for every checked item. Not "no findings"; "checked all and they hold."
- **BLOCKED** — at least one finding the implementer must address. Each finding cites file:line and the discipline / invariant violated.
- **ADVISORY** — AC met, with non-blocking observations (style nudge, perf cleanup, follow-up seed). Ships with notes.
- **UNVERIFIED** — AC technically met but **not visually / runtime confirmed** because test infra cannot reach the code path. Does NOT ship until either (a) test infra is extended, OR (b) user explicitly acknowledges shipping unverified. See `peer-review-extras.md` § UNVERIFIED tier for ADVISORY-vs-UNVERIFIED distinction and dispatcher obligation.

After review:
- **PASS / PASS+ADVISORY** — implementer commits.
- **BLOCKED** — implementer addresses → re-dispatch review.
- **UNVERIFIED** — dispatcher surfaces to user; no commit until user picks (a) or (b).

## Loop bound

Maximum two review iterations. If review #2 is still BLOCKED, dispatcher surfaces both reviews + implementer response to the user. The user resolves.

## Findings flow

The reviewer writes findings into the owning task's Implementation Notes as a `## Review (<reviewer-agent>, <date>)` block. Verdict in the header; findings itemised with file:line + discipline citations.

## Commit-message line

Every commit records review status:

```
Reviewed-By: <reviewer-agent>
```

Multiple `Reviewed-By:` lines are valid (peer + architect on cross-cutting CL). Skip cases record reason instead, on a single line:

```
Review-Skipped: <reason — backlog-only / hook-internal / mechanical-rename / etc>
```

The `peer-review` gate inside `commit-gate.js` (TASK-167) enforces presence with the same loud-failure shape as the attribution gate.

## Long-form

`peer-review-extras.md` — full rationale, anti-pattern catalogue, recorded incidents (TASK-140/165, TASK-166/167, TASK-176/177).

## Cross-references

- `backlog-workflow.md` — task → dispatch → review → commit.
- `commit-message-policy.md` — owns the `Reviewed-By:` / `Review-Skipped:` line format.
- `paper-audit.md` — same issue-biased property.
- `visual-validation.md` — UNVERIFIED tier for visual ACs not exercisable by current infra.
- `agent-dispatch.md` — reviewer dispatches default to background.
