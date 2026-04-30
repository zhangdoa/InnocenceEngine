---
id: TASK-77.1.2
title: >-
  GPUPathTracerDenoisePass — denoise read + composition (TASK-77.1 phase 1
  sub-2)
status: To Do
assignee: []
created_date: '2026-04-30 19:43'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies:
  - TASK-77.1.1
parent_task_id: TASK-77.1
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Author `GPUPathTracerDenoisePass` (compute, one thread per pixel) that reads the per-frame noisy buffer + hash-grid produced by TASK-77.1.1, applies the composition rule, and writes the denoised result. Wire it into the rendering client between `GPUPathTracerPass` and the `l_hdrSource` consumer site at `ExampleRenderingClient.cpp:483-490`. Respects `DispatchOrBypass` clear-on-bypass semantics (TASK-171 / TASK-182) so debug A/B does not see stale denoised output.

## Composition rule (resolved by design call 2026-04-30)

```
denoised = lerp(noisy, cached, saturate(sampleCount / 32))
```

Reads the cached `(radiance, sampleCount)` payload from the hash-grid; falls back to the noisy frame when `sampleCount` is low.

## Owner

- **Pass authoring** (Setup / Initialize / PrepareCommandList / Execute / m_OnResize / Terminate, ClearOnBypass plumbing): `rendering-researcher`.
- **HLSL composition kernel + resource state hazards** on the noisy → denoised buffer transition: `graphics-api-expert`.

## Files (anticipated, not prescriptive)

- **New**: `Source/ExampleProject/RenderingClient/GPUPathTracerDenoisePass.{h,cpp}`.
- **New**: `Source/Shaders/HLSL/GPUPathTracerDenoise.hlsl` (compute kernel implementing the lerp).
- **Modified**: `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` near line 483-490 — swap PT-mode `l_hdrSource` source from `GPUPathTracerPass::GetResult()` to `GPUPathTracerDenoisePass::GetResult()`.

## Out of scope

- Visual A/B numerics, paper-port audit, closure (TASK-77.1.3 owns those).
- Hash-grid resource authoring (TASK-77.1.1 owns it — this task consumes).

## Cross-refs

- Parent: TASK-77.1.
- Hard dependency: TASK-77.1.1 (must land first — consumes its per-frame noisy buffer + hash-grid).
- Sibling: TASK-77.1.3 (validation closure).
- Discipline anchors: `peer-review-required.md`, TASK-171 (m_Bypassed dispatch), TASK-182 (ClearOnBypass semantic), `regression-fix-flow.md`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 GPUPathTracerDenoisePass authored with the standard pass shape (Setup, Initialize, PrepareCommandList, Execute, m_OnResize, Terminate)
- [ ] #2 Compute kernel implements lerp(noisy, cached, saturate(sampleCount/32)) using the cached (radiance, sampleCount) payload from the hash-grid
- [ ] #3 m_ClearOnBypass = true opt-in (TASK-182 pattern) + RecordClearCommandList override that clears the denoised UAV when the pass is bypassed
- [ ] #4 Wired into ExampleRenderingClient such that PT-primary mode routes the denoised buffer to l_hdrSource (line 483-490)
- [ ] #5 Engine builds RelWithDebInfo clean; GBV clean on smoke test (no resource-state errors on the noisy → denoised transition)
- [ ] #6 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
