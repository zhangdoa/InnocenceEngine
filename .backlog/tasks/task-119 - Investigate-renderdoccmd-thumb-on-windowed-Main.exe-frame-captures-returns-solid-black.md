---
id: TASK-119
title: >-
  Investigate: renderdoccmd thumb on windowed Main.exe frame captures returns
  solid black
status: Done
assignee: []
created_date: '2026-04-20 19:30'
updated_date: '2026-04-20 20:01'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Main.exe -total_frames 20 -capture_frame 8` (windowed mode) produces a valid .rdc at Build/captures/frame_capture.rdc. `renderdoccmd thumb` succeeds (exit 0, says "Wrote thumbnail...") but the output PNG is solid black.

Reproduced against two commits: HEAD~1 (30a3ff99 [S1.3]) and HEAD (39de2a1b [S1.5]). Both versions exit 0 in offscreen mode, both show black thumbnail in windowed+capture mode. Engine log for windowed mode shows scene load + pass execution + normal exit.

This breaks the CLAUDE.md "SOP — use RenderDoc instead of asking the user" workflow for radiance cache quality verification. Options to investigate:
- Does the capture land on a frame prior to the first Present, so the swap chain is still in its cleared state?
- Is the in-process RenderDoc API capturing a different frame than `-capture_frame N` promises?
- Does the engine's windowed-mode presentation write to a different surface than renderdoccmd's thumb tool reads?
- Alternative: extract a specific render target (e.g. OpaquePass RT0, or LightPass Illuminance) via `renderdoccmd convert` / xml-tools instead of relying on `thumb`.

Fixing this unblocks visual A/B comparisons for every subsequent radiance-cache CL (TASK-6 / TASK-115/116/117).</description>
<parameter name="labels">["tooling", "rendering", "TASK-6"]
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-04-20: Resolved by user insight — black frame 8 thumbnails were GISponza mid-async-load. Capture frame 60 of total_frames 100 shows the fully loaded scene. The tooling works; the CLAUDE.md auto-test scene schedule comment (“Frame 5: GISponza loads, good capture range 6–9”) was the problem — with the async loader it's more like frame 60+. Documenting in the roadmap so subsequent radiance-cache CLs use the right capture window.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Thumbnails from windowed `Main.exe -capture_frame N` now recovered — the issue wasn't the `renderdoccmd thumb` tool or the engine's capture API, it was capturing a frame too early. Sponza streams in asynchronously, and at frame 8 (or even frame 15) the GBuffer still renders black. Frame 60 of a 100-frame run shows the fully loaded scene correctly, confirming the capture pipeline is functional. Outcome: visual A/B for radiance-cache CLs is unblocked; the "good capture range" note in CLAUDE.md under RenderDoc SOP should be updated from "6–9" to something like "60+ after a 100-frame warmup" to match the async loader's actual cadence.
<!-- SECTION:FINAL_SUMMARY:END -->
