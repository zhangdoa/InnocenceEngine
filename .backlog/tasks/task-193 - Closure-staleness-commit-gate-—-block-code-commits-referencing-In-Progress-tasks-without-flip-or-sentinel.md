---
id: TASK-193
title: >-
  Closure-staleness commit-gate — block code commits referencing In-Progress
  tasks without flip or sentinel
status: Done
assignee:
  - ai-expert
created_date: '2026-04-28 19:36'
updated_date: '2026-04-28 19:58'
labels:
  - harness
  - commit-gate
  - discipline-enforcement
dependencies: []
references:
  - .claude/hooks/gates/test-run.js
  - .claude/hooks/lib/common.js
  - .claude/disciplines/backlog-workflow.md
  - .claude/hooks/commit-gate.js
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Motivation

Producer's TASK-175-chain closure pass (2026-04-28) surfaced a systemic gap: code-bearing commits routinely reference TASK-N (e.g. `feat(rendering): TASK-176 ...`) without any closure flip on the referenced task. Concrete evidence from the chain:

| Commit | TASK referenced | Status flipped? | Final Summary written? |
|---|---|---|---|
| `033b4520` | TASK-176 | No | No |
| `d2b2e9fe` | TASK-176 | No (stayed `In Progress`) | No |
| `f41a4ffb` | TASK-177 | No (stayed `To Do`) | No |
| `c478a833` | TASK-177 | No (stayed `To Do`) | No |

The discipline already exists in `.claude/disciplines/backlog-workflow.md` § "Cross-session continuity" — it's just not enforced. User's framing: "we are bad at closing issues."

## Goal

