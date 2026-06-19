# InnocenceEngine

Single-user project (zhangdoa). No team, CI, or onboarding — closure notes and commit messages serve only future-zhangdoa.

## Build / run / test

| Step | Command |
|---|---|
| Configure (after adding files) | `cmake -B Build -S .` |
| Run engine | `Scripts/StartEngineWin.ps1 -Preset <name>` — never launch Main.exe / RenderTest.exe by path |
| Engine path-tracing test | `Scripts/TestPT.ps1` |
| Editor tests | `cd Source/Editor-Next && npx playwright test tests/<spec>.spec.js` (needs a live engine) |

## Continuity

Tasks and per-task history: `.backlog/tasks/` (Backlog.md). Durable decisions and invariants: basic-memory project `InnocenceEngine` (address explicitly; not the default).

## Pending cleanup (not enforced)

`.omp/agents/task-mgmt.md` and the impl agents were built for an abandoned auto-dispatch / session-start-briefing workflow. Prune when convenient.
