---
id: TASK-134
title: >-
  commit-gate test-run gate misses subagent test runs — isRealUserPrompt accepts
  tool_result strings
status: Done
assignee: []
created_date: '2026-04-25 18:03'
labels:
  - harness
  - commit-gate
  - infrastructure
dependencies: []
references:
  - .claude/hooks/lib/common.js
  - .claude/hooks/gates/test-run.js
  - .claude/hooks/commit-gate.js
  - TASK-132
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

When a subagent (e.g. `editor-tooling-expert`) runs `npx playwright test ...` multiple times in a turn and then attempts `git commit`, the commit-gate's `test-run` gate blocks with "no integration test run in this turn" — even though the subagent transcript clearly contains many matching `tool_use` entries.

Reproduced 2026-04-25 during TASK-132 closure. Subagent transcript `b5ae016f-a550-4e9f-ac6d-1786ae9da21f/subagents/agent-a2613621d6c7bc3cd.jsonl` had 8 `npx playwright test` Bash tool_use blocks before the commit attempt at row 102 / 123. Gate still emitted the block message both times.

## Root cause (revised 2026-04-25)

The original spec blamed `isRealUserPrompt` accepting string-form `tool_result` blocks — that diagnosis was wrong. Verification:

1. `isRealUserPrompt` already correctly rejects non-string content (`typeof content !== 'string'` returns false).
2. The subagent transcript stores every `tool_result` as a list-typed user message (`content: [{type: "tool_result", ...}]`), which the existing predicate already rejects.
3. Simulating the gate against the subagent JSONL alone shows `lastUserIdx = 0` and 8 qualifying `npx playwright test` hits in the post-prompt window — the gate would PASS.

**Actual mechanism:** when a sub-agent's `Bash {git commit}` triggers PreToolUse, Claude Code passes the **parent session's** transcript path as `transcript_path`, not the sub-agent's own JSONL. The transcript-dependent gates (test-run, live-engine, serialize-test) then scan a transcript that contains none of the sub-agent's tool_use blocks, no closure intent, and no test evidence. Sub-agent JSONLs live at `<projects>/<sessionId>/subagents/agent-<agentId>.jsonl` and are invisible to a parent-only scan.

Confirmation: at the moment of the sub-agent's commit attempt (2026-04-25T17:57:33Z), the parent transcript had 57 rows; last real user prompt was index 26 ("all of them"); zero qualifying test runs in the parent's post-prompt window. Meanwhile the sub-agent's own JSONL contained 8 qualifying runs.

Three options were considered:
- (A) Walk all sidechain transcripts and union their tool_use blocks into the parent's scan window.
- (B) When parent dispatches an `Agent`, eagerly union the corresponding `subagents/agent-<id>.jsonl` blocks into the parent scan.
- (C) Reframe semantics: a sub-agent's commit should scan its OWN transcript only — fix the harness's transcript resolution to pick the sidechain JSONL when the call originates from sidechain.

## Fix landed (Option C with Option A discovery)

`.claude/hooks/lib/common.js::resolveActiveTranscriptPath(xpFromHook, currentCmd)`:
1. Derives the sub-agents directory from the parent transcript path (`<dirname(xp)>/<sessionId>/subagents`).
2. Walks every `agent-*.jsonl` and finds the one whose **last assistant Bash tool_use** matches the in-flight `currentCmd` exactly. At PreToolUse the assistant turn IS persisted but no following tool has run — so the last tool_use is the in-flight one.
3. Unique match → swap that JSONL into the gate's transcript scan. Zero/multiple matches → fall back to the parent (preserves existing behavior).

`.claude/hooks/commit-gate.js` calls the helper before reading the transcript. All three transcript-dependent gates (test-run, live-engine, serialize-test) benefit transparently — the fix is in the dispatcher's transcript loader, not in any single gate.

Why Option C with A's discovery: the harness can't ask Claude Code to pass a different `transcript_path`, but it CAN identify the active sidechain JSONL and swap it in itself. Each sub-agent's commit then scans the transcript that actually contains its work, so each sub-agent must show evidence in its own transcript — the gate is not weakened. Chained dispatches (sub-sub-agent commits) work transparently because `subagents/` is a flat directory under the root sessionId.

## Verification

Simulation against the TASK-132 transcript pair:
- Parent transcript path → swap to sub-agent JSONL `agent-a2613621d6c7bc3cd.jsonl`.
- Sub-agent transcript: `lastUserIdx = 0`, qualifying-test hits in window = 8.
- `test-run.run(ctx).ok = true` — gate would have passed.

Negative cases verified: (a) parent-only command → no swap; (b) bad xp → returns input unchanged; (c) empty cmd → no swap; (d) two sub-agents matching same command → falls back to parent (ambiguous).

## Note on AC #3

