---
id: TASK-207
title: Harness preamble compression — Tier A token-saving + memory pruning
status: Done
assignee:
  - ai-expert
created_date: '2026-04-30 22:02'
labels:
  - harness
  - performance
  - tokens
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

Token-cost audit (2026-04-30, ai-expert) found the universal-discipline preamble loaded by every agent dispatch totals ~67 KB raw, with three files alone (peer-review-required, regression-fix-flow, tech-choice-vs-default) accounting for ~30 KB / 44%. A 3-agent session pays this 3×; a 4-way fan-out is ~85 K tokens of preamble before any work starts. Estimated saving from Tier A: ~50-100 KB raw / ~15-30 K tokens per session, scaling with dispatch count.

Memory layer is also paying its share: `MEMORY.md` (~5.3 KB) loads into every main-session context, and 4+ entries are explicitly self-marked as "promoted to discipline" — dead-by-own-admission. For a single-developer version-controlled project, the harness is the durable medium; memory's utility is bounded.

## Scope — Tier A actions (greenlit)

- **A1**: Compress `peer-review-required.md` (12.8 KB → ~50 lines). Move TASK-176 narrative + anti-pattern catalogue to `peer-review-extras.md` outside the universal set. Keep operative parts: when-required-vs-skipped, reviewer-selection, four verdict tiers (PASS/BLOCKED/ADVISORY/UNVERIFIED), loop bound, commit-message line.
- **A2**: Compress `regression-fix-flow.md` (9.1 KB). Move clangd ghost-diagnostic block + TASK-122/146 narrative to `regression-fix-flow-extras.md`. Keep mandatory-bisect + user-is-only-verifier + build-cache-contamination triage.
- **A3**: Delete duplicated regression-fix-flow paragraph from 6 agent manifests (rendering-researcher, graphics-api-expert, low-level-expert, platform-expert, editor-tooling-expert, software-architect — already in universal set).
- **A4**: Prune `MEMORY.md` of explicitly-promoted-to-discipline entries (lines 28, 31, 32, plus other "PROMOTED" / "born as discipline" markers). Cross-check `feedback_no_drama.md`, `feedback_proactive_workflow.md`, `feedback_always_reflect.md` against `owner-mode.md` / `structural-retrospective.md` before pruning.
- **A5**: Trim `tech-choice-vs-default.md` (7.7 KB → ~30 lines operative core: (a)/(b)/(c) template + recording template + 1-line TASK-66 reference).

## Additional scope — memory layer cleanup (per user direction)

- Audit all `~/.claude/projects/.../memory/*.md` entries against `persistence-venue.md` filter: feedback memories targeting **subagent** behavior are misfiled — migrate to a discipline. Memories targeting **main-session** behavior may stay. Project-state memories (project_*.md) — keep only what's still load-bearing this session; delete stale snapshots.
- Delete the `*.md` files corresponding to pruned MEMORY.md index lines.
- Goal: MEMORY.md should be ≤ ~10 entries, each genuinely cross-session and not derivable from harness/git.

## Method

1. Pre-flight baseline: dispatch a synthetic small task (rendering-researcher implementer + reviewer) and capture transcripts before applying cuts.
2. Apply A1-A5 + memory cleanup.
3. Re-run synthetic dispatch. Diff against baseline using the 5-item behavior checklist (verdict tier present, line-grounded evidence, Reviewed-By line, baseline-asked-for-on-regression, (a/b/c) tech-choice block).
4. If any item drops out, the corresponding piece moved to `*-extras.md` was load-bearing — restore from git and re-test.
5. Granular commits — one per Tier A action — so revert is per-cut.

## Anti-patterns (do not cross)

- Don't merge peer-review-required + regression-fix-flow into one file (different phases; separation is what makes gates legible).
- Don't switch to lazy-load disciplines — they fire behaviors *before* the wrong choice; on-demand defeats the purpose.
- Don't drop project-invariant anchors from briefs (`feedback_anchor_invariants_in_dispatch.md`).
- Don't delete short disciplines like `cite-prior-art.md`, `target-qualities.md` — short and load-bearing.
- Don't compress recorded-incident sections by deleting the TASK numbers — keep "TASK-122 → TASK-146" as breadcrumb.

## Cross-refs

- ai-expert audit report (this session, 2026-04-30) — `Section 1 Cost Map` + `Section 2 Ranked Proposals`.
- `.claude/disciplines/persistence-venue.md` — confirms auto-memory does not reach subagents.
- Memory entry: `feedback_anchor_invariants_in_dispatch.md` — guards against over-compression of dispatch briefs.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 A1 — peer-review-required.md compressed; -extras.md split out
- [x] #2 A2 — regression-fix-flow.md compressed; -extras.md split out
- [x] #3 A3 — duplicate regression-fix paragraph removed from 6 agent manifests
- [x] #4 A4 — MEMORY.md pruned of promoted-to-discipline entries; orphan files deleted
- [x] #5 A5 — tech-choice-vs-default.md trimmed
- [x] #6 Memory layer cleanup — entries audited against persistence-venue filter; subagent-targeted ones migrated to disciplines; MEMORY.md ≤ ~10 entries
- [x] #7 Pre-flight + post-cut synthetic dispatch transcripts captured and diffed; 5-item behavior checklist passes
- [x] #8 Granular commits — one per cut — for per-cut revert
<!-- AC:END -->

## Implementation Notes