Add a deterministic commit-gate that catches the failure at the moment it happens (mirroring the existing `closure-evidence` gate's posture, but on the upstream side).

## Scope

Add `.claude/hooks/gates/closure-staleness.js` to the commit-gate dispatcher (`.claude/hooks/commit-gate.js`).

Behavior:
1. Parse `TASK-\d+` references from the staged commit message.
2. For each referenced TASK-N: read the task file's `status:` frontmatter (use existing helpers in `.claude/hooks/lib/common.js`).
3. **Block** the commit when ALL of the following hold:
   - Status is `In Progress` or `To Do`
   - Staged diff contains source / shader / config files (i.e. NOT a `docs(backlog)` flip itself)
   - No `[task-stays-open]` sentinel is in the commit message
4. Block message: *"Commit references TASK-N which is still `In Progress`/`To Do`. Either flip TASK-N to `Done` in this CL (preferred), file a `docs(backlog)` follow-up flip, or add `[task-stays-open]` to the commit message if the work is genuinely partial."*
5. Fail-open on internal errors (consistent with other gates).

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `.claude/hooks/gates/closure-staleness.js` exists and is wired into `commit-gate.js`
- [x] #2 Gate fires on a code-bearing commit referencing an `In Progress` task → blocks
- [x] #3 Gate respects `[task-stays-open]` sentinel → allows
- [x] #4 Gate skips for `docs(backlog)` flip commits → allows
- [x] #5 Gate skips when no `TASK-\d+` reference exists → allows
- [x] #6 Existing commit-gate tests continue to pass; new test covers the new gate
- [x] #7 Update `.claude/disciplines/backlog-workflow.md` to reference the gate (so the rule and the enforcement are co-located in docs)

## References

- Template gate: `.claude/hooks/gates/test-run.js` (`closure-evidence` rule — symmetric case)
- Helpers: `.claude/hooks/lib/common.js` (commit-message parsing, file-set inspection)
- Existing rule (un-enforced): `.claude/disciplines/backlog-workflow.md` § "Cross-session continuity"
- Commit-gate dispatcher: `.claude/hooks/commit-gate.js`
- Producer's diagnosis (2026-04-28): closure-discipline-gap report

## Deferred

- **Option B** (producer-brief reconciliation step) — defer; once Option A lands, B's role is the "in-session catch-net" and the gain is incremental.
- **Option C** (implementer-flips-status-before-peer-review) — defer; soft-discipline, file as separate task once Option A lands.
<!-- AC:END -->

## Notes for the implementer (ai-expert)

- This is hook-internal work — falls in your `Review-Skipped: hook-internal` category per peer-review-required.md, BUT given the systemic nature of this fix and that it changes commit semantics, consider PASS-required peer review by another ai-expert peer or by software-architect. Use judgment.
- Test the gate end-to-end: stage a fake code commit referencing an `In Progress` TASK file, attempt commit, verify block; then with sentinel, verify allow.
- The `closure-evidence` gate (test-run.js) is the inverse-direction template; study it for posture and structure.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
### 2026-04-28 — implementation landed (ai-expert), pre-review

**Files**:
- `.claude/hooks/gates/closure-staleness.js` (new, 150 lines) — the gate.
- `.claude/hooks/commit-gate.js` (modified) — wired into phase-1 gate list between `paper-port` and `peer-review`.
- `.claude/hooks/tests/commit-gate.test.js` (modified) — added 3 test groups, 19 new assertions covering `parseTaskRefs`, `findTaskFile`, and all six `run()` scenarios.
- `.claude/disciplines/backlog-workflow.md` (modified) — added "Harness enforcement — closure-staleness gate" sub-section under "Cross-session continuity" (per AC #7).

**Design choices**:
- **Lookup by ID, not exact filename.** Globs `.backlog/tasks/` for `task-<id> - *.md` (case-insensitive on the prefix to absorb MCP rename idiosyncrasies). The dash-space separator is the disambiguator (`task-9 ` ≠ `task-90 ` ≠ `task-900 `).
- **Staged-content-first read** (option (b) in the brief). When the task file is in the staged set, the gate reads its content via `git show :<path>` so a same-CL `+status: Done` flip is honored. When not staged, it reads on-disk. This makes the gate self-consistent — TASK-193's own closure CL passes through itself.
- **Docs-only bypass** mirrors `test-run.js` posture: when every staged file matches `DOCS_ONLY_PATH` the gate passes through (so `docs(backlog)` flip CLs ship). Closure-evidence (in test-run.js) handles the inverse — closure claim without test evidence — so the two gates compose.
- **Sentinel: `[task-stays-open]`** as audit trail for genuinely partial work.
- **Fail-open** on any read error (missing task file, unreadable frontmatter) — same posture as every other gate.

**Validation**:
- Unit tests: `node .claude/hooks/tests/commit-gate.test.js` → **46 passed, 0 failed** (27 pre-existing + 19 new).
- E2E against the live commit-gate dispatcher (scratch harness in `Build/`, deleted): all 5 scenarios pass —
  1. Code commit + `In Progress` TASK ref → exit 2 (block) with full block message.
  2. Same commit + `[task-stays-open]` → exit 0.
  3. `docs(backlog)` flip CL (only task file staged) → exit 0.
  4. Code commit with no `TASK-N` reference → exit 0.
  5. Same-CL flip (code + `status: Done` task file both staged) → exit 0 (option-b validated end-to-end against real `git show :path`).

**Pending**:
- Peer review by `software-architect` (per the brief's directive that systemic-impact judgment overrides the manifest's `Review-Skipped: hook-internal` category).
- Closure flip + commit happen post-review.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Closure-staleness commit-gate landed

**Implementation** (5 files, all in `.claude/`):
- `hooks/gates/closure-staleness.js` (new, ~150 lines) — the gate. Phase-1, transcript-independent, fail-open. Parses `TASK-\d+` refs from staged commit message; reads each task's effective `status:` (staged-content first via `git show :path`, on-disk fallback); blocks when status is `In Progress`/`To Do`, staged set is non-docs-only, and no `[task-stays-open]` sentinel.
- `hooks/commit-gate.js` (modified) — wired the gate into the phase-1 list between `paper-port` and `peer-review`.
- `hooks/tests/commit-gate.test.js` (modified) — 19 new assertions covering AC-matrix scenarios + parseTaskRefs edges + findTaskFile ID-disambiguation + Done/To Do/mixed/unknown-ID paths.
- `disciplines/backlog-workflow.md` (modified) — new "Harness enforcement — closure-staleness gate" sub-section under § "Cross-session continuity" (rule + enforcement co-located).

**Self-consistency**: gate's own closure CL flips TASK-193 to `Done` in the same commit; staged-first read sees `status: Done` and allows the commit through. No sentinel needed.

**Validation**:
- `node .claude/hooks/tests/commit-gate.test.js` → 46 passed, 0 failed (27 pre-existing + 19 new).
- 5 E2E scenarios against live commit-gate dispatcher with tmp git repo + fixture: block on stale ref, allow on `[task-stays-open]`, allow on `docs(backlog)` flip, allow on no-`TASK-N`, allow on same-CL flip. All exit codes match expectations.
- Peer review: software-architect — **PASS** with 3 non-blocking advisories (minor doc nit on header comment ordering, 1-line comment opportunity for `closingTasks` field intentionally unused, noted edge of empty-staged-set).

**Commit**: this CL.

**Out of scope (deferred)**:
- Option B (producer-brief reconciliation step) — defer; once Option A lands, B is the in-session catch-net with incremental gain.
- Option C (implementer-flips-status-before-peer-review) — defer; soft-discipline.
<!-- SECTION:FINAL_SUMMARY:END -->
