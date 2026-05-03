# Stages

An agent is a stage of actions, guided by disciplines. Each stage is invoked by the dispatcher when work matches its description.

| Stage | What it does | File |
|---|---|---|
| (project-owner) | Product direction, priorities, merge approval | — (the human user) |
| `task-mgmt` | Session-start briefing, backlog ops, cross-stage coordination, dispatch | `.claude/agents/task-mgmt.md` |
| `design` | Decide structure / naming / tech / split. Plan, no diff. | `.claude/agents/design.md` |
| `harness-impl` | `.claude/` infrastructure (hooks, agents, disciplines, settings) | `.claude/agents/harness-impl.md` |
| `ci-build-impl` | CMake + scripts + build-chain plumbing | `.claude/agents/ci-build-impl.md` |
| `code-impl` | C++ / TypeScript source diffs (engine, editor, foundation, services, platform, tests) | `.claude/agents/code-impl.md` |
| `shader-impl` | HLSL diffs | `.claude/agents/shader-impl.md` |
| `bug-fix` | Reproduce → bisect → identify breaking commit → understand → hand off to impl | `.claude/agents/bug-fix.md` |

Paper-porting is not a separate stage — it folds into `code-impl` or `shader-impl` (or both) when the task carries the `paper-port` label. The alignment audit is a fresh-context dispatch of the same impl stage at closure.

## Dispatch

Main-session = dispatcher. Per turn:

1. Read scope from owned subtrees' `CLAUDE.md` and staged-file paths.
2. Identify which stage the work matches.
3. Invoke via `Agent` tool.
4. Relay results; enforce universal gates on commit.

Cross-stage work → coordinate via `task-mgmt`.

## Universal gates

`.claude/hooks/commit-gate.js`: file size, attribution, peer review, closure evidence with test-run backing. Stage-specific disciplines (paper-port alignment, serialize-determinism round-trip, live-engine Playwright) live in the matching `disciplines/<stage>/` directory; agents load them when they enter that stage.
