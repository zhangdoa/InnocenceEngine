---
id: TASK-228
title: >-
  Pre-existing TAA upper-half-inversion artifact in GISponza autotest —
  root-cause and fix
status: To Do
assignee: []
created_date: '2026-05-16 21:08'
labels:
  - rendering
  - bug
  - TAA
  - regression
dependencies: []
references:
  - .alignments/TASK-226.4-port-audit.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-226.4 peer review (commit e0e68900). The GISponza autotest frame capture shows the upper half of the framebuffer rendering an inverted (upside-down) view, plus a central black void in the scene. Pre-existing — present in both the pre-change and post-change captures of the TASK-226.4 review. Not caused by TASK-226.4.

Captured A/B frames at the time of discovery: `Bin/RelWithDebInfo/gpu_output_0055_baseline.png` and `gpu_output_0055_postchange.png` (may be cleaned between sessions — re-capture if needed).

Likely suspects (need bisect):
- TAA pass — motion-vector convention mismatch between RT0/RT3 GBuffer and TAA-pass reprojection.
- Final-blend / composition — UV flip in a y-axis transform.
- Renderer split between rasterized + GI passes — incorrect render-target slice composition.

This artifact has been present long enough that it shipped through several TASK-226.x docs commits without being flagged; either it was a recently-introduced regression masked by side-cache fill or a longer-standing issue. Bisect against the TASK-226.2 baseline frames if available.

Blocks: clean baseline for TASK-226.8 visual + perf gate (which compares against TASK-226.2 baseline). Without addressing this, fine-grained AC #4/#5 disocclusion-fill comparison is impractical.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause identified via bisect / RenderDoc capture (TAA / composition / GBuffer-flip / other)
- [ ] #2 GISponza autotest frame capture renders correctly — no upper-half inversion, no central black void
- [ ] #3 Bisect range + breaking commit recorded in alignment / closure note
- [ ] #4 Fix landed and visual A/B vs known-good baseline confirms regression closed
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-17: bug-fix diagnostic dispatch — reproduce + bisect + root-cause hypothesis. **No fix landed.**

**Reproduce:** YES at HEAD (`90750a1e` at the time of audit; current `5f24a923`). Capture frames 25 + 55 of GISponza autotest via `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen -capture_frame N`. Visual Read confirms upper-half upside-down + central black void.

**Duplicate found — RECONCILE:** TASK-223 ("FinalBlend readback path produces vertically-mirrored Sponza output", filed 2026-05-13) describes the same symptom. Its audit at `aaebe695` concluded "content-asset / camera-on-axis bilateral symmetry" — that audit checked left/right mirror (1.1 delta at x=640 centerline, 8-10% column-pair identity = no bilateral pixel mirror) but **did NOT check top/bottom mirror**, which is the actual artifact. TASK-223 should be closed-as-duplicate of TASK-228 or reopened with the corrected diagnosis.

**Bisect:** UNVERIFIED — regression predates the practical bisect window.
| SHA | Date | Subject | Verdict |
|---|---|---|---|
| 90750a1e | 2026-05-16 | docs(backlog): file TASK-226.4 followups | BAD |
| fe8d9f7c | 2026-05-15 | fix(rendering): SSAO kernel 32 | BAD |
| aab13d53 | 2026-05-14 | fix: drop double-gamma sqrtf in WriteCaptureToFile | BAD |
| 5c4553b7 | 2026-05-07 | docs(backlog): TASK-77.2 CL-2 peer review | BAD |
| 21d0e058 | 2026-05-01 | docs(backlog): TASK-77.1.{1,2,3} sub-tasks | BUILD-TESTED, autotest blocked by older harness gate state |

Artifact is at minimum 28+ commits / 10+ days old. Older bisect window worth exploring: TASK-77.x denoise stack (May 6–10: `9c876761`, `4588a5ac`, `53331e1f`, `20380ad4`, `b8ecf900`) and TASK-219 mass file splits (May 6 onward: `5f8c3565`).

**Root-cause hypothesis (audit, not bisect-verified):** `Source/Shaders/HLSL/opaqueGeometryProcessPass.frag:118-119` applies `y = 1.0 - y` to BOTH `screenPos_orig` AND `screenPos_prev` before computing `motionVec = screenPos_prev - screenPos_orig`. Flipping both inputs sign-inverts the Y component of the delta. `Source/Shaders/HLSL/TAAPass.comp:56` consumes `pixelPos + round(motionVector)` directly. Over many converged frames on a static camera, a Y-sign-inverted motion vector smears history across the horizontal centerline → upper-half temporal mirror of lower-half.

**Secondary suspect:** `Source/Shaders/HLSL/skyPass.comp:42-43` applies a Y-flip on output write (`writeCoord.y = viewportSize.y - 1 - y`) without a matching flip in the consumer's read path. Could mis-place sky content; co-suspect but lower priority.

**Recommended fix shape (shader-impl):** experiment: drop the `screenPos_orig.y` flip at line 118 (keep `screenPos_prev.y` flip → delta is `flip(prev) - orig`), rebuild shaders, recapture frame 55. If artifact resolves, ship. If not, try keeping both flips off entirely. If still not resolved, escalate the sky-pass write-flip secondary suspect. ~10 min of work to validate.

**Files for the fix dispatch:**
- `Source/Shaders/HLSL/opaqueGeometryProcessPass.frag:106-129` (motion-vector compute)
- `Source/Shaders/HLSL/TAAPass.comp:43-65` (motion-vector consumption; also line 57 `<=` vs `<` bounds check — separate one-off)
- `Source/Shaders/HLSL/skyPass.comp:42-45` (Y-flip on write)
- `Source/Shaders/HLSL/common/skyResolver.hlsl:1-12` (ray-direction math)
- `Source/Shaders/HLSL/preTAAPass.comp:22` (unused `flipYTexCoord` artifact — clean if touching)

**What was NOT verified by the diagnostic dispatch:**
- Hypothesis NOT tested via fix-and-recapture; only audit-grounded.
- Bisect window not exhausted past `5c4553b7` due to older-harness tooling.
- TASK-223's "camera-on-axis bilateral" content explanation may partially apply to a horizontal pattern that's separate from this vertical mirror; not verified.

Main session housekeeping after the bug-fix dispatch ended: sub-agent operated in main repo (NOT a worktree as the brief instructed) — left HEAD detached at `21d0e058`. Restored via `git checkout ecs-overhaul`; 3 session commits (`471a0804`, `e0e68900`, `90750a1e`) preserved via reflog.
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
