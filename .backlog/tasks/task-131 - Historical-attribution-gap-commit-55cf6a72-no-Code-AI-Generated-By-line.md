---
id: TASK-131
title: >-
  Historical attribution gap: commit 55cf6a72 missing Code-AI-Generated-By line
status: Done
assignee: []
created_date: '2026-04-25 14:30'
updated_date: '2026-04-25 14:30'
labels:
  - harness
  - historical-artefact
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Commit `55cf6a72` (CL1 of TASK-125) landed without an AI-authorship attribution line, despite `.claude/disciplines/commit-message-policy.md` declaring attribution mandatory and `.claude/hooks/commit-gate.js` claiming to enforce it.

This task is a historical artefact record, not an actionable bug. The gate has been hardened (see Implementation Notes); the affected commit is left in place because rewriting public history is off-limits without explicit user direction.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] Root cause identified and recorded
- [x] Gate fix landed so the bypass cannot recur
- [x] Discipline doc updated with the new ordering invariant
- [x] Affected historical commit recorded; no retroactive amend
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Root cause** — `.claude/hooks/commit-gate.js` (pre-fix) fetched the session transcript before evaluating any gate; on a missing or unreadable transcript, `failOpen` exited 0 and the entire gate chain was skipped. Attribution is a pure-text check that has no reason to depend on transcript availability, but it was wired as the final entry in a single gate list that ran only after the transcript loaded successfully. Any session where `transcript_path` was absent / pointed at a not-yet-flushed file / contained an unparseable line caused all six gates — including attribution — to be bypassed.

The previous regex-based attribution check (`ATTRIBUTION_RE` in `.claude/hooks/lib/common.js`) was correct; the structural defect was in the dispatcher. The "first failure wins" comment was accurate but only described the happy path; the fail-open path silently allowed the commit.

**Fix** — `.claude/hooks/commit-gate.js` now partitions gates by a per-gate `needsTranscript` flag. Transcript-independent gates (`file-size`, `paper-port`, `attribution`) run BEFORE the transcript fetch; transcript-dependent gates (`test-run`, `live-engine`, `serialize-test`) run after. A transcript I/O failure now fails open for phase 2 only — attribution still blocks.

Reproduction (kept in commit message of the gate-fix CL): synthetic commit message identical to `55cf6a72` + a transcript path pointing at a non-existent file. Pre-fix exit 0 (commit allowed). Post-fix exit 2 (attribution-missing block).

**Affected commit** — `55cf6a72 fix(gi): port Capsaicin's color-delta + dynamic history cap to GIDenoise [CL1 of TASK-125] [skip-size-gate]`. No retroactive amend; this task is the canonical record that the gap exists. If a future audit asks "is the missing attribution on `55cf6a72` known?", the answer is "yes, see TASK-131".

**Side observations from this work**:
- The `[skip-size-gate]` and `[skip-test-gate]` sentinels are scoped correctly to their respective gates (no cross-gate leak). That hypothesis was checked and rejected.
- `failOpen` writes to stderr but nothing in the harness collects those lines for retrospective audit, so a session in which the gate failed open never surfaces the fact to the user. Possible follow-up: route hook fail-open events through a more visible channel (statusline, session-end summary). Not filed yet — single observation, not a recurring drift.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
The commit-gate dispatcher's fail-open envelope coupled attribution (a pure-text invariant) to transcript availability (an I/O dependency). When transcript I/O failed, the entire gate chain was skipped and `55cf6a72` landed without attribution. Fix: split gates into transcript-independent (phase 1) and transcript-dependent (phase 2) phases; attribution sits in phase 1 so it cannot be bypassed by transcript failure. Discipline doc updated with the ordering invariant. The historical commit is left in place per the standing no-rewrite-history rule.
<!-- SECTION:FINAL_SUMMARY:END -->
