---
name: backlog-workflow
description: Use when filing, working, or closing a backlog task in this project. Project-specific venue + gate behaviour.
---

# Skill: backlog-workflow

## Venue

Tasks live at `.backlog/tasks/*.md`. Manipulate via `mcp__backlog__*` tools (preferred) or direct file edits.

## Gates

| Gate | Behaviour |
|---|---|
| `closure-staleness.js` | Commit references `TASK-N` but task is still `In Progress`/`To Do` AND staged files include non-docs paths → block. Bypass: `[task-stays-open]` in commit subject. |
| `test-run.js` | Closing a task (status → Done) requires an integration test run in this turn (via Bash). Docs-only path: auto-skip. Exemption: `Closure-Reason: <value>` footer for obsolete/non-reproducible/superseded closures. |

## Integration test commands

| Domain | Command |
|---|---|
| Engine | `Bin/RelWithDebInfo/Main.exe -total_frames N` |
| Editor | `cd Source/Editor-Next && npm test -- tests/<spec>.spec.js` |
| RenderTest | `Bin/RelWithDebInfo/RenderTest.exe -test <name>` |

Docs-only path regex lives in `.claude/hooks/lib/common.js::DOCS_ONLY_PATH`. Extend that regex if a legitimately-exempt path triggers the gate.
