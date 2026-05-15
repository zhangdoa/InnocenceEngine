---
id: TASK-226.8
title: 'Visual + perf gate on Sponza, port-audit alignment artifact'
status: To Do
assignee: []
created_date: '2026-05-15 20:25'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
  - audit
dependencies:
  - TASK-226.1
  - TASK-226.3
  - TASK-226.4
  - TASK-226.5
  - TASK-226.6
  - TASK-226.7
references:
  - .alignments/TASK-226-gap-matrix.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp
parent_task_id: TASK-226
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Final TASK-226 closure gate. After TASK-226.1 through TASK-226.7 have landed, re-run the TASK-226.2 captures (frame-time + 4-angle screenshots) post-port. Diff visually against a Capsaicin reference Sponza capture (user-provided; if unavailable, against the TASK-226.2 RasterizedGI=OFF / PT-only baseline as the next-best proxy). Make the visual-parity AND 60-FPS-bar calls based on the captured numbers.

Outputs:
- `.alignments/TASK-226-port-audit.md` — full alignment artifact: per-stage Capsaicin line-range citation, the perf delta vs TASK-226.2 baseline, the visual-parity assessment, and the 60-FPS-bar status.
- If perf bar missed: written re-scope proposal for AC #5 ("no regression vs RasterizedGI=ON baseline").
- If visual parity missed in any stage: re-open the relevant TASK-226.N or file a TASK-226.N-fix sub-CL.

This closes umbrella TASK-226 ACs #4 and #5.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Post-port Sponza captures (frame-time + 4 screenshots) saved under .alignments/TASK-226-port-audit/
- [ ] #2 #2 .alignments/TASK-226-port-audit.md written: per-stage Capsaicin citations, perf delta vs TASK-226.2 baseline, visual-parity verdict, 60-FPS verdict
- [ ] #3 #3 If perf bar missed: re-scope proposal for AC #5 included in the audit
- [ ] #4 #4 If visual parity missed: re-opened sub-CL(s) filed and referenced in the audit
- [ ] #5 #5 TASK-226 umbrella ACs #4 (visual) and #5 (60-FPS or re-scoped) marked according to audit verdict
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
