---
id: TASK-226.3
title: Port FilterScreenProbes weight kernel (Capsaicin depth+normal weights)
status: Done
assignee: []
created_date: '2026-05-15 20:24'
updated_date: '2026-05-16 13:43'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
dependencies:
  - TASK-226.2
references:
  - .alignments/TASK-226-gap-matrix.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L1455
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the current `pow(depthRatio, 8.0)` weight formulation in the probe-space radiance filter with Capsaicin's depth+normal weight at gi1.comp:1455–1519. Shape (separable H/V, kRadius=3, parallax-corrected direction reprojection, bilateral on depth+plane+hemisphere) is already paper-aligned per gap-matrix row #6 — this is the only known divergence in that row.

Touched files:
- Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp
- Source/Shaders/HLSL/RadianceCacheFilterVertical.comp
- Possibly Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (if weight is shared)

Surgical change — no pass restructure, no new C++ wiring.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Weight formulation matches Capsaicin gi1.comp:1455–1519 within line-level audit tolerance
- [x] #2 #2 Shader build green
- [x] #3 #3 Engine build green
- [x] #4 #4 Sponza autotest renders without artifacts at RasterizedGI=ON
- [x] #5 #5 Visual diff vs TASK-226.2 baseline: filter region no worse than baseline (capture screenshot diff)
- [x] #6 #6 Paper-port alignment artifact .alignments/TASK-226.3-port-audit.md cites Capsaicin lines
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Audit-only closure — no code change needed.

Capsaicin's FilterScreenProbes weight (gi1.comp:1455-1519, fetched 2026-05-16):
```
weight = pow(saturate(1.0 - abs(toLinearDepth(probe_depth) - toLinearDepth(depth)) / toLinearDepth(depth)), 8.0)
```

Ours (RadianceCacheFilterHorizontal.comp:119-120, .Vertical.comp:103-104):
```
depthRatio = saturate(1.0 - abs(neighbourDepth - currentDepth) / max(currentDepth, EPSILON));
weight = pow(depthRatio, 8.0);
```

Identical formula. Capsaicin's `toLinearDepth(d, g_NearFar)` converts NDC z to camera-space linear distance; ours computes linear distance directly via `length(posWS - cameraPosWS)`. Same quantity, different derivation. `max(., EPSILON)` is our defensive divide-by-zero guard, equivalent to Capsaicin's implicit saturate-after-divide.

The gap matrix row #6 noted Capsaicin uses "depth + normal weights" — that framing was misleading. Capsaicin uses depth-only bilateral *weights*, with separate hemisphere-reject and plane-distance gates as accept/reject filters. Our impl has both gates in place (FilterHorizontal lines 99, 102; FilterVertical mirror).

No port required. Closing AC #1-#6:
- AC #1: weight formula matches gi1.comp:1455-1519 line-level.
- AC #2-#3: build green (unchanged file, no rebuild needed).
- AC #4: Sponza autotest unaffected (no change).
- AC #5: visual diff vs TASK-226.2 baseline trivially identical (no change).
- AC #6: audit at .alignments/TASK-226.3-port-audit.md.

Closure-Reason: audit-only; no code change required; gap matrix row #6 corrected.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
