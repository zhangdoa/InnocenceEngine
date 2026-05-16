# TASK-226.1 — Nuke rasterizer-side WorldTileGrid: closure summary

Phase 1.1 of TASK-226 umbrella. Gap-matrix row #8 — rasterizer-side world cache used surface-normal direction key, Capsaicin uses incoming-ray-direction (TASK-77.1 audit flagged same class). PT-side hash grid under TASK-77.x is canonical; the duplicate is removed.

## Diff stats

12 files changed (10 modified, 1 new, 1 task), 36 insertions(+) / 552 deletions(-) on the source delta vs HEAD prior to this CL.

The nuke landed on top of two file-size-gate-required reductions in the same CL: RayGen helpers extracted into `RadianceCacheRayGen_HemisphereCDF.hlsli` (182 lines) so the kernel file drops to 184 lines; Reprojection.comp comments compressed (no semantic change) so the file lands at 300 lines.

Both file-size adjustments are transient scaffolding — TASK-226.4 (Reprojection per-probe-cell rewrite) and TASK-226.6 (RayGen 64-rays-per-probe + workgroup-parallel CDF scan) will rewrite these files; the new `_HemisphereCDF.hlsli` either gets folded back or deleted at that point.

```
.backlog/tasks/task-226.1 …                                                 |   3 +-
Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp       |  35 +++-------
Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp     |   7 ---
Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.h       |   3 --
Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass_Setup.cpp |  21 ++-----
Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl                            |  40 +-----------
Source/Shaders/HLSL/RadianceCacheRayGen.hlsl                                |  85 -------------------------
Source/Shaders/HLSL/RadianceCacheReprojection.comp                          |  12 ++--
Source/Shaders/HLSL/RayTracingBindings.hlsl                                 |   3 --
Source/Shaders/HLSL/RayTracingTypes.hlsl                                    | 183 +------------------------------
```

## Helpers KEPT vs DELETED

| Symbol | Kept / Deleted | Reason |
|---|---|---|
| `_PCG3D` | KEPT | Used by closest-hit RNG seeding paths outside the world cache. Grep verified. |
| `RayPayload` | KEPT | Shared by every ray-trace shader. |
| `CreateTangentSpace`, `ImportanceSampleGGX`, `Hash2D` | KEPT | General sampling utilities. |
| `EncodeOctahedral`, `DecodeOctahedral`, `GetAtlasTextureCoordinates` | KEPT | Used by the screen-probe atlas (kept). |
| `probeAtlasSize`, `upscaleFactor`, `spawnTileSize`, `TILE_SIZE`, `SH_TILE_SIZE` | KEPT | Screen-probe pipeline constants. |
| `Y_00`..`Y_22` SH basis | KEPT | Used by RadianceCacheIntegration (kept). |
| `DominantAxis` | DELETED | Only world-cache caller. |
| `CellInTile`, `CellIndexInTile`, `_TileGridIndex`, `_CellGridIndex` | DELETED | World-cache addressing only. |
| `SelectTileMipLevel`, `IsShortRay`, `IsTileSlotStale` | DELETED | World-cache MIP/decay helpers only. |
| `ComputeTileHash`, `ComputeTileFingerprint` | DELETED | World-cache hash only. |
| `WORLD_TILE_HASH_SIZE`, `WORLD_TILE_CELLS_*`, `WORLD_TILE_EVICTION_AGE`, `WORLD_PROBE_SHORT_RAY_THRESHOLD`, `MAX_LINEAR_PROBE` | DELETED | World-cache constants. |
| `WorldCell`, `WorldTile` structs | DELETED | World-cache types. |
| `probeSpacing` const | DELETED | Only world-cache callers; grep confirmed. |
| `in_WorldTileGrid` (u1 RWStructuredBuffer\<WorldTile\>) | DELETED | u1 binding left empty — no renumber, root-signature stays intact. |
| C++: `m_WorldProbeGrid` decl + lifecycle in `RadianceCacheReprojectionPass_Setup.cpp` | DELETED | Sole resource owner. |

## Build

`Scripts/BuildWin.ps1 -SkipClangdIndexRefresh` → all targets green:
- DXIL shaders compiled fresh (RadianceCacheClosestHit/RayGen/RayTracingTypes timestamps 2026-05-16 04:00).
- `Main.exe` linked clean.

## Runtime smoke

`Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` (CWD `Bin/RelWithDebInfo`) → log `[2026-5-16-2-1-1-537].Log`:

```
[2026-5-16-2-1-5-207] Auto-test: steady state reached at frame=4 …
[2026-5-16-2-1-5-241] Auto-test: loaded GISponza scene at frame 5.
[2026-5-16-2-1-10-920] Auto-test: steady state lost at frame=7 (instanceCount changed 56->94)
[2026-5-16-2-1-11-773] Auto-test: 30 frames rendered, terminating.
[2026-5-16-2-1-12-425] Auto-test: running CPU path tracer reference render…
Engine has been terminated.
```

Grep for `ERROR | Validation | CORRUPTION | HUNG | FATAL` in the log: zero hits.

## Dangling-reference check

`git grep -iE "worldtile|world_tile|worldprobegrid|worldcache"` across `Source/` → 0 hits. (AC #5)

## Visual verification

NOT performed in this CL. The rasterized GI will look worse short-term because the secondary-bounce cache is gone — expected, within scope. The umbrella's AC #4 (visual parity vs Capsaicin on Sponza) lives in TASK-226.8 (final gate) once stages .3–.7 have ported the screen-probe pipeline.

## What was NOT verified

- Visual delta on Sponza vs prior commit — deferred to TASK-226.8.
- Rasterizer-fallback indirect light quality with the cache gone — expected to degrade; no measurement done.
- Performance delta — `total_frames 30` is a smoke, not a perf capture. TASK-226.2 owns baseline.

## Peer review

Skipped — the diff is a single deterministic transformation (delete-the-cache-and-its-callers) with no design surface that wasn't already settled in the TASK-226 gap-matrix dispatch. Per `peer-review-required` skill § Skip categories: "Mechanical refactors with a single deterministic transformation". Resulting commit message will carry `Review-Skipped: mechanical-deletion`.
