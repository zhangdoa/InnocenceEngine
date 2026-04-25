---
id: TASK-95
title: >-
  Enforce setter-reply contract: IPC mutations return read-back state, not
  echoed payload
status: Done
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
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

Convention comment landed in `Source/Engine/Services/EditorService.cpp` at the top of `RegisterBuiltinHandlers`, next to the `reg` lambda. Cites prior-art commits 2586477b and d4fe5462. Audit confirmed all SET_*-style handlers comply.

`regSetter` template helper not built — the gating condition (three or more violations) was not met (zero violations among true SET_* handlers).

## Final Summary

**Audit (SET_* handlers + UPDATE_ENTITY_PROPERTY):**

| Handler | Authoritative read-back source | Status |
|---|---|---|
| `SET_DEV_TOGGLE` | `DevToggleRegistry::Get(name)` | compliant |
| `SET_VIEWPORT_SOURCE` | `ViewportSourceOverride::Get()` | compliant |
| `UPDATE_ENTITY_PROPERTY` | re-reads component field after write (`m_LocalPos`, `m_LuminousFlux`, etc.) | compliant |

**Other mutating handlers (out of strict SET_* scope, observed for completeness):**

| Handler | Reply shape | Note |
|---|---|---|
| `LOAD_SCENE` | echoes `{path}` | scene load is async; the spec test (`SCENE_UPDATED event drives sceneStore.refresh and clears isLoading`) explicitly relies on the reply NOT clearing `isLoading` and waits for the engine-fired `SCENE_UPDATED` event to deliver authoritative scene state. Treating it as a setter would be wrong. |
| `IMPORT_ASSET` | echoes `{path}` | fire-and-forget enqueue; no engine-side authoritative state to read back from this handler. |
| `SAVE_SCENE` | reads `sceneService->GetCurrentSceneName()` | already authoritative. |
| `ENTITY_CREATE` / `ENTITY_DELETE` / `ENTITY_RENAME` | re-list scene from `EntityRegistry` | already authoritative. |
| `TRIGGER_DEV_ACTION` | echoes `{name}` | fire-and-forget action — out of scope per the original task description. |

No latent SET_*-pattern violations to fix in this CL. The two non-compliant payload-echoes (`LOAD_SCENE`, `IMPORT_ASSET`) are deliberate: their authoritative state lands via separate event/poll channels, not the reply.

**Build:** `MSBuild Source/Engine/Engine.vcxproj -p:Configuration=Debug -p:Platform=x64` succeeded; only pre-existing C4003 (`max` macro) warnings, unrelated to this change.

**Tests:** `npx playwright test tests/ipc-contract.spec.js tests/scene-vertical.spec.js --workers=1` → 8/8 passed in 17.6s. (Parallel workers timed out due to port-8081 contention from the editor spawning the engine binary in each worker — unrelated to this change; existing infra issue.)

**Not verified:**
- Did not run the full Playwright suite — only the two specs the producer brief named.
- Did not exercise a real engine roundtrip for the convention-only comment change; the change is comment-only and cannot affect runtime behaviour, so the suite confirms only that no regression slipped in.
- Did not validate the `LOAD_SCENE` / `IMPORT_ASSET` async-state-arrives-via-event paths beyond what the existing scene-vertical spec already exercises; these were observed during the audit but flagged for the producer to triage if the contract should be tightened to cover async-deferred mutators.
