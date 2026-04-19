---
id: TASK-95
title: >-
  Enforce setter-reply contract: IPC mutations return read-back state, not
  echoed payload
status: To Do
assignee: []
created_date: '2026-04-19 17:34'
labels:
  - editor
  - ipc
  - structural
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Context

Two recent fixes — SET_DEV_TOGGLE (commit 2586477b) and SET_VIEWPORT_SOURCE (commit d4fe5462) — addressed the same latent flaw: the reply body was a mirror of the incoming payload rather than the authoritative post-mutation state read back from the engine. Clients that committed the reply as "server truth" were actually committing their own guess.

UPDATE_ENTITY_PROPERTY already follows the right pattern (writes `committed = read_back_field`). TRIGGER_DEV_ACTION is a fire-and-forget action — not a setter — so doesn't apply.

## Structural weakness

The setter-reply contract is implicit and unenforced. Each `reg("SET_...")` handler author independently decides whether to (a) echo the payload or (b) read back authoritative state. Nothing in the dispatcher's type, naming, or doc comments hints that (b) is required. The next new setter will likely regress.

## Acceptance

- A short convention comment in EditorService.cpp's handler-registration block spelling out: "`SET_*` handlers MUST return the post-mutation state read back from the authoritative source, never the submitted payload." Place at the top of `RegisterBuiltinHandlers` where `reg` is defined.
- Optional follow-up (do not block): a `regSetter(...)` helper that takes separate `mutate(payload)` and `readback()` lambdas and composes them — so the contract is expressed by the type, not by comment. Only worth building if a third setter motivates it.
- Audit the current EditorService.cpp handlers once after the doc lands; confirm SET_DEV_TOGGLE, SET_VIEWPORT_SOURCE, UPDATE_ENTITY_PROPERTY all comply, and note any others found.

## Why backlog, not ship-now

The two known violations are already fixed. This task exists so the next setter-handler author sees the rule.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
