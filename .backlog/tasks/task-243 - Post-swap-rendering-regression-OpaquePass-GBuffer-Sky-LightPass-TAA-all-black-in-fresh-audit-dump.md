---
id: TASK-243
title: >-
  Post-swap rendering regression: OpaquePass GBuffer / Sky / LightPass / TAA all
  black in fresh audit dump
status: To Do
assignee:
  - code-impl
created_date: '2026-06-15'
labels:
  - rendering
  - bug
  - dx12
  - render-graph
  - regression
dependencies:
  - TASK-242
priority: high
---

## Description

### Evidence (fresh audit dump, 2026-06-15, post-TASK-242 fix)

With the audit-dump path now working (TASK-241 + TASK-242), the first fresh
post-swap dump reveals a deep rendering regression. Per-pass luminance stats
(decoded from `Bin/audit_*.hdr`, 1280x720, custom RGBE parser):

| Pass | nonzero% | lum mean | lum max | Verdict |
|------|---------:|---------:|--------:|---------|
| OpaquePass RT0 (BaseColor) | **0.0%** | 0.0 | 0.0 | **BLACK — GBuffer empty** |
| SunShadowRT_Visibility | 100% | 1.004 | 1.004 | Flat 1.0 (no occlusion, but written) |
| SkyPass | **0.0%** | 0.0 | 0.0 | **BLACK** |
| LightPass Luminance | **0.0%** | 0.0 | 0.0 | **BLACK** |
| LightPass Illuminance | **0.0%** | 0.0 | 0.0 | **BLACK** |
| TAA | **0.0%** | 0.0 | 0.0 | **BLACK** |
| FinalBlend | 100% | 6e-5 | 6e-5 | Flat near-black (cleared-buffer floor) |

### Comparison with 2026-06-01 pre-swap baseline

The 2026-06-01 dumps (in `Bin/RelWithDebInfo/audit_*.hdr`, pre-`df40414a` LightPass
swap and pre-`51c83b30` C++23) showed:
- OpaquePass RT0: **healthy** (96.8% nz, mean 1.869, max 5.256)
- Sky: **healthy** (100% nz, mean 1608)
- LightPass Luminance: **healthy** (35.2% nz, mean 0.60, max 26.4)
- SunShadowRT_Visibility: all-zero (the pre-existing shadow bug)
- FinalBlend: dim-but-structured (mean 0.072, max 0.83)

So the regression is **strictly worse than pre-swap**: the entire opaque
GBuffer now produces nothing, which cascades to every downstream pass. The only
improvement is SunShadowRT now writes 1.0 (was 0.0).

### Why this matters / scope

The scene loads without crashing (UnitTest.InnoScene: meshes, materials,
textures all initialize — confirmed in the audit run log). So the geometry data
exists but is **not being rasterized into the GBuffer**. Candidate causes:
- OpaquePass draw-call submission broken in the render-graph path (the
  `df40414a` / TASK-227 render-graph overhaul migrated OpaquePass to a JSON node;
  the draw-call binding or the DrawModelGroups dispatch may not be wired).
- The OpaquePass OutputMergerTarget RTVs not bound, so draws go nowhere.
- A transition-state bug leaving the GBuffer in the wrong state for the raster
  write.

### How to reproduce

```
cd Bin
RelWithDebInfo\Main.exe -c Engine/Configuration/Presets/Audit.json
# (Audit.json now: Host + isOffscreen:true + isHeadless:false + initialScene UnitTest + totalFrames 35 + triggerAtFrame 30)
# Dumps land in Bin/audit_*.hdr at frame 30, then the engine exits.
```

Decode with `local://rgbe_parse.py` + `local://inspect_fresh.py` (custom RGBE
parser; imageio's HDR plugin mis-decodes as uint8).

### Investigation pointers

- Break in the OpaquePass `RecordPass` / draw-call submission; confirm draw count > 0
  and the RTV is bound. Use the debugger (`native-debugger` skill) — do NOT
  speculatively add logs.
- Check the OpaquePass node in `Data/ExampleProject/RenderGraph/ExampleRenderGraph.json`:
  are the `DrawModelGroups` dispatch and the OM RT bindings present and correct?
- Diff the OpaquePass JSON node against the pre-swap imperative OpaquePass (git
  history around `df40414a` / TASK-227 phases).
- The shutdown-ordering `std::terminate` from `s_BinaryLoaderThread`'s atexit
  destructor after AuditDumpService's `std::exit(0)` is benign (exit status 0,
  HDRs already written) but should be cleaned up — the binary-loader thread
  should be joined in TextureResourceService::Terminate before exit, or the
  audit dump should signal a clean shutdown instead of `std::exit`.

## Acceptance Criteria

- [ ] #1 OpaquePass GBuffer (RT0 BaseColor) renders the UnitTest scene geometry —
      fresh `audit_00a_OpaquePass_RT_0_BaseColor.hdr` shows > 50% nonzero with
      mean comparable to the 2026-06-01 baseline (~1.8).
- [ ] #2 Downstream passes (Sky, LightPass, TAA, FinalBlend) recover non-trivial
      content once the GBuffer is populated.
- [ ] #3 Root cause identified via debugger (not speculation) and documented.
- [ ] #4 Build green; TestSuite no new fails.

## Definition of Done

- [ ] #1 Code compiles
- [ ] #2 Pre-existing integration tests re-run green
- [ ] #3 User-observable outcome verified — fresh audit HDRs show populated GBuffer
- [ ] #4 Final summary lists what was NOT verified
