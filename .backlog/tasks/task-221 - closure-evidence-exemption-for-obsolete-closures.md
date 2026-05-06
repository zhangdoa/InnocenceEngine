---
id: TASK-221
title: 'closure-evidence: exemption for obsolete closures'
status: Done
assignee:
  - '@claude'
created_date: '2026-05-06 20:14'
updated_date: '2026-05-06 21:00'
labels:
  - harness
  - tech-debt
dependencies: []
references:
  - .claude/hooks/gates/test-run.js
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
<!-- SECTION:DESCRIPTION:BEGIN -->
The closure-evidence path of `gates/test-run.js` requires a qualifying integration test for any task flipping to `status: Done`, with no exemption for genuinely-obsolete closures. Hit during TASK-189 closure (commit `d749b157`): the task was a non-reproducible observation never validated and never re-triggered, but landing the closure required running `Main.exe -total_frames 1` purely to satisfy the gate.

Fix shape: recognise an explicit closure-reason marker (e.g. `closure_reason: obsolete` in frontmatter, or a `Closure-Reason: obsolete` commit footer) and bypass the test-run requirement when present. Keep the gate strict for normal "work landed" closures.
<!-- SECTION:DESCRIPTION:END -->
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — N/A (harness-only change; no engine build). Hook tests run via `node .claude/hooks/tests/commit-gate.test.js`.
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — `commit-gate.test.js`: 64 passed, 0 failed (was 63; +1 from the r5b empty-marker-with-footers regression case added per peer-review).
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — N/A; the new path is exercised by direct gate-function calls, which is the established pattern for this gate's tests.
- [x] #4 Self-authored mock-based tests are not the sole validation — flagged: validation IS solely the standalone gate-runner tests in `commit-gate.test.js` plus a one-shot manual regex check (`node -e ...`). No live commit-gate end-to-end run was performed; the next real commit (this CL itself) exercises the full dispatcher path.
- [x] #5 User-observable outcome verified — terminal transcript of `node .claude/hooks/tests/commit-gate.test.js` (`64 passed, 0 failed`) and `node -e` regex probe confirming `Closure-Reason:\n\nReviewed-By:` no longer satisfies the tightened pattern.
- [x] #6 Final summary lists what was NOT verified — see Final Summary § Not verified.
<!-- DOD:END -->

## Implementation Notes

Single dispatch + advisory follow-up. Design settled at brief: commit-footer marker (`Closure-Reason: <value>`) consistent with the existing `Reviewed-By:` / `Review-Skipped:` family; presence-based exemption that re-applies the docs-only bypass on closing-CL paths only; code-bearing CLs still require a qualifying test. Frontmatter variant explicitly declined as out-of-scope.

Initial implementation landed regex `/^Closure-Reason:\s*\S/m` and seven test assertions covering all six cases from the design.

## Review (harness-impl, 2026-05-06) — ADVISORY

**Verdict**: ADVISORY. Gate semantics correct; one regex edge case the original test r5 claimed to cover but did not.

**Finding**: `CLOSURE_REASON_RE` used `\s*\S` — and `\s` includes `\n`, so `Closure-Reason:\n\nReviewed-By: ai` matched the regex despite the empty value. Test r5 only covered the EOF-terminated empty case (no `\S` anywhere after); in any real CL with attribution / review footers the loose marker would silently bypass. Fix: tighten to `[ \t]*\S` (same-line value) and split r5 into r5a (EOF) + r5b (followed by other footers).

**Confirmed correct**: boolean logic in `run()` (`test-run.js:36-42`); `ctx.messageText || ''` undefined-handling; block-message append (4 lines, action-first); discipline/code consistency between `backlog-workflow.md`, `commit-message-policy.md`, and the gate; tests are deterministic / single-process / zero-dep; out-of-scope creep zero (only the four files in brief plus the task frontmatter touched).

**Out-of-scope structural observation** (from both the implementer and reviewer): `ATTRIBUTION_RE` in `lib/common.js` shares the `\s*\S` shape and the same vulnerability; multiple other footer regexes (`Reviewed-By:` / `Reviewed-Visually:` / `Review-Skipped*` / `Code-Human-Written:` etc.) likely share it too. A single audit-and-fix pass over `lib/common.js` and the per-gate regexes would resolve the family together; piecemeal is worse. Not filing as a follow-up task per `backlog-workflow.md` § "Don't pile on backlog tasks" — no incident has proven the family-wide bug costly. Surface here as a known structural observation; file if/when it bites.

## Follow-up dispatch — ADVISORY addressed

Tightened regex to `/^Closure-Reason:[ \t]*\S/m`; updated comment so it doesn't over-claim. Test r5 split: r5a covers `Closure-Reason:\n` (EOF), r5b covers `Closure-Reason:\n\nReviewed-By: ai\nCode-AI-Generated-By: Claude\n` — both block. Test count 63 → 64.

## Final Summary

**Files changed (5)**:
- `.claude/hooks/gates/test-run.js` — `CLOSURE_REASON_RE` + exemption branch + block-message exemption paragraph + leading comment update.
- `.claude/hooks/tests/commit-gate.test.js` — `test-run gate — Closure-Reason: exemption` group; 8 assertions across 7 cases (r1, r2, r3, r4, r5a, r5b, r6).
- `.claude/disciplines/always/backlog-workflow.md` — exemption paragraph under § Closure-evidence is main-session-only.
- `.claude/disciplines/on-commit/commit-message-policy.md` — `Closure-reason` row added to the footer table.
- `.backlog/tasks/task-221 - closure-evidence-exemption-for-obsolete-closures.md` — this file (status, assignee, Implementation Notes, DoD ticks, Final Summary).

**Verified**:
- `node .claude/hooks/tests/commit-gate.test.js` → `64 passed, 0 failed`.
- Manual regex probe: `Closure-Reason:` empty value followed by `Reviewed-By:` no longer matches.
- The TASK-189-shaped scenario (docs-only closure with `Closure-Reason:` footer) bypasses the gate; same scenario without the footer still blocks.

**Not verified**:
- The full commit-gate dispatcher end-to-end on a `git commit` invocation — the next real commit (this CL) exercises it, but no synthetic dispatcher fixture was run beforehand.
- `ATTRIBUTION_RE` and other footer regex shapes — out of scope; not filed as a follow-up task (see Implementation Notes § Out-of-scope structural observation).
- No live engine build was run (intentional — harness-only diff).
