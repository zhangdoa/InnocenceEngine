---
id: TASK-201
title: >-
  Backlog-status drift audit — find existing tasks where code landed but status
  not flipped (TASK-193 retroactive companion)
status: In Progress
assignee: []
created_date: '2026-04-29 07:19'
updated_date: '2026-04-29 08:19'
labels:
  - backlog-hygiene
  - post-mortem
  - retroactive
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

TASK-193 (closure-staleness commit-gate) catches the gap going forward. But existing tasks in `To Do` / `In Progress` may have already been worked-and-merged without a status flip — same gap, just retroactive.

Concrete precedent from this session: TASK-184's fix landed in commit `bfe66510` (`fix(editor): TASK-184 — color edit propagates instead of being overwritten by K-mode`) but the task file's status field was never flipped. The drift was discovered only because TASK-193's gate fired on a downstream commit that referenced TASK-184 in the message. The retrofit-flip happened in commit `124f0763` — weeks after the actual work landed.

If this happened to TASK-184, it almost certainly happened to others. The closure notes / Final Summary fields are also likely stale.

## Goal

Audit every existing `To Do` / `In Progress` task against git log. For each:
- If git log shows code-bearing commits referencing the task → flip status to Done with retroactive Final Summary citing the commit SHA(s).
- If no commits reference the task → leave alone (task is genuinely open).

## Approach

Producer-driven audit (this is exactly producer's session-state-reconciliation scope). For each open task:
1. Grep git log for `TASK-N` reference in commit subject/body.
2. If found, classify: code commit (likely closure), docs(backlog) commit (already counted), discussion-only mention.
3. For genuine code closures with no flip: write retrofit-flip with a closure note citing the commit. Pattern matches TASK-184's retrofit.

Output: a docs(backlog) commit (or a sequence) flipping all retroactive-Done tasks. Each flip annotated with "Status flip retrofitted YYYY-MM-DD; work landed in `<sha>`."

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Audit run against all `To Do` / `In Progress` tasks
- [x] #2 Each found drift annotated with the closure commit SHA in the task's Final Summary
- [x] #3 Status flipped where appropriate
- [ ] #4 Audit script (or recipe) saved somewhere repeatable so the audit can be re-run periodically
- [ ] #5 Producer-brief incorporates the same check on session start (if affordable) — companion to TASK-193's commit-time enforcement, catching anything that slipped through before the gate landed

## Owner

`producer` (backlog reconciliation is producer scope).

## References

- TASK-193 (closure-staleness commit-gate — landed `66327f34`)
- TASK-184 retrofit precedent — flipped in `124f0763`
- `.claude/disciplines/backlog-workflow.md` § Cross-session continuity (§ Harness enforcement — closure-staleness gate)
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

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
## 2026-04-29 audit run

Producer-driven sweep over every `To Do` / `In Progress` task at the time of the run (62 open tasks). For each task, grepped `git log --all` body+subject for `TASK-N` references and classified each match as code-closure / docs-only mention / discussion-only.

### Retrofit-flipped (3)

- **TASK-21** — `ff28c0be feat(logic): runtime-tweaks ImGui panel for player tunables (TASK-21)` + follow-up `e7fcb5c7 fix(logic): silence per-tick TweakRegistry save log (TASK-21 follow-up)`. Player Settings panel + TweakRegistry header shipped exactly the task scope.
- **TASK-96** — `69efaa70 build(branding): embed engine logo as Win32 .exe icon (Main, RenderTest)`. Both engine executables now carry `Data/Engine/Icons/icon.ico` via shared WinMain.rc.
- **TASK-101** — `bf07d562 test(editor): assert GET/UPDATE entity-property symmetry (TASK-101)`. Live-engine Playwright spec round-trips every component.<prop> for GISponza; 290 OK / 8 READ_ONLY / 0 drift. Structural option-2 (shared property table) deferred per the spec's findings.

### Verified-not-retrofit (representative subset)

- **TASK-23** (foundation-class audit) — only the FixedSizeString item landed in `f8b02160`-era commits; Array / ObjectPool / ThreadSafeQueue / Memory::Reallocate items still pending. Genuinely In Progress.
- **TASK-72** (EntityRegistry component-ref invalidation) — design-only commit `fd769ef8`; Implementation Notes explicitly state "Implementation deferred." Genuinely To Do.
- **TASK-71** (Player m_SmoothInterp/m_IsTP runtime toggle) — dependency on TASK-21 satisfied, but the actual #ifdef removal + ImGui toggles haven't shipped. Genuinely To Do.
- **TASK-129 / TASK-130 / TASK-160** — awaiting decisions / sign-off / batch-DIFF ergonomics work; no code landed.

All other open tasks either had no code-bearing commit referencing them, or matched only docs(backlog) discussion mentions (filing follow-ups, cross-references, blocked-on notes).

### Deferred to follow-up dispatches (AC #4 + #5)

- **AC #4** (audit script / recipe saved somewhere repeatable) — the recipe used here is documented inline in TASK-201's Description and in this Implementation Note. Codifying it as a script under `Scripts/` or as a producer-skill discipline fragment is harness work — dispatch to `ai-expert`.
- **AC #5** (producer-brief incorporates the same check on session start) — same owner. Adds friction to every session start; needs a perf budget evaluation before wiring. Dispatch to `ai-expert` with cross-reference to `.claude/disciplines/session-start.md`.

The current audit's output — the retrofit-flip docs(backlog) commit on `ecs-overhaul` — is the deliverable for AC #1–#3. Closing TASK-201 fully waits on the harness side.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**AC #4 and AC #5 BLOCKED** as of 2026-04-28 on TASK-203 — sub-agent ai-expert dispatches cannot Write/Edit `.claude/**` paths (two attempts: `adfe8c2fc7ff4c182` and `a7fc71385058829ad` both hit the same permission wall). Main-session writes work fine, and an earlier ai-expert dispatch in this same session successfully wrote `.claude/` files (`66327f34` closure-staleness gate). The wall is dispatch-scoped — likely a sub-agent permission-inheritance gap. Filed as TASK-203 (high priority, ai-expert).

Workaround until TASK-203 lands: producer's manual audit recipe (this commit's methodology) provides operational coverage. The 24-candidate prototype output verified the design soundness; codification is mechanical once the perm wall is removed.

Design reference for the next attempt:
- Script: `.claude/hooks/lib/audit-backlog-drift.js` (~190-210 lines). Single `git log --all --grep=TASK- --format=%H%x09%s%x09%b`; per-task hash-set lookup; strong-signal (subject) vs weak-signal (body) distinction; modes `--quiet`, `--json`. Public `audit()` API.
- Discipline: `.claude/disciplines/backlog-drift-audit.md`. Codifies classification recipe (code-closure / cross-reference / multi-CL / explicit-deferred). Cite TASK-201 + TASK-72 as canonical examples.
- Wire: `.claude/disciplines/session-start.md` step 4 — drift check (perf budget 194ms measured << 5s ceiling, fits comfortably).
<!-- SECTION:FINAL_SUMMARY:END -->
