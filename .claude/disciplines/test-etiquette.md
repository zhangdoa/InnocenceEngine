# Discipline: test-etiquette

Tests cost the user. Engine launches are ~500MB Main.exe + asset I/O; editor launches are ~150MB Electron + Vite + IPC. zhangdoa is on the same machine — concurrent runs show up as foreground load while he is thinking. Plan launches before opening tools.

## Rules

- **Launch budget in plan.** Before any tool call that launches the editor, engine, Playwright, or a long-running script, write the budget: *"editor launches at most N times: 1 to reproduce, 1 to verify, 1 final."* No bounded count → plan is wrong, fix it before tooling.
- **Read first, run last.** Form a hypothesis from static reading; launch only when runtime confirmation is genuinely needed. One launch per hypothesis cycle, not per tweak.
- **Serialize Playwright, and invoke through `npm test`.** For editor E2E specs (`Source/Editor-Next/tests/`), always run via `npm test -- --workers=1 <spec>`, never `npx playwright test` directly. `Source/Editor-Next/package.json` has a `pretest: vite build` hook that rebuilds `dist/` before the spec runs; it fires only on `npm test` / `npm run test`, so direct `npx playwright test` invocations silently run against a stale bundle if editor source changed since the last manual build. The `--` separator is required so npm forwards args to the Playwright script. Keep `--workers=1` as the default — override only when the spec is genuinely independent and serial is too slow to matter; document the override in the brief.

  ```
  # Wrong (bypasses pretest hook — stale dist/):
  npx playwright test --workers=1 render-toggles.spec.js

  # Right (pretest fires, dist/ rebuilds):
  npm test -- --workers=1 render-toggles.spec.js
  ```

  Incident anchor: TASK-211 step 3 (commit `50c36a69`); fix-up commit `d680f1f7` added the `pretest` hook, this rule is the second half — change *where* specs are invoked from, not just *what* happens before invocation.
- **Time one iteration before scaling to N.** Never kick off an N-iteration loop without first timing one and confirming clean exit. Wrap long runs in `timeout N`. If multiple invocations hang at the same point, suspect environment corruption (zombies, driver, AV) and ask the user to restart rather than burning more cycles.
- **Sweep orphans between iterations.** `taskkill.exe //F //IM electron.exe` and `taskkill.exe //F //IM Main.exe` — Playwright + Electron leak when specs abort, timeouts hit, or assertions throw before teardown.
- **Don't co-dispatch launcher-agents.** Sequence editor / engine work even when subtrees are orthogonal — combined load lands on the user.

## Cross-references

- `agent-dispatch.md` — both protect user attention; agent-dispatch governs dispatcher-surface occupancy, this one governs editor / engine launch budgets.
- `perf-measurement-frame-budget.md` — paired for engine launches that *do* happen: pick N from the perf bucket so each launch is cheap.
- `regression-fix-flow.md` — bisect steps require user-verified launches; "make each step cheap" applies to bisect steps too.
