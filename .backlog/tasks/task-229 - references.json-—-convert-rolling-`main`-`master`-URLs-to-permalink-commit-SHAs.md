---
id: TASK-229
title: >-
  references.json — convert rolling `main`/`master` URLs to permalink commit
  SHAs
status: To Do
assignee: []
created_date: '2026-05-16 21:08'
labels:
  - infra
  - documentation
  - harness
dependencies: []
references:
  - .alignments/TASK-226.4-port-audit.md
  - .claude/references.json
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-226.4 peer review (commit e0e68900). All 14 existing entries in `.claude/references.json` cite rolling-pointer URLs (`https://github.com/.../blob/main/...` or `.../blob/master/...`), which silently drift as the upstream repo moves. When the audit's line numbers go stale (as already happened for TASK-226.4's `:845-857` / `:920` references vs the actual `:407-428` / `:487-496` in the local snapshot), the commit-gate's citation-evidence enforcement can fire false-negatives or pass-throughs that aren't reproducible.

Convert each entry's `reference` field to a permalink commit SHA (`/blob/<sha>/...`) anchored to the local `.alignments/_audit_refs/` snapshot that was used to write the entry. For entries whose local snapshot path is also recorded, cross-reference both.

Project-wide cleanup, not blocking any specific task. Low priority — defer until a paper-port task touches an entry with a known stale-line-number issue.

Touched file: `.claude/references.json` (14 entries).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 All `reference` URLs in `.claude/references.json` use permalink commit SHAs, not `main`/`master`
- [ ] #2 Each entry's permalink matches the local snapshot under `.alignments/_audit_refs/` (where recorded)
- [ ] #3 Commit-gate citation-evidence rules still pass on a representative paper-port file (run a smoke commit on a no-op edit)
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
