---
id: TASK-196
title: >-
  Cross-agent stash protection — prevent agents from sweeping other agents'
  uncommitted work
status: Done
assignee:
  - ai-expert
created_date: '2026-04-28 19:49'
updated_date: '2026-04-30 18:59'
labels:
  - harness
  - agent-dispatch
  - orthogonality
  - post-mortem
dependencies: []
references:
  - .claude/hooks/session-gate.js
  - .claude/hooks/lib/common.js
  - .claude/disciplines/owner-mode.md
  - .claude/agents/
  - Stash incident 2026-04-28
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Incident (2026-04-28)

Three agents ran in parallel (rendering-researcher on TASK-183, editor-tooling-expert on TASK-188, ai-expert on TASK-193). Their owned subtrees are disjoint.

The editor-tooling-expert hit a build blocker and ran `git stash push -m "TASK-188 build-blocker stash"`. Because `git stash` operates on the entire worktree (not just the editor subtree), it swept up the rendering-researcher's in-progress TASK-183 source edits — 7 files across `Source/Engine/Common`, `Source/Engine/Services/PerFrameDataService*`, `Source/Shaders/HLSL/**`, `Source/ExampleProject/RenderingClient/`.

Cascade:
1. Rendering agent's next file read returned pre-edit content; they thought a "linter was reverting their edits" and returned blocked.
2. Their edits were re-applied multiple times before the stash, producing two stashes (the second a strict subset of the first).
3. Recovery required `git stash pop stash@{0}` to restore the rendering work.

The `stash@{0}` label said "TASK-188 build-blocker" but the contents were 100% rendering files — a misleading audit trail.

## Goal

Make `git stash` (and any other worktree-disruptive operation) safe under parallel-agent dispatch.

## Possible approaches (pick during design)

**A — Hook-based block.** Add a session-gate or new pre-bash gate that intercepts `git stash` (and `git checkout -- <path>`, `git reset --hard`, `git restore`) calls and:
- Reads `git status` to find dirty files
- Cross-references each dirty file against agent ownership (per agent manifests + subtree CLAUDE.md)
- Blocks the operation if any dirty file is owned by an agent OTHER than the caller
- Block message instructs: use `git stash push -- <owned-paths>` instead

**B — Discipline-only.** Tighten `.claude/disciplines/owner-mode.md` and `agent-dispatch.md` to forbid worktree-wide stash; mandate `git stash push -- <paths>` with explicit subtree paths. No enforcement; relies on agent reading.

**C — Worktree per agent.** Use `git worktree add` per parallel dispatch so each agent has an isolated checkout. Heavy — coordination cost is high.

## Recommendation

A is the leverage-equivalent of TASK-193's closure-staleness gate: deterministic, fail-stop at the moment of failure. Combine with a small B-style discipline note. Defer C unless cross-agent collision becomes a recurring issue.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 A hook (gate) blocks `git stash` when dirty files cross subtree ownership
- [x] #2 Block message tells the caller exactly which paths they own and which they don't
- [x] #3 `[stash-cross-subtree-OK]` sentinel allows opt-out (rare; document in discipline)
- [x] #4 `.claude/disciplines/owner-mode.md` references the gate
- [x] #5 Tests: cross-agent dirty → block; same-agent dirty → allow; sentinel → allow
<!-- AC:END -->

## Notes

- Owner-determination requires reading agent manifests + subtree CLAUDE.md `Owned by` markers. Helper for this likely already exists or can be added to `.claude/hooks/lib/common.js`.
- Same gate posture (and same fail-open-on-error policy) as the existing commit-gate rules.
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
## Implementation (ai-expert, 2026-04-30)

**Layer chosen.** PreToolUse session-gate sub-gate (`.claude/hooks/gates/cross-subtree-stash.js`), wired through `.claude/hooks/session-gate.js`. The collision happens at the Bash-tool boundary mid-work, not at commit time and not at agent-dispatch time — session-gate is the right layer. Approach (A) from the brief; (B) added as the discipline note in `owner-mode.md`; (C) deferred per brief's recommendation.

