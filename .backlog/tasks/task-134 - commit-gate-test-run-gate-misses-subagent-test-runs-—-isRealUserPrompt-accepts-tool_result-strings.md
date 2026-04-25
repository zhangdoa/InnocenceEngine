---
id: TASK-134
title: >-
  commit-gate test-run gate misses subagent test runs — isRealUserPrompt accepts
  tool_result strings
status: To Do
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

When a subagent (e.g. `editor-tooling-expert`) runs `npx playwright test ...` multiple times in a turn and then attempts `git commit`, the commit-gate's `test-run` gate blocks with "no integration test run in this turn" — even though the subagent transcript clearly contains 30+ matching `tool_use` entries.

Reproduced 2026-04-25 during TASK-132 closure. Subagent transcript `b5ae016f-a550-4e9f-ac6d-1786ae9da21f/subagents/agent-a2613621d6c7bc3cd.jsonl` had 23 Bash `tool_use` blocks including multiple `cd ... && npx playwright test editor.spec.js` invocations. Gate still emitted the block message.

## Root cause

`.claude/hooks/lib/common.js::isRealUserPrompt` filters synthetic prompts by checking string-prefix sentinels (`<system-reminder>`, `<task-notification>`, etc.). It accepts ANY user-role message whose content is a non-string OR a string that doesn't start with a known sentinel.

But subagent transcripts (and likely main transcripts under some serialization paths) store **tool_result blocks as user messages with `content` as a plain string** — e.g. `{"type":"user","message":{"role":"user","content":"  1 passed (4.8s)"}}`. These slip past the filter and count as "real user prompts", so `lastUserIdx` advances to the *latest tool_result*, which is **after** the actual test runs. The test-run scan window (lastUserIdx+1 .. end) is then empty.

## Fix sketch

`isRealUserPrompt` should additionally reject user messages that look like tool_results. Two complementary checks:

1. If the parent message has `tool_use_id` or any block has `type: tool_result`, treat as non-prompt (already handled when content is array — the `typeof content !== 'string'` check rejects).
2. When `content` is a string, look at the wrapping message envelope: a real user prompt has `parentUuid: null` (start of turn) or follows an assistant message with no pending tool_use; a tool_result-as-string follows an assistant tool_use turn. Simpler heuristic: examine the parent message's role/type — if the prior assistant message ended with a `tool_use`, the next user message is a tool_result regardless of content shape.

Cleanest fix: in the dispatcher's lastUserIdx loop, additionally require that the user message at index `i` is NOT immediately following an assistant `tool_use` block. Or — track tool_use_id presence and skip those entries. Or — only accept user messages whose content array contains a `text` block (not a `tool_result` block); for string-content, only accept if the prior message in the transcript is NOT an assistant turn ending in tool_use.

## Impact

Gate is overly conservative for subagents — every test-validated commit from a subagent now requires `[skip-test-gate]` despite legitimate test runs. This trains agents to use the escape hatch, which defeats the gate's purpose.

## Workaround

Use `[skip-test-gate]` and document the test runs in the commit body. TASK-132's commit hit this and is currently blocked.

## Scope

Cross-cutting harness fix; ai-expert ownership (`.claude/hooks/lib/common.js`). Single-file change. After fix, re-attempt TASK-132 commit without sentinel to confirm.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 isRealUserPrompt (or equivalent guard in commit-gate.js) rejects user messages whose content is a tool_result, including the string-content serialization variant seen in subagent transcripts
- [ ] #2 Reproducer: dispatch a subagent that runs `npx playwright test <spec>` and attempts `git commit` of an editor change — commit succeeds without `[skip-test-gate]`
- [ ] #3 TASK-132's commit (currently blocked) is re-attempted without the sentinel and the gate passes
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
