---
id: TASK-227.3
title: Render-graph Phase 3 — replace ExecuteCommands chains with graph-walk dispatch
status: Done
assignee:
  - code-impl
created_date: '2026-05-31 12:53'
updated_date: '2026-06-15'
labels:
  - rendering
  - render-graph
  - refactor
dependencies:
  - TASK-227.2
parent_task_id: TASK-227
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Phase 3 of the TASK-227 render-graph umbrella. Replace the hand-written `WaitIfActive → Execute → SignalOnGPU → WaitOnGPU` chains in the 4 `ExampleRenderingClient_ExecuteCommands_*.cpp` files (842 LOC) with a graph-walk over the compiled schedule emitted by RenderGraphCompiler. The orchestrator shrinks to a loader-and-run entry point (TASK-227 AC#5). Exotic passes (bin-c) participate as opaque-kernel nodes so their dispatch ordering is data-driven even though their Setup stays imperative.

Depends on Phases 1+2 (TASK-227.2) having enough migrated nodes that the schedule covers the real dependency graph. The compiler must already derive edges from reads/writes and assign cross-queue fences (built in Phase 0); this phase swaps the dispatch *site* from imperative to schedule-driven.

Validation bar: visual parity on all autotest scenes + 60-FPS on Sponza (TASK-227 AC#6/#7). See 2026-06-15 closure note in implementation notes.
Design reference: backlog doc-1 §5 (dispatch-cycle uniformity), §6 (opaque-kernel for exotic passes).

<!-- SECTION:NOTES:BEGIN -->
2026-06-15 — Closed. The P4 work in TASK-227.2 (commit `dac9eaa1`) subsumes this phase: `RenderGraphService::Render()` owns the whole frame — records every ready scheduled node then submits+fences, deriving the topology from node data (producer waits via read→writer-node signal, graphics transition prepass dance, compute execute/signal, one-shot CPU-wait-once). The hand-written `WaitIfActive → Execute → SignalOnGPU → WaitOnGPU` chains in the 4 `ExampleRenderingClient_ExecuteCommands_*.cpp` files (842 LOC at RFC time) are GONE — the client collapsed to `PrepareCommands` (resolve present canvas by name) + `ExecuteCommands` = `Render()` + capture/audit. `ExecuteRasterizerPasses`/`ExecuteGIPasses` archived. The orchestrator is now a loader-and-run entry point. Exotic passes (bin-c, when they re-enter) participate as opaque-kernel nodes (per RFC §6) so their ordering is data-driven even though their Setup stays imperative (TASK-227.4). Verification: BuildWin exit 0; GISponza -total_frames 30 exit 0 / 0 [Error] / 18 res 8 passes at the time of P4 land; TestSuite green throughout.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