**Detection rule.** Collision shape, not caller identity. Enumerate dirty files via `git status --porcelain -z`, resolve each to its owning agent, block if the owner-set has cardinality > 1. Knowing *which* sub-agent issued the Bash call would require parent-transcript correlation; knowing whether a worktree-wide stash *would cause* a collision is sufficient and simpler. The block message names the owners and paths so the calling agent immediately sees what they would sweep.

**Files.**
- `.claude/hooks/lib/ownership.js` (new, 134 LOC) — `OWNERSHIP_RULES` (mirror of subtree CLAUDE.md `Owned by` markers), `resolveOwner`, `getDirtyFiles`, `parseGitStashCommand`. Split out instead of extending `lib/common.js` (which would have crossed the 400-line file-size gate — ai-expert manifest says split-before-grow on hook files).
- `.claude/hooks/gates/cross-subtree-stash.js` (new, 111 LOC) — the gate itself.
- `.claude/hooks/session-gate.js` — third entry in `GATES`, comment header updated.
- `.claude/disciplines/owner-mode.md` — "Cross-subtree stash protection" section + sentinel docs (AC #4).
- `.claude/hooks/tests/cross-subtree-stash.test.js` (new, 258 LOC) — 52 tests, all PASS.

**Subcommand discrimination.** Gates `git stash` / `git stash push` / `git stash save` (worktree-wide unless `-- <pathspec>` is present). Does NOT gate `git stash pop / list / show / drop / clear / branch / apply / create / store` — these don't sweep dirty work. Path-scoped `git stash push -- <paths>` is also allowed (caller already scoped).

**Out of scope (deliberate).** `git checkout -- <path>`, `git reset --hard`, `git restore .` were named in the brief's "Goal" section but the AC explicitly scopes to `git stash`. Different command shapes have different semantics (checkout-with-pathspec is fine when path is owned; reset --hard is always worktree-wide and destructive). Documented in the gate's docstring as intentional deferral so a follow-up agent knows where to extend.

**Sentinel.** `[stash-cross-subtree-OK]` anywhere in the bash command (typically as a trailing `# [stash-cross-subtree-OK]` comment). Discipline note documents the "rare; not a convenience" framing.

**Validation.** 
- Unit: `node .claude/hooks/tests/cross-subtree-stash.test.js` — 52/52 PASS (parseGitStashCommand discrimination, resolveOwner per-subtree, gate.run end-to-end with real temp git repos for cross-agent block, same-agent allow, sentinel allow, pop allow, path-scoped push allow, clean-tree allow, non-Bash ignore).
- Existing tests still green: `node .claude/hooks/tests/commit-gate.test.js` — 46/46 PASS.
- Dispatcher smoke: `echo '{...}' | node .claude/hooks/session-gate.js` exits 0 on a benign tool call.
- Incident replay: synthesised the 2026-04-28 shape (7 dirty files across rendering / engine-common / editor / unowned subtrees, command `git stash push -m "TASK-188 build-blocker stash"`) — gate trips, block message lists all four buckets and the offending paths.

**Peer review.** `Review-Skipped: hook-internal` per `peer-review-required.md` § "When" — pure harness self-edit; the diff IS the gate logic a reviewer would consult. Documented exception in the discipline.

## Drift NOTED (not filed per don't-pile-on)

- `Source/Engine/Services/PerFrameDataService.{cpp,h}` (and likely siblings) live in `Source/Engine/Services/` but aren't covered by the OWNERSHIP_RULES — they fall through to `null` (treated as `<unowned>` by the gate, conservative). The CLAUDE.md tree doesn't currently declare an owner for `Source/Engine/Services/` itself; only the DX12/VK/Asset/Scene sub-clusters are claimed. A follow-up should clarify ownership of the rest of `Services/` (likely `software-architect` for the architecture-level services, `low-level-expert` for plumbing-level) and the table extended to match. NOT filed as a separate task; surface here for the next sequential session.
<!-- SECTION:NOTES:END -->
