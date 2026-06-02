---
name: backlog-workflow
description: Use when filing, working, or closing a backlog task in this project. Project-specific venue + commit-guard behaviour.
---

# Skill: backlog-workflow

## Venue

Tasks live at `.backlog/tasks/*.md` (Backlog.md format). Manipulate via the `backlog` CLI
(`backlog task create|edit|list|view ...`) or direct file edits. There is **no backlog MCP
under omp** — the `mcp__backlog__*` tools are gone.

## Enforcement (commit-guard extension)

`.omp/extensions/commit-guard/` intercepts `git commit` and enforces:

| Gate | Behaviour |
|---|---|
| closure-staleness | Commit references `TASK-N` but the task is still `In Progress`/`To Do` AND staged files include non-docs paths → block. Bypass: `[task-stays-open]` in the commit message. |
| test-run | A task closure (frontmatter flips to `status: Done`) requires a qualifying integration test in this turn. Docs-only commits auto-skip. Exemption: `Closure-Reason: <value>` footer for obsolete/non-reproducible/superseded closures. |

## Integration test commands

| Domain | Command |
|---|---|
| Engine | `Bin/RelWithDebInfo/Main.exe -total_frames N` |
| Editor | `cd Source/Editor-Next && npx playwright test tests/<spec>.spec.js` (live engine) |
| RenderTest | `Bin/RelWithDebInfo/RenderTest.exe -test <name>` |

Docs-only path regex (`DOCS_ONLY_PATH`) lives in `.omp/extensions/commit-guard/constants.ts`.
Extend it there if a legitimately-exempt path trips a gate.
