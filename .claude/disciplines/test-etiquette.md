# Discipline: test-etiquette

Tests cost the user. Engine launches are ~500MB Main.exe + asset I/O; editor launches are ~150MB Electron + Vite + IPC. zhangdoa is on the same machine — concurrent runs show up as foreground load while he is thinking. Plan launches before opening tools.

## Rules

- **Launch budget in plan.** Before any tool call that launches the editor, engine, Playwright, or a long-running script, write the budget: *"editor launches at most N times: 1 to reproduce, 1 to verify, 1 final."* No bounded count → plan is wrong, fix it before tooling.
- **Read first, run last.** Form a hypothesis from static reading; launch only when runtime confirmation is genuinely needed. One launch per hypothesis cycle, not per tweak.
- **Serialize Playwright** — default to `npx playwright test --workers=1`. Override only when the spec is genuinely independent and serial is too slow to matter; document the override in the brief.
- **Time one iteration before scaling to N.** Never kick off an N-iteration loop without first timing one and confirming clean exit. Wrap long runs in `timeout N`. If multiple invocations hang at the same point, suspect environment corruption (zombies, driver, AV) and ask the user to restart rather than burning more cycles.
- **Sweep orphans between iterations.** `taskkill.exe //F //IM electron.exe` and `taskkill.exe //F //IM Main.exe` — Playwright + Electron leak when specs abort, timeouts hit, or assertions throw before teardown.
- **Don't co-dispatch launcher-agents.** Sequence editor / engine work even when subtrees are orthogonal — combined load lands on the user.

## Cross-references

- `agent-dispatch.md` — both protect user attention; agent-dispatch governs dispatcher-surface occupancy, this one governs editor / engine launch budgets.
- `perf-measurement-frame-budget.md` — paired for engine launches that *do* happen: pick N from the perf bucket so each launch is cheap.
- `regression-fix-flow.md` — bisect steps require user-verified launches; "make each step cheap" applies to bisect steps too.
