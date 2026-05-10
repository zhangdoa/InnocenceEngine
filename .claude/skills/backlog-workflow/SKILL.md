---
name: backlog-workflow
description: Use when filing, working, or closing a backlog task in this project. Project-specific extensions to user-level `backlog-workflow`: venue, gate filenames, integration-test commands, paper-port alignment artefacts.
---

# Skill: backlog-workflow (project extension)

Generic task-first workflow, status flow, ownership, cross-session continuity, and closure-evidence rules live in user-level `backlog-workflow`. This skill carries project-specific bindings.

## Venue

- Tasks live under `.backlog/tasks/*.md` as the single AI-authored cross-session medium.
- Manipulate via `mcp__backlog__*` tools (preferred) or direct file edits.
- `paper-port`-labelled tasks also produce an alignment artefact under `.alignments/` at closure (see skill `paper-audit`).

## Closure-staleness gate

`gates/closure-staleness.js` parses `TASK-\d+` from commit messages. Blocks when any referenced task is `In Progress` / `To Do` AND staged files include non-docs paths. Bypass: `[task-stays-open]` in the commit subject.

## Closure-evidence enforcement

`gates/test-run.js` parses the *main-session* bash transcript. Sub-agent transcripts are isolated. Before closing integration-test-relevant work (engine, editor, rendering, shaders), main-session pre-runs:

- **Engine**: `cmake --build Build --config RelWithDebInfo --target Main` then `Bin/RelWithDebInfo/Main.exe -total_frames N`.
- **Editor**: `cd Source/Editor-Next && npm test -- --workers=1 tests/<spec>.spec.js`.
- **Rendering pipeline**: `Bin/RelWithDebInfo/RenderTest.exe -test <name>`.

Docs-only path bypass (`DOCS_ONLY_PATH` regex in `.claude/hooks/lib/common.js`) handles backlog/harness commits automatically. Bypass not firing for a legitimately exempt path → extend the regex.

Closure-only docs CL exemption: a `Closure-Reason: <value>` commit-message footer re-applies the docs-only bypass when a task is flipping to Done. Use only for genuinely-obsolete / non-reproducible / superseded closures. Code-bearing CLs still require a qualifying test run.

## Mechanical exemptions (peer-review skip categories — see `peer-review-required`)

- Backlog file edits.
- Pure mechanical refactors where the file IS the diff.
- Hook self-edits / harness-internal disciplines.

## Cross-references

- User-level `backlog-workflow` — generic task-first workflow, status flow, ownership, anti-patterns.
- `peer-review-required`, `persistence-venue`, `commit-message-policy`, `session-start`.
- User-level `surface-dont-chase`.
