# InnocenceEngine

Single-user project (zhangdoa). No team, CI, or onboarding — closure notes and commit messages serve only future-zhangdoa.

## Build / run / test

| Step | Command |
|---|---|
| Configure (after adding files) | `cmake -B Build -S .` |
| Run engine | `Scripts/StartEngineWin.ps1 -Preset <name>` — never launch Main.exe / RenderTest.exe by path |
| Engine path-tracing test | `Scripts/TestPT.ps1` |
| Editor tests | `cd Source/Editor-Next && npx playwright test tests/<spec>.spec.js` (needs a live engine) |
| Verify a run | Assert on captured **stdout**, not the `*.Log` (empty offscreen). Bounded launch that survives the TASK-241 hang: `Invoke-EngineBounded` in `Scripts/Lib/Test-Engine.psm1`. |

## Continuity

Tasks and per-task history: `.backlog/tasks/` (Backlog.md). Durable decisions and invariants: basic-memory project `InnocenceEngine` (address explicitly; not the default).
