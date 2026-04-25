---
id: TASK-129
title: 'Producer agent: gain Agent/Task tool for chain-dispatch to other agents'
status: To Do
assignee: []
created_date: '2026-04-25 11:58'
labels:
  - harness
  - agent-config
  - producer
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Gap surfaced 2026-04-25** during a session where the user asked the producer (this agent) to dispatch `ai-expert` in background mode for a tiered-model evaluation.

The producer's tool-set does not currently include `Agent`/`Task`. The `Agent` tool surfaces in main-session Claude (the dispatcher), but not in sub-agents — so the producer cannot chain-dispatch to other specialised agents on its own. To run an `ai-expert` background job, the user had to bounce back to main session.

This is a real workflow gap: the producer's job description in `CLAUDE.md` includes "cross-agent dispatch plans," and the natural way to enact a dispatch plan is to actually dispatch. Today the producer can only *write* the brief and hand it back to main session for execution.

**Possible fixes (evaluate before implementing)**:

1. Grant the producer agent access to the `Agent` tool in its manifest (`.claude/agents/producer.md`). Lowest-friction option. Risk: producer can now spin off arbitrary sub-agents — need to bound this with explicit rules in the manifest (e.g. "producer only chain-dispatches when the user explicitly requests a sub-agent run; otherwise hand the brief back").

2. Keep producer toolless for dispatch, but formalise the hand-back pattern: producer always returns dispatch briefs as structured output, main session always executes. Cleaner separation; more user latency (extra round-trip).

3. Hybrid: producer can chain-dispatch in **background mode only** (fire-and-forget), but synchronous `Agent` calls require main-session execution. Matches the principle that producer orchestrates async work but doesn't block on sub-agent results.

**Action**: ai-expert decides which option fits the harness-discipline philosophy and implements. Cross-reference to the in-flight dispatch-discipline work ai-expert is already running (the discipline being encoded there should cover whether sub-agents can themselves dispatch).

**Originating session**: 2026-04-25, ecs-overhaul branch, model-tiering evaluation thread. The brief that surfaced this gap is the tiered-model evaluation itself, which will likely live as a backlog task or Implementation Note once `ai-expert` completes.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Decision recorded for which of the three options (or a fourth) is adopted
- [ ] #2 If option 1 or 3: `.claude/agents/producer.md` updated with explicit chain-dispatch rules
- [ ] #3 If option 2: producer manifest documents the hand-back pattern as the standard
- [ ] #4 Cross-referenced with the dispatch-discipline work currently in flight under ai-expert
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
