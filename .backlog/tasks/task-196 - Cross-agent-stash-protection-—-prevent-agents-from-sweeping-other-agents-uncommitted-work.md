---
id: TASK-196
title: >-
  Cross-agent stash protection — prevent agents from sweeping other agents'
  uncommitted work
status: To Do
assignee:
  - ai-expert
created_date: '2026-04-28 19:49'
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

## Acceptance criteria

- [ ] A hook (gate) blocks `git stash` when dirty files cross subtree ownership
- [ ] Block message tells the caller exactly which paths they own and which they don't
- [ ] `[stash-cross-subtree-OK]` sentinel allows opt-out (rare; document in discipline)
- [ ] `.claude/disciplines/owner-mode.md` references the gate
- [ ] Tests: cross-agent dirty → block; same-agent dirty → allow; sentinel → allow

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
