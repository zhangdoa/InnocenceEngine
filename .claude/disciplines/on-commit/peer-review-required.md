# Discipline: peer-review-required

Every non-trivial implementation dispatch is followed by a peer-review dispatch before commit.

## Skip categories

- **Backlog / docs-only** (`.backlog/`, `Documents/`, `.md`, `.claude/`, `.alignments/`, `.gitignore`). Closing a task → review still required.
- **Hook fixes / harness self-edits** where the diff is the gate logic the reviewer would consult. Surface which gate in the commit message.
- **Mechanical refactors with a single deterministic transformation** (rename, file move, generated-data sweep). Articulate why there's no design surface.

Unsure → required.

## Reviewer selection

Always a fresh agent dispatch — never main-session, never the implementer.

1. **Same impl stage, fresh dispatch** (default). A `code-impl` review of a `code-impl` diff. Different fresh context catches motivated reasoning while keeping domain depth.
2. **Cross-stage review** when the CL legitimately spans two impl stages (e.g. C++ + HLSL paired refactor) — dispatch the matching second stage as the reviewer, or `task-mgmt` if neither fits.
3. **`superpowers:code-reviewer`** as a last-resort fallback.

Brief contains: diff, original implementation brief, relevant disciplines, anchored invariants. **Not** the implementer's reasoning trace.

## Review checks terminal goal, not diff hygiene

Rendering CL → "did you `Read` the candidate frame; did the closure metric measure the actual quality axis or a proxy?" Serialization CL → "did the round-trip test exercise the new code path?"

Trivial-only review reports (naming, formatting, comment density only) are themselves a finding → escalate the next review on similar work to a fresh dispatch of a different impl stage, or to `task-mgmt` for cross-cutting framing.

## Reviewer visual inspection (rendering CLs)

When the diff affects rendered output AND the closure record references `Build/captures/` paths → the reviewer independently `Read`s the captures and writes the structured per-failure-mode block defined in `../on-implement/visual-validation.md` § Layer 1. Implementer prose ("no rings") cannot substitute. Captures unopenable → verdict `uncertain` → tier UNVERIFIED.

## Verdict tiers

| Tier | Meaning | After |
|---|---|---|
| PASS | Diff meets brief + disciplines, line-grounded evidence | Implementer commits |
| ADVISORY | AC met, non-blocking observations | Ships with notes |
| BLOCKED | At least one finding citing file:line + discipline | Implementer addresses → re-dispatch |
| UNVERIFIED | AC technically met but not visually / runtime confirmed (infra cannot reach) | Surface to user; commit only after user picks (a) extend infra or (b) acknowledge unverified. Record `Review-Unverified: <AC ref> — <gap> — <a\|b>` |

Loop bound: max 2 review iterations. Still BLOCKED → surface both reviews + implementer response to the user.

## Findings flow

Reviewer writes `## Review (<reviewer-stage>, <date>)` block in the owning task's Implementation Notes. Verdict in header; findings itemised file:line + discipline citations.

## Commit-message footers

```
Reviewed-By: <reviewer-stage>
```

Multiple lines valid. Skips:

```
Review-Skipped: <reason — backlog-only / hook-internal / mechanical-rename / etc>
```

When body references `Build/captures/`:

```
Reviewed-Visually: <reviewer-stage> — <improvement | regression | uncertain | per-scene-mixed>
```

Visual mandate doesn't apply:

```
Review-Skipped-Visual: <reason>
```

Enforced by `gates/peer-review.js` and `gates/visual-review.js`.

## Cross-references

- `../dispatcher/dispatch-briefs.md` § Stop-the-line.
- `../always/backlog-workflow.md`, `commit-message-policy.md`, `../on-implement/paper-audit.md`, `../on-implement/visual-validation.md`, `../on-dispatch/agent-dispatch.md`.
