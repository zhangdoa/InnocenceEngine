# Project direction

Point-in-time strategic facts the dispatcher needs at session-start to route new work correctly. Update by editing this file in the same CL that lands the directional change.

## PT-primary rendering (2026-04-30)

GPU path tracer is the primary rendering pipeline. Rasterization-trick subsystems — shadowmap pipeline (cube atlas, all shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, and most of TAA's reason-for-being — are demoted to fallback / debug-comparison only. PT subsumes them at higher quality (true visibility via ray query, true GI via path integration, no screen-space artifacts); maintaining quality investment in two parallel pipelines does not pay for itself for a single-user research engine.

Operational consequences:

- New rendering work goes through PT first; rasterizer parity is no longer load-bearing.
- When auditing a rasterization-side task, ask "does this still pay for itself if rasterizer is fallback-only?" — if no, archive / defer / demote.
- Immediate-focus task: **TASK-77.1** (world-space radiance cache as PT denoiser) — first concrete decomposition of TASK-77's umbrella. Pivoted 2026-04-30 from "temporal reprojection reusing TAA motion vectors" because motion vectors come from a rasterized G-pass; coupling PT denoiser to a demoted-subsystem output is structurally wrong. World-space cache is rasterizer-independent.
- TASK-108 (unified radiance cache): PT-side world-space tier folded into TASK-77.1; SSGI-side scope obsolete under PT-primary.
- TASK-128 (ping-pong helper extraction): trigger link to TASK-77.1 dropped — world-space caches do not ping-pong (they are 3D-world constructs: probe grids, hash grids, voxel cascades). Task remains demoted, awaiting a different third-caller site.
- TASK-77 umbrella closure condition = rasterizer demotion CL lands (i.e. the moment the rasterizer becomes labelled debug-comparison-only in code, not just intent).

Direction-approval batch (commit `3dcead84`, 2026-04-30): TASK-166 closed (peer-review gate live), TASK-153 closed obsolete (no shadowmaps under PT), TASK-137 archived (split-before-grow on demoted subsystem), TASK-108 paused, TASK-128 demoted, TASK-77 direction-approved, TASK-77.1 filed.
