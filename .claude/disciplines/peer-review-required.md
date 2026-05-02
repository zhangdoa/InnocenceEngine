# Discipline: peer-review-required

Every non-trivial implementation dispatch is followed by a peer-review dispatch before commit. The implementer has motivated reasoning; only a fresh agent breaks the frame.

## When required

Required for every implementation dispatch whose result will be committed, **except**:

- **Backlog/docs-only CLs** — same exemption as the test-run gate (`.backlog/`, `Documents/`, `.md`, `.claude/`, `.alignments/`, `.gitignore`). Exception: if the CL is closing a task, the closure claim is itself the substantive change → review required.
- **Hook fixes / harness self-edits** where the diff is purely the gate logic the reviewer would consult (avoids bootstrapping loop). Surface in the commit message which gate's logic was edited.
- **Mechanical refactors with a single deterministic transformation** (rename, file move, generated-data sweep) — the dispatcher must articulate why there is no design surface.

If unsure, default to required. Skips are opted into in writing.

## Reviewer selection

The reviewer is always a fresh agent dispatch — never main-session Claude, never the implementer. Choose in this order:

1. **Peer in the same role family** (default) — another `graphics-api-expert` reviewing `graphics-api-expert` work, etc. Catches domain-specific issues a generalist misses.
2. **`software-architect`** — when the implementer's role has no peer (sole subtree owner), the CL crosses two role-family boundaries, or the same-family reviewer has recently let substantive issues slip past (see "Substance over trivials" below).
3. **`superpowers:code-reviewer`** — fallback for CLs that map to no role family (rare).

The reviewer brief contains: the diff, the original implementation brief, relevant disciplines, anchored invariants. It does NOT contain the implementer's reasoning trace.

## Substance over trivials

Peer review only justifies its cost when it catches the issues that would actually have blocked closure had a senior owner looked at the diff. A review that surfaces only trivials (naming, comment density, formatting) while letting through obvious functional regressions — visible artifacts, wrong-axis metrics, structurally-blind closure evidence — is performative.

Brief contract for the reviewer:

1. The brief must include the *terminal-goal* check, not just diff hygiene. For a rendering CL: "did you `Read` the candidate frame; did the closure metric measure the actual quality axis or a proxy?" For a serialization CL: "did the round-trip test exercise the new code path?" — the reviewer's check matches the CL's terminal goal, not its diff shape.
2. If the change-class implies a layer-1 *Visual Read assessment* (per `visual-validation.md` §1), the reviewer must confirm the block exists in the closure record AND independently `Read` the candidate frame to corroborate or dispute the implementer's verdict.
3. A trivial-only review report is itself a finding. The dispatcher surfaces it to the user as a process miss, does not nod through, and escalates the next review on similar work to a cross-domain reviewer (`software-architect`) until the same-family reviewer demonstrates substance-catching on the class.

Recorded incident: phase-1 of TASK-77.1 closed past peer review with ring-like artifacts and a stddev-only AC; the reviewer either did not run the candidate visually or did not push back on the proxy metric. The user's framing: "the review does not make sense if it can't catch anything obvious but just trivials."

## Reviewer visual inspection (rendering CLs)

When the diff under review is a rendering CL — any change that affects rendered output (shaders, rendering passes, cache, denoise, composition, tone-map, post-process, lighting, materials) — and the commit body or implementer's closure record references capture paths under `Build/captures/`, the reviewer MUST independently `Read` the candidate captures and append a structured block to their review:

```
Visually inspected
- Captures opened: <list of absolute paths the reviewer Read this turn>
- Per-failure-mode checklist:
  - cell blockiness / hash-grid pattern: <none | observed at <where>>
  - rings / banding: <none | observed at <where>>
  - runaway brightness / NaN / clipping: <none | observed at <where>>
  - geometry holes / missing surfaces: <none | observed at <where>>
  - scene structurally unusable (e.g. black frame, wrong camera): <none | observed at <where>>
- Verdict on visual quality: improvement | regression | uncertain | per-scene-mixed
```

Implementer prose describing the captures ("no rings, no banding") is informational only — it cannot substitute for the reviewer's independent inspection. The reviewer is the only agent positioned to verify visual claims with a frame, and the structured block is the audit artifact that proves they did.

If the candidate captures cannot be opened (file missing, format error), the visual verdict is automatically `uncertain` and the review tier is **UNVERIFIED** per the table below — not PASS.

The block lands in the Implementation Notes review record AND in the commit message body via the `Reviewed-Visually:` footer (see "Commit-message line" below). Body content is the full block; footer is the audit-trail one-liner.

Recorded incident: the TASK-77.1 rework chain (six CLs `20b6dbdf` → `b9a103cc`) all passed peer review with no reviewer-visual-inspection block; the user opened the final capture and rejected it as visibly unacceptable (hash-grid cell blockiness in GISponza, GITestBox structurally broken from CL 1 onward). The reviewers verified structural and paper-port claims thoroughly but accepted the implementer's prose for the visual axis. This rule closes that gap.

### Cross-references

- `dispatcher/dispatch-briefs.md` § Stop-the-line — upstream pause when carry-forwards accumulate; this section is the downstream artifact catch.

## Verdict tiers

- **PASS** — diff meets brief and disciplines, with line-grounded evidence cited for every checked item. Not "no findings"; "checked all and they hold."
- **BLOCKED** — at least one finding the implementer must address. Each finding cites file:line and the discipline / invariant violated.
- **ADVISORY** — AC met, with non-blocking observations (style nudge, perf cleanup, follow-up seed). Ships with notes.
- **UNVERIFIED** — AC technically met but **not visually / runtime confirmed** because test infra cannot reach the code path. Does NOT ship until either (a) test infra is extended, OR (b) user explicitly acknowledges shipping unverified. UNVERIFIED is the only tier the dispatcher MUST surface to the user before commit; commit message records the path with a `Review-Unverified: <AC ref> — <gap> — <a|b>` line.

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

When the commit body references capture paths under `Build/captures/`, the reviewer's independent visual inspection (see "Reviewer visual inspection" above) is a separate audit artifact. The reviewer adds a one-line footer:

```
Reviewed-Visually: <reviewer-agent> — <verdict>
```

Verdict is one of `improvement`, `regression`, `uncertain`, `per-scene-mixed`. Multiple `Reviewed-Visually:` lines are valid (one reviewer per scene-set, or peer + architect both inspecting). When the reviewer's mandate is non-applicable for the CL — e.g. test-infra CLs that *produce* captures without claiming visual quality from them, or pure refactors with toggle-off bit-identical proof — record:

```
Review-Skipped-Visual: <reason>
```

The `visual-review` gate inside `commit-gate.js` enforces presence whenever the commit body references `Build/captures/` paths, with the same loud-failure shape as the peer-review gate.

## Cross-references

- `backlog-workflow.md` — task → dispatch → review → commit.
- `commit-message-policy.md` — owns the `Reviewed-By:` / `Review-Skipped:` line format.
- `paper-audit.md` — same issue-biased property.
- `visual-validation.md` — UNVERIFIED tier for visual ACs not exercisable by current infra.
- `agent-dispatch.md` — reviewer dispatches default to background.