The spec called for re-attempting TASK-132's commit without `[skip-test-gate]`. That commit (`c00eb7c5`) does NOT contain the sentinel — it landed via a different path after the sub-agent's two attempts were blocked. So AC #3 is moot in its original form; the verification path above (simulation against the same transcript pair the sub-agent hit) is the equivalent evidence.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 (revised) Sub-agent `git commit` PreToolUse resolves the active sidechain transcript and the transcript-dependent gates (test-run, live-engine, serialize-test) scan the sub-agent's own JSONL when it's the call's origin. Falls back to the parent transcript when no unique sidechain match is found.
- [x] #2 Reproducer simulation: against the TASK-132 transcript pair, `resolveActiveTranscriptPath` swaps the parent JSONL for `agent-a2613621d6c7bc3cd.jsonl`; running `test-run.run(ctx)` on the resolved transcript returns `{ok: true}` (8 qualifying playwright runs found in the post-prompt window).
- [x] #3 (moot) TASK-132's landed commit `c00eb7c5` does NOT contain `[skip-test-gate]`; equivalent evidence is the simulation in #2 against the same transcript pair the sub-agent hit live.
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — Node parses both files; `node -e "require('./.claude/hooks/commit-gate.js')"` and the in-process gate simulation both load without error.
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — the harness has no JS unit suite for the gates; equivalent evidence is the live simulation against the TASK-132 transcript pair (4 cases: positive swap, parent-only command no-swap, bad xp passthrough, ambiguous-match falls back).
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — the in-process simulation against real recorded transcripts (`b5ae016f-...jsonl` parent + `agent-a2613621d6c7bc3cd.jsonl` sub) is the integration check; no mocks involved.
- [x] #4 Self-authored mock-based tests are not the sole validation — N/A; validation runs against recorded production transcripts.
- [x] #5 User-observable outcome verified — simulation truncates the sub-agent JSONL to its PreToolUse-time state (rows 0..102) and the resolver picks it; test-run gate then returns `{ok: true}`.
- [x] #6 Final summary lists what was NOT verified — NOT verified live: a fresh sub-agent dispatch that runs a qualifying test then commits, end-to-end through the actual hook (would require dispatching a sub-agent for an unrelated change just to test). The simulation reproduces the exact PreToolUse moment from the recorded TASK-132 incident; reasoning that holds at simulation time holds live because the only state difference is the future-state appended after the commit attempt, which the resolver doesn't read.
<!-- DOD:END -->

## Implementation Notes
<!-- IMPL:BEGIN -->
**Landed in this CL.** Files touched:
- `.claude/hooks/lib/common.js` — added `resolveActiveTranscriptPath(xpFromHook, currentCmd)` and exported it. The function derives the sidechain directory from the parent transcript path, scans every `agent-*.jsonl` for the one whose last assistant Bash `tool_use` matches the in-flight command, and returns that path on a unique match (otherwise returns the input).
- `.claude/hooks/commit-gate.js` — phase-2 transcript loader now passes `input.transcript_path` through `resolveActiveTranscriptPath` before `fs.existsSync`/parse. Comment block at the swap site explains why and references this task.

**Design choices and trade-offs:**
- **Why match by command rather than agentId:** the hook input shape (`tool_name`, `tool_input`, `transcript_path`, `cwd`) doesn't include the calling agent's ID. The in-flight Bash command IS in `tool_input.command` — that's the unique fingerprint we have at PreToolUse.
- **Why "last assistant Bash tool_use" not "any":** at PreToolUse, the assistant turn containing the in-flight tool_use is persisted but no following tool has run — so it IS the last one in the live JSONL. For historical simulation we have to truncate, but the live behavior is direct.
- **Why fall back to the parent on ambiguity:** two sub-agents ending on the exact same command is not a contract we can rely on to disambiguate. Falling back preserves the current (broken-for-sidechain) behavior rather than picking the wrong transcript.
- **Why not augment instead of swap:** unioning windows would let a sub-agent's evidence bleed across to a parent's commit (or vice versa) once both transcripts are scanned. Each commit should be backed by evidence in its own scope. Swap preserves "evidence in caller's transcript" semantics.

**Holds for chained dispatches.** Sub-sub-agent JSONLs live in the same flat `subagents/` dir under the root sessionId, so the resolver finds them with no extra logic.

**Not added:**
- No `lastUserIdx` heuristic change. The original spec proposed gutting `isRealUserPrompt`; verification showed the predicate is fine — the bug is wrong-transcript, not wrong-window-within-transcript.
- No Claude Code platform request. Even if Claude Code starts passing the sidechain JSONL as `transcript_path`, this resolver becomes a no-op (sidechain JSONL → derived sub-dir doesn't contain it → returns input unchanged).
<!-- IMPL:END -->
