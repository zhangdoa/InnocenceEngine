---
id: TASK-227.4
title: Render-graph Phase 4 — exotic pass Setup disposition (schema-extend vs exempt)
status: To Do
assignee: []
created_date: '2026-05-31 12:54'
labels:
  - rendering
  - render-graph
  - refactor
dependencies:
  - TASK-227.3
parent_task_id: TASK-227
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Phase 4 of the TASK-227 render-graph umbrella. Per-pass decide, for each bin-(c) exotic pass, whether to extend the graph schema to absorb its Setup or leave it a permanent exemption (TASK-227 AC#4 — migrate OR explicitly exempt with reason). By Phase 3 these already dispatch via opaque-kernel nodes; this phase is about their *Setup* (P1), not ordering (P2).

Bin-(c) dispositions (from RFC doc-1 §9), to confirm/revisit:
- PTPass — compile-time feature-gated binding count (19–26 via `if constexpr`), scene-load callbacks, material-buffer rebuild, TLAS poll. Likely needs conditional-binding schema; else exempt.
- LightPass — 21 bindings from 6+ upstream, conditional SSRC source. Candidate for moderate-tier if conditional-binding schema lands.
- NRDIntegrationAdapter — bypasses RenderPassComponent model (raw ID3D12 CL, own NRD heaps). PROPOSED PERMANENT EXEMPTION.
- VolumetricPass — commented-out stub, own internal graph, unwired. Exempt until un-stubbed.
- DebugPass — CPU readback + conditional texture selection. Likely exempt.

Closes TASK-227 AC#4. Design reference: backlog doc-1 §6, §9.
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
