# Discipline: test-etiquette

Engine launches are ~500MB Main.exe + asset I/O. Editor launches are ~150MB Electron + Vite + IPC. Concurrent runs land as foreground load on the user. Plan launches before opening tools.

## Rules

- **Launch budget in plan.** Before any tool call that launches the editor, engine, Playwright, or a long-running script: write the budget. *"Editor launches at most N times: 1 to reproduce, 1 to verify, 1 final."* No bounded count → fix the plan.
- **Read first, run last.** Form a hypothesis from static reading; launch only when runtime confirmation is needed. One launch per hypothesis cycle, not per tweak.
- **Serialize Playwright; invoke through `npm test`.** For editor E2E specs (`Source/Editor-Next/tests/`): always `npm test -- --workers=1 <spec>`, never `npx playwright test` directly. `Source/Editor-Next/package.json` has a `pretest: vite build` hook that fires only on `npm test` / `npm run test`. Direct `npx playwright test` runs against a stale bundle. The `--` separator is required so npm forwards args to the Playwright script. `--workers=1` default — override only when serial is genuinely too slow, document the override in the brief.

  ```
  # Wrong (bypasses pretest hook — stale dist/):
  npx playwright test --workers=1 render-toggles.spec.js

  # Right (pretest fires, dist/ rebuilds):
  npm test -- --workers=1 render-toggles.spec.js
  ```

- **Time one iteration before scaling to N.** Never kick off an N-iteration loop without first timing one and confirming clean exit. Wrap long runs in `timeout N`. If multiple invocations hang at the same point → suspect environment corruption (zombies, driver, AV); ask the user to restart.
- **Sweep orphans between iterations.** `taskkill.exe //F //IM electron.exe` and `taskkill.exe //F //IM Main.exe`.
- **Don't co-dispatch launcher-agents.** Sequence editor / engine work even when subtrees are orthogonal.

## Cross-references

- `on-dispatch/agent-dispatch.md` — protects user attention via dispatcher-surface occupancy; this one via launch budgets.
- `on-bug/perf-measurement-frame-budget.md` — paired for engine launches; pick N from the perf bucket.
- `on-bug/regression-fix-flow.md` — bisect steps require user-verified launches.
