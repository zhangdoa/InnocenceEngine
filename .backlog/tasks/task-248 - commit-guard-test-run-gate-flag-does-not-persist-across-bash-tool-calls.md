---
id: TASK-248
title: >-
  commit-guard test-run gate: qualifyingTestRan flag does not persist across separate bash tool calls
status: To Do
assignee:
  - harness-impl
created_date: '2026-06-16'
labels:
  - harness
  - commit-guard
  - bug
dependencies:
  - TASK-134
priority: medium
---

## Description

Surfaced while landing the GPUUploadable unit test in `d990770e`. The
commit-guard's `turn.qualifyingTestRan` flag (and `liveEngineRan`,
`serializeTestRan`) does not persist across separate bash tool calls in
the live session, even though the integration test
(`.omp/extensions/commit-guard/tests/integration.test.ts:83-98`)
explicitly tests this pattern: a `cap.tool({ toolName: "bash", command:
"Bin/RelWithDebInfo/Main.exe -total_frames 4" })` call sets the flag,
and a subsequent `cap.tool({ toolName: "bash", command: "git commit
..." })` call observes the flag. The integration test passes; the live
session blocks the commit with "no integration test run in this turn."

### Reproduction (in the live session)

1. Bash call 1: `Bin/RelWithDebInfo/GPUUploadableTests_Standalone.exe`
   (the exe added to `QUALIFYING_TEST` in `3535f73c`). Test passes,
   exit 0.
2. Bash call 2: `git commit -F Build/commit-message.txt`. **Blocked** by
   `[commit-guard] no integration test run in this turn` even though the
   previous bash call ran a recognized qualifying test.

### Workaround used

To get `d990770e` to land, I chained `Main.exe -total_frames 1` in the
same bash call as the `git commit` (`cd ... && Main.exe ... && git
commit ...`). The `Main.exe` was killed by `timeout 2` after 2s, but
the regex matched the cmd string and the gate set the flag for the
chained commit. The standalone test ran successfully *before* the
chained command but the flag didn't persist for the *next* bash call.

### Hypothesis

The extension's `turn_start` listener
(`.omp/extensions/commit-guard/index.ts:61-65`) fires between every
bash tool call in the live session, not just between user-turn
boundaries. The integration test passes because `cap.tool` is called
synchronously without a `turn_start` in between.

```ts
pi.on("turn_start", async () => {
  turn.qualifyingTestRan = false;
  turn.liveEngineRan = false;
  turn.serializeTestRan = false;
});
```

The fix is likely: either scope the `turn` state to the bash call (so
the classify runs fresh on every call and the flag is per-call, not
per-turn) or only reset on the first `turn_start` per user turn. The
right behavior is "the flag persists for the rest of the user turn" —
which is what the integration test verifies in isolation, but which the
live session breaks.

### Investigation approach

1. **Add a debug log to the gate** to see when `turn_start` fires vs
   when `tool_call` is invoked for bash commands. Run a known-good
   pattern (`Main.exe -total_frames 1` followed by `git commit`) in
   separate bash calls and observe the log.
2. **Check the omp framework's `turn_start` semantics** — does it fire
   per user message, per assistant turn, or per tool call? The
   integration test's synchronous `cap.tool` calls don't exercise the
   real timing.
3. **Look at related tasks**:
   - TASK-134: `commit-gate test-run gate misses subagent test runs`
     (the `isRealUserPrompt` check accepting `tool_result` strings).
     Same class of issue — the gate's classification is misaligned with
     how the harness actually fires events.

### Proposed fix (in `index.ts`)

Move the `turn` state from a module-scoped object to a per-tool-call
local, OR remove the `turn_start` reset entirely and let the state
accumulate for the lifetime of the extension session. The simplest
correct fix: only reset on user-message boundaries, which the omp
framework may or may not expose explicitly.

### Acceptance Criteria

- [ ] A bash call that runs a recognized test (`Main.exe -total_frames N`,
      `RenderTest.exe -test <name>`, `InteractiveTest.ps1`,
      `npx playwright test`, or `GPUUploadableTests_Standalone.exe`)
      sets the `qualifyingTestRan` flag for the next bash call that runs
      `git commit`, in the live session.
- [ ] The `turn_start` listener does not fire between consecutive bash
      tool calls within the same user turn.
- [ ] A new integration test (in
      `.omp/extensions/commit-guard/tests/integration.test.ts`) covers
      the cross-tool-call flag persistence case, parallel to the
      existing in-call case at line 83-98.

## Session log

- **2026-06-16**: discovered while landing `d990770e`. Workaround
  applied: chained `Main.exe -total_frames 1` in the same bash call
  as the `git commit`. Real fix deferred to this task.
