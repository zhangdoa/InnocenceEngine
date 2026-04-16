---
id: TASK-48
title: >-
  Add diagnostic logging to all silent ObjectStatus guards in rendering client
  passes
status: Done
assignee: []
created_date: '2026-04-16 19:05'
updated_date: '2026-04-16 21:02'
labels:
  - reliability
  - rendering
  - observability
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** Every rendering client pass has `if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated) return false;` with no logging. That's ~18 passes across Source/ExampleProject/RenderingClient/ that silently skip every frame when initialization fails.

**Fix:** Add `Log(Warning, "PassName::PrepareCommandList skipped: RenderPassComp not Activated");` to each guard. Consider extracting a shared macro or helper since the pattern is identical across all passes.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16 (completion):** Added `Log(Warning, "<resource> not Activated, skipping.")` to every silent `ObjectStatus != Activated` guard in the 22 rendering-client passes under `Source/ExampleProject/RenderingClient/`. The `Log` macro's `__FUNCTION__` prefix already provides the pass + method context, so the message stays terse.

Recap of guards added (most passes: 1; some passes more):
- `m_RenderPassComp` guard: all 22 passes.
- `m_Result` / `m_*Result` / `m_RadianceCache_{Even,Odd}` guards: FinalBlend, LightPass (5 resources), LuminanceAverage, OpaqueCulling, PostTAA, PreTAA, RadianceCacheFilterH/V, RadianceCacheIntegration, RadianceCacheReprojection, SSAO, SkyPass, SunShadowCulling, TAA, TiledFrustumGeneration.

Integration run now surfaces previously-invisible early-frame behavior: `RadianceCacheReprojectionPass` trips on `RadianceCache Even/Odd not Activated`, `LightPass` on `LuminanceResult not Activated`, `SkyPass` on `Result not Activated` — all during the first frames before deferred resources finish initializing. Build clean, RenderTest exit 0, integration exit 1 (expected — TASK-52 TDR unchanged).
<!-- SECTION:NOTES:END -->