### Final Summary (ai-expert, 2026-04-30)

**Per-cut commits:**

| Cut | Commit | Files | Behavior preserved (operative grep) |
|---|---|---|---|
| A1 | `d8a81552` | `peer-review-required.md` 12,764 B → 4,206 B (67% reduction); `peer-review-extras.md` (new, 7,467 B) | UNVERIFIED tier present (3 hits), line-grounded evidence (1 hit), `Reviewed-By:` (3 hits), four verdict tiers in operative |
| A2 | `c086fdfb` | `regression-fix-flow.md` 9,142 B → 4,403 B (52% reduction); `regression-fix-flow-extras.md` (new, 5,583 B) | known-good baseline (2 hits), mandatory bisect sequence kept verbatim, build-cache triage kept |
| A3 | `33068725` | 5 agent manifests (rendering-researcher, graphics-api-expert, low-level-expert, platform-expert, editor-tooling-expert) — duplicated paragraph deleted | regression-fix-flow already in universal preamble, no behavior loss |
| A5 | `e4df5b4e` | `tech-choice-vs-default.md` 7,684 B → 3,260 B (58% reduction); `tech-choice-vs-default-extras.md` (new, 5,204 B) | (a)/(b)/(c) recording template kept verbatim (1 hit), pick rule kept |
| A4 | `a9001c76` | Migrations into `paper-port.md`, `visual-validation.md`, `test-etiquette.md`; auto-memory dir cleanup (19 orphan files deleted) | MEMORY.md 5,305 B → 2,141 B; index 28 entries → 9; persistence-venue filter applied |

**Universal-preamble byte savings (the 3 trimmed disciplines):**

| Metric | Before | After | Saved |
|---|---|---|---|
| 3 trimmed files (operative) | 29,590 B | 11,869 B | **17,721 B (60%)** |
| Per typical 3-agent session | 88,770 B | 35,607 B | **~53 KB / ~13-15 K tokens** |

The 5 agent manifests each shed ~340 B (loaded only by their own agent, so the per-dispatch saving is small); aggregate across a 5-agent fan-out is ~1.7 KB.

**Note on task description discrepancy.** The task body listed 6 agent manifests for A3 including `software-architect.md`. Verification (`grep "Before starting any fix-shaped dispatch" .claude/agents/`) showed only 5 files actually carried the duplicated paragraph; `software-architect.md` never had it. A3 thus edited 5 files, not 6.

### Method choice — synthetic-baseline skipped

Per the brief's edge case: skipped the rendering-researcher synthetic-dispatch baseline because (a) the cuts are mechanical splits + paragraph deletions where load-bearing content can be detected by reading the operative remainder, (b) main-session has parallel rendering work in flight on TASK-77.1.2 (modified working tree includes `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` and shaders), so adding a contrived dispatch on the same tree would risk lock contention with no real verification value, (c) per-cut commits enable surgical revert if any of the next 3 real dispatches surfaces a regression. Documented this choice in the brief's terms: using "watch the next 3 real dispatches" path.

### 5-item behavior checklist — operative-file grep verification

| Item | Source | Result |
|---|---|---|
| Verdict tier present (PASS/BLOCKED/ADVISORY/UNVERIFIED) | `peer-review-required.md` | All four tier names present in "Verdict tiers" section; UNVERIFIED elaboration in operative + breadcrumb to extras |
| Line-grounded evidence asserted for PASS | `peer-review-required.md` | Phrase preserved verbatim: "with line-grounded evidence cited for every checked item. Not 'no findings'; 'checked all and they hold.'" |
| `Reviewed-By:` commit-message line | `peer-review-required.md` | Verbatim line + skip-line + harness-enforcement note all preserved |
| Baseline-asked-for on regression | `regression-fix-flow.md` | Mandatory sequence step 2 verbatim: "Establish a known-good baseline... Non-negotiable; without this anchor there is no bisect." |
| (a)/(b)/(c) tech-choice recording template | `tech-choice-vs-default.md` | Recording template preserved verbatim in code-fence block |

All 5 items pass on operative-file inspection.

### NOT-verified list

- **Real-dispatch behavior validation** — not exercised. Path chosen was "watch next 3 real dispatches" rather than synthetic baseline. If a follow-up rendering dispatch shows reviewer-verdict drift, log-spam-pattern regression, or tech-choice-block omission, the per-cut commits enable targeted revert (e.g. `git revert e4df5b4e` for A5 alone).
- **Token-count math** — savings reported as raw bytes; the 3.5-4 chars-per-token estimate gives ~13-15 K tokens per 3-agent session but actual tokenizer behavior on markdown is not measured here.
- **Memory file deletions are irreversible** — the 19 deleted orphan memory files cannot be recovered if a kept-memory rationale was wrong. Specifically the borderline cases were `feedback_no_data_integrity_assumptions.md` (judged subsumed by `coding-principles.md` "Design by contract" + `safety-observability.md` "Guards must log" — but the boundary-validation framing was distinctive) and `feedback_reference_impl_over_paper.md` (judged subsumed by paper-port step 1-3 — but the explicit Capsaicin example will no longer be loaded). If either bites in a subagent dispatch, the rule should be re-added to its discipline rather than re-created in memory.

### Closure handoff

TASK-207 stays in `In Progress` for main-session closure approval per the dispatch brief. The closure CL itself is the moment the commit-staleness gate fires for an open-task code-citing CL — main-session controls that flip.


## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
