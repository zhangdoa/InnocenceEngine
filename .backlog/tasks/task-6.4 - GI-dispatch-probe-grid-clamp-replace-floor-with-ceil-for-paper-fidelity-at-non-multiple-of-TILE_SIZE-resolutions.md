---
id: TASK-6.4
title: >-
  GI dispatch + probe-grid clamp: replace floor with ceil for paper-fidelity at
  non-multiple-of-TILE_SIZE resolutions
status: Done
assignee: []
created_date: '2026-04-25 22:25'
updated_date: '2026-05-15'
labels:
  - rendering
  - GI
  - paper-fidelity
  - coordinates
  - superseded
dependencies: []
references:
  - Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl
  - Source/ExampleProject/RenderingClient/RadianceCacheConstants.h
parent_task_id: TASK-6
priority: low
---

## Closure (2026-05-15) — superseded by TASK-226

Closed as superseded by the AMD GI 1.0 reference-port effort (TASK-226). A floor → ceil dispatch fix is a local bandaid on a broken implementation; the reference impl has the correct dispatch arithmetic and we'll mirror it directly. No need to fix the divergence twice.

If after the port the dispatch is still wrong for the same reason this ticket flagged, file a fresh task scoped against the post-port file:line.

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from the TASK-127 paper-auditor findings (2026-04-25). The TASK-127 fix unified `SH_TILE_SIZE` between C++ and HLSL and resolved the visible 2/3 cutoff. But the auditor identified a separate, smaller divergence from AMD GI-1.0 / Capsaicin that was not the smoking gun for TASK-127 (because at 1920×1080 both axes are multiples of 8 and the divergence is invisible) but IS still a real paper-fidelity gap.

### The divergence

Capsaicin allocates and clamps the probe grid using **ceiling division**:

```
probe_count = (buffer_dimensions + probe_size - 1) / probe_size
maxProbeIndex = ceil(viewport / TILE_SIZE) - 1
```

Engine `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl:245` uses **floor**:

```
maxProbeIndex = uint2(g_Frame.viewportSize.xy) / RADIANCE_CACHE_TILE_SIZE - 1
```

And `:256`:

```
gridSize = int2(g_Frame.viewportSize.xy) / int(RADIANCE_CACHE_TILE_SIZE)
```

C++ allocation (`RadianceCacheReprojectionPass.cpp:297-298`) already uses ceiling, so the texture has the correct extent — but the shader's clamp under-reads the allocated grid by one row/column whenever the viewport is not a multiple of `TILE_SIZE`.

Same shape engine-wide for dispatch math (the TASK-127 fix added `RadianceCache::TileCount(...)` for the GI/RadianceCache/GIDenoise passes; equivalent floor patterns remain in `LightPass.cpp:320`, `GIFilterHorizontalPass.cpp:142`, `GIFilterVerticalPass.cpp:140`, `PreTAAPass.cpp:139`, `PostTAAPass.cpp:125`, `TAAPass.cpp:154`, `SSAOPass.cpp:238`, `SkyPass.cpp:115`, `FinalBlendPass.cpp:175`).

### Visible impact

At a viewport size where W % TILE_SIZE != 0 or H % TILE_SIZE != 0: a ≤ (TILE_SIZE - 1) pixel strip on the right and/or bottom of the frame is left out of GI dispatch / clamped to a stale probe entry. At 1920×1080 with TILE_SIZE = 8 the loss is **zero** (both axes are exact multiples). At a windowed-mode arbitrary resolution (e.g. 1377×873), the loss is up to 7 pixels per affected axis — visible but small.

### Acceptance

- `RadianceCacheCommon.hlsl` `maxProbeIndex` and `gridSize` use ceil-div
- All listed dispatch-extent floor patterns engine-wide migrated to a `TileCount`-style helper
- A windowed run at a non-multiple-of-8 resolution shows no edge-strip GI loss (paired before/after captures)

### References

- Paper-auditor artifact for TASK-127 (in main session transcript 2026-04-25)
- `Build/captures/TASK127_fix/` — captures showing the unrelated 2/3 fix (this task is about edge fidelity at arbitrary resolutions, not the rectangular cutoff)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 RadianceCacheCommon.hlsl maxProbeIndex (line 245) and gridSize (line 256) use ceil-div
- [ ] #2 All listed dispatch-extent floor patterns (LightPass, GIFilter*, TAA*, SSAO, Sky, FinalBlend) migrated to a TileCount-style helper
- [ ] #3 Windowed run at a non-multiple-of-8 resolution shows no edge-strip GI loss — paired before/after captures in the final summary
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
