# Discipline: test-etiquette

Tests cost the user. Each engine launch is ~500MB Main.exe + asset I/O; each editor launch is ~150MB Electron + Vite + IPC; each Playwright run spins up both. zhangdoa is on the same machine as the harness — concurrent test runs are visible to him as foreground process load on the box he's actively using.

## Why

This discipline is not about correctness — every gate-driven test run is correct. It's about **resource etiquette**: don't spawn what you can avoid; clean up what you spawn; serialize what you must run. A repro-test-fix-test-test loop that leaks 5 editor instances slows zhangdoa's machine while he's trying to think.

## How

### Plan launches before opening tools

Before any tool call that launches the editor, the engine, a Playwright spec, or a long-running script, write the launch budget into the plan:

- *"I will launch the editor at most N times: 1 to reproduce, 1 to verify the fix, 1 final."*
- *"I will run the engine for K frames once, capture, and exit."*

If the work doesn't have a bounded launch count, the plan is wrong — fix the plan before tooling.

### Read-first, run-last

For repro / debugging / "look at this":

1. Read the relevant code (Vue, HLSL, C++, IPC handlers) and form a hypothesis from static reading.
2. Only launch when the hypothesis genuinely needs runtime confirmation.
3. One launch per hypothesis cycle, not per tweak.

The pattern to avoid: launch → tweak → launch → tweak → launch. Each launch is a 30-second startup + asset load + Vite bundle. Five iterations is a coffee break of pure wait, plus orphan-process accumulation if the previous run's Electron didn't clean up.

### Serialize Playwright workers

When Playwright is needed, default to `npx playwright test --workers=1`. Default parallelism (cores ÷ 2) spawns N concurrent Electron + Main.exe sets — a 4-worker run on an 8-core machine is 4× the resource hit of serial. Serial runs longer in wall-clock but caps the in-flight resource cost.

Override only when the spec is genuinely independent and serial is too slow to matter — and document that override in the brief.

### Trap orphan cleanup between iterations

Playwright + Electron in particular leak when a spec aborts mid-run, when the engine takes longer than the test timeout, or when an assertion throws and the teardown hook doesn't fire. Between launches:

```
taskkill.exe //F //IM electron.exe   # kills all Electron instances
taskkill.exe //F //IM Main.exe       # kills any orphan engine
```

Run this between iterations if the agent is doing more than one launch. Run it before the first launch if the worktree-state suggests a previous session left orphans (the `tasklist | grep electron` snapshot at session start is fast).

### Don't dispatch repro work in parallel with other launchers

If two agents both launch the editor or the engine, the user sees combined load — and the per-agent launch budget compounds. Sequence editor / engine work, even when the file-level subtrees are orthogonal.

This is a stronger constraint than the general orthogonality rule: orthogonal subtrees are usually fine for parallel dispatch, but if both touch the launcher surface, serialize.

### The dispatcher's responsibility

When dispatching an agent that touches editor / engine / Playwright:

1. **State the launch budget in the brief.** Not "reproduce + diagnose + fix + test" (open-ended) but "reproduce once, diagnose from reading, fix, verify in one run, then commit."
2. **Require orphan cleanup.** "Use `taskkill` between launches if any Electron / Main.exe instance survives."
3. **Don't co-dispatch with another launcher-agent.** If a rendering agent is running engine smoke tests, defer the editor agent's run-time work until the rendering agent finishes.

If you can't write the launch count in advance, the brief is too open and will leak processes. Tighten before dispatching.

## Anti-patterns

- **"It's just one more launch."** The leak is cumulative. The 5th orphan Electron is invisible-to-the-agent but visible-to-the-user.
- **"The agent will clean up its own processes."** They don't, by default. Playwright timeouts, mid-test exceptions, and aborted runs all bypass teardown.
- **"Parallel = faster."** Only if the user has spare cores. Co-dispatching launcher-agents on a single-developer machine (which this repo is) is net-negative for user attention, even when wall-clock is shorter.
- **"Memory will catch this next time."** It won't. Memory files decay; this is in `disciplines/` because every agent reads it before acting.

## Cross-references

- `agent-dispatch.md` — both disciplines protect user attention; agent-dispatch governs dispatcher-surface occupancy, this one governs editor / engine launch budgets.
- `perf-measurement-frame-budget.md` — paired discipline for engine launches that *do* happen: pick N from the perf bucket so each launch is cheap.
- `regression-fix-flow.md` — bisect steps require many user-verified launches; this discipline's "make each step cheap" applies to bisect steps too.
