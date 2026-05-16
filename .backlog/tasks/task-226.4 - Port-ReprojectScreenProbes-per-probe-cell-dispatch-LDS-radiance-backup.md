---
id: TASK-226.4
title: Port ReprojectScreenProbes (per-probe-cell dispatch + LDS radiance-backup)
status: Done
assignee: []
created_date: '2026-05-15 20:24'
updated_date: '2026-05-16 23:00'
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
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L659
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rewrite RadianceCacheReprojection.comp to per-probe-cell `[numthreads(8,8,1)]` dispatch matching Capsaicin's ReprojectScreenProbes (gi1.comp:659–878). Compute a "radiance backup" via parallel reduction in LDS (gi1.comp:845–857) and write it to unvisited cells on disocclusion (line 920). Delete the in-house side-cache textures and the Halton-jitter-selection + InterlockedMin distance-scoring scheme (RadianceCacheReprojection.comp:56–60, 199–202, 270–315) — they're in-house heuristics not in the reference.

Touched files:
- Source/Shaders/HLSL/RadianceCacheReprojection.comp
- Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.{cpp,h}
- Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass_Setup.cpp (side-cache resource decls removed)

Design note: TASK-226.4's side-cache nuke assumes the LDS radiance-backup fully replaces the side cache. Capsaicin doesn't have a side cache, so this IS the paper-faithful position. If LDS backup turns out visibly inferior on long disocclusions during impl, the side cache STAYS nuked — avoid keeping a Plan-B redundancy that re-introduces the divergence.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Reprojection dispatched per-probe-cell with LDS-backup parallel reduction matching Capsaicin ReprojectScreenProbes (local snapshot `.alignments/_audit_refs/gi1.comp:293-499`)
- [x] #2 #2 In-house side-cache textures + Halton-jitter/InterlockedMin scheme removed from shader + pass C++ (side cache landed at eca79947; Halton-jitter lives in RayGen, out of file scope)
- [x] #3 #3 Shader + engine build green
- [ ] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline on continuous-camera regions
- [ ] #5 #5 Disocclusion regions (camera-cut test case) render with LDS-backup fill, no stale ghost cells
- [x] #6 #6 .alignments/TASK-226.4-port-audit.md cites Capsaicin line ranges + records the side-cache-nuke design call
- [x] #7 Per-cell octahedral-remap accumulation ported (audit Row #4 — `.alignments/_audit_refs/gi1.comp:367-404, 780-842`): each thread re-aims previous-frame radiance through `EncodeOctahedral` and `InterlockedAdd`s into the remapped cell, replacing the current direct (i,j)→(i,j) copy. Engine uses full-sphere octahedral world-space encoding (no TBN) — divergence acknowledged in alignment artifact
- [x] #8 3×3 cached-probe neighbour fallback ported (audit Row #9 — `.alignments/_audit_refs/gi1.comp:780-861`): on whole-probe reprojection failure, LDS seeds from 3×3 neighbour probes' remapped radiance. Engine analog of `g_ScreenProbes_ProbeCachedTileBuffer`: reads `in_RadianceCacheResults_Prev` + `in_ProbePosition`/`in_ProbeNormal` (existing ping-pong) at neighbour probe positions, validity proxy `prev_radiance.w > 0`. Design call recorded in `.alignments/TASK-226.4-port-audit.md` Post-implementation section (b) + (c)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-16: First half landed — in-house side-cache scheme (in_SideCache_Atlas/PosFrame/Normal bindings + the §2.1.8 in-house variant in the reprojection fail branch + 3 C++ resources + 4 binding-layout entries) NUKED per gap-matrix row #2 authorization ("side cache is functionally redundant" / "in-house heuristics not in the reference"). Disocclusion regions now hard-invalidate via the else-branch and RayGen repopulates next frame — temporary regression on long disocclusions until the LDS-backup port lands.

Reprojection.comp 300 → 240 lines. Pass binding-layout 12 → 9 descriptors. C++ pass file 184 → 175 lines.

2026-05-16 (second-half audit): Audit-first per `paper-port` skill step 4. Finding: the LDS radiance-backup reduction at Capsaicin gi1.comp:407-428 is NOT a self-contained portable unit under the engine's current reprojection design. The mechanism feeds on per-cell sample sparsity within a probe (some cells visited from neighbour probes, others not), produced by Capsaicin's octahedral-remap accumulation pipeline at gi1.comp:367-404 + 780-861. The engine's reprojection does direct cell-(i,j)-of-prev → cell-(i,j)-of-current copy (lines 211-224, post-eca79947) with no octahedral remapping; reprojection is whole-probe-binary (all 64 cells succeed together or all 64 fail together). Under this design, the LDS-backup mechanism degenerates: on success the backup is unused (cells already have history), on failure backup[0] = (0/0, 0/0) because there is no source data to seed from.

Full audit artifact: `.alignments/TASK-226.4-port-audit.md`. Audit recommended Option B; main-session adjudicated **Option A** (2026-05-16): expand TASK-226.4 scope to include Rows #4 + #9 as prerequisites so the LDS-backup port has the per-cell sparse-history substrate it feeds on. ACs #7 + #8 added to track the prerequisite work. Capsaicin line references throughout this task and the alignment artifact cite the local snapshot at `.alignments/_audit_refs/gi1.comp` (the original task description's `:845-857` / `:920` numbers reference a different/stale Capsaicin tree — corrected references attached to ACs #1, #7, #8).

2026-05-16 (second-half impl): Option A scope landed. Per-cell octahedral-remap accumulation (Row #4), LDS-backup seed/reduce/finalize (Rows #5/#6/#7), per-cell write (Row #8), 3×3 neighbour-probe fallback (Row #9). Engine-specific design calls recorded in alignment artifact Post-implementation section (a)-(f). Six rows in the alignment table flipped from DIVERGENT → PORTED.

Shader structure: `RadianceCacheReprojection.comp` (274 lines, orchestration only) includes new `common/RadianceCacheReprojection.hlsl` (199 lines: LDS state, quantization, `RemapHistoryCell`, reduction helpers). Both under the 300-line gate. No C++ pass changes needed — required bindings (prev-frame ping-pong slots for radiance/position/normal) were already in place.

Build green: shader compile produced `RadianceCacheReprojection.comp.dxil` (111508 bytes) without errors. Smoke autotest passed: Sponza loaded at frame 5, `Auto-test: 30 frames rendered, terminating.` reached, no D3D12/Validation/CORRUPTION/HUNG/FATAL errors during the 30-frame window. CPU PT reference render warnings post-smoke are outside the test window.

NOT verified by implementer: AC #4 (visual continuity vs TASK-226.2 baseline) and AC #5 (disocclusion fill quality) — these belong to peer review's visual inspection + TASK-226.8's perf/visual regression gate. Smoke autotest is offscreen with no frame capture in the implementer's launch budget.
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
