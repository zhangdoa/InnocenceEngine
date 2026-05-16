# TASK-226.4 — ReprojectScreenProbes port audit

Paper: GI-1.0 (Boissé et al., AMD, 2022) §2.1.1, `Build/GI1_0.pdf`.
Reference impl: Capsaicin master @ `src/core/src/render_techniques/gi1/gi1.comp`, function `ReprojectScreenProbes` (snapshot at `.alignments/_audit_refs/gi1.comp:293-499`).
In-house impl: `Source/Shaders/HLSL/RadianceCacheReprojection.comp` post-eca79947 (side-cache nuke, LDS-backup pending).
Audit date: 2026-05-16.

Bias: divergence-first.

## Design call recorded

Per TASK-226.4 description and gap-matrix row #2 / open question:

> TASK-226.4's "nuke side cache" assumes the LDS radiance-backup fully replaces the side cache. Capsaicin doesn't have a side cache, so this IS the paper-faithful position. If LDS backup turns out visibly inferior on long disocclusions during impl, the side cache STAYS nuked — avoid keeping a Plan-B redundancy that re-introduces the divergence.

The first half of TASK-226.4 (commit eca79947) already nuked the side cache. This audit examines whether the LDS-backup port (second half) can structurally replace it.

## Alignment table

| # | Decision | Paper / Capsaicin | Engine impl | Status |
|---|---|---|---|---|
| 1 | Per-probe-cell dispatch (8×8 numthreads, one group per probe) | `[numthreads(8, 8, 1)]`, group = one 8×8 probe tile, `local_index` 0-63 = cell index (gi1.comp:292) | `[numthreads(8, 8, 1)]`, dispatch `(W/8, H/8, 1)`, `groupIndex` 0-63 = cell index | FAITHFUL |
| 2 | Probe-cell semantic = octahedral hemisphere direction | `mapToHemiOctahedron((cell + 0.5) / probe_size)` (gi1.comp:379) and per-cell direction CDF | Atlas write via `GetAtlasTextureCoordinates(probeScreenPos, sampleDir) = probeScreenPos + EncodeOctahedral(sampleDir) * 8` (`RadianceCacheRayGen.hlsl:172`, `RayTracingTypes.hlsl:116-120`) | FAITHFUL (semantic match) |
| 3 | Winner selection (best previous probe per group) | `lds_ScreenProbes_Reprojection[probe_segment]` packed `f32tof16(score) << 16 \| local_index`, `InterlockedMin`, score = `distance(probe_pos, world_pos) / cell_size` (gi1.comp:344-348) | `sharedReprojectionScore` packed `(distScore << 16) \| input.groupIndex`, `InterlockedMin`, score = `saturate(dist / cellSize) * 65535` (`RadianceCacheReprojection.comp:163-167`) | FAITHFUL in shape; engine uses 16-bit scaled-uint score, Capsaicin uses `f32tof16`. Minor precision divergence, equivalent ordering. |
| 4 | Per-cell history-radiance accumulation (each cell receives contributions from `remap_cell_index` in 3×3 cached probes) | Per-thread `InterlockedAdd(lds_ScreenProbes_RadianceValues[(remap_cell_index << 2) + k], quantized.k)` with `remap_cell_index = mapToHemiOctahedronInverse(reprojected_dir)` (gi1.comp:393-402, 780-842) | NONE — engine copies `prev_atlas[prevScreenCoord_Probe + (i,j)]` directly to `current_atlas[screenCoord_Probe + (i,j)]` (lines 211-224), keeping (i,j) identity rather than reprojecting the direction | **DIVERGENT** |
| 5 | LDS-radiance-backup seed (per cell: `(recovered_radiance.rgb, has_sample?1:0)`) | gi1.comp:407-411 | absent | DIVERGENT (degenerate target under #4) |
| 6 | Hillis-Steele up-sweep parallel scan, stride doubling 1→2→4→8→16→32 | gi1.comp:414-419 | absent | DIVERGENT (degenerate target under #4) |
| 7 | Backup finalize: `radiance = total.xyz / max(total.w, 1)`, `empty_count = 64 - total.w`, `backup[0] = (radiance / max(empty_count, 1), MAX_HIT_DISTANCE)` | gi1.comp:421-428 | absent | DIVERGENT (degenerate target under #4) |
| 8 | Per-cell unvisited write: `if (sample_count > 0) radiance /= sample_count; else radiance = backup[0]` | gi1.comp:487-496 | Current engine: whole-probe failure → `else if (groupIndex == 0)` clears all 64 cells + writes `PROBE_MASK_INVALID` (lines 226-237) | DIVERGENT in mechanism. Engine fail-mode is whole-probe-binary; Capsaicin fail-mode is per-cell. |
| 9 | 3×3 cached-probe neighbour fallback when this probe's reprojection fails | `cached_tile` loop scanning 9 neighbour probes, accumulating their remapped radiance into `lds_ScreenProbes_RadianceValues` (gi1.comp:780-861) | absent | DIVERGENT (not in TASK-226.4 scope) |
| 10 | Disocclusion mask flag (write `kGI1_InvalidId` / `PROBE_MASK_INVALID` when no reprojection found) | gi1.comp:432-461 (line 457 writes invalid mask) | line 194, line 236 (writes `PROBE_MASK_INVALID`) | FAITHFUL |
| 11 | Sky/out-of-bounds early termination preserves group barrier uniformity | Capsaicin doesn't have an explicit sky early-exit; `is_sky_pixel` checked at the InterlockedMin guard, all threads still participate in barriers | engine uses `earlyExit` flag, all threads participate in all `GroupMemoryBarrierWithGroupSync`, branches on flag at write phase (line 130, 174, 181) | FAITHFUL (engine adapts to per-shader-standards uniformity rule, equivalent semantics) |
| 12 | `in_RadianceCacheResults_Prev` snapshot semantics — what's in the previous-frame atlas | `g_ScreenProbes_PreviousProbeBuffer` = post-temporal-filter result of frame N-1 (FilterScreenProbes writes back to it) | `m_RadianceCache_Even/Odd` ping-pong: at frame N start, the "Previous" slot holds the post-filter-V output of frame N-1 (`ExampleRenderingClient.cpp` order: Reprojection → Raytracing → FilterH → FilterV) | FAITHFUL (precondition met) |

## Detail entries for divergences

### Row #4 — Per-cell history-radiance accumulation absent

Capsaicin's reprojection scatters previous-frame radiance into possibly-different current-frame cells. The flow (gi1.comp:367-404) is:

1. Find winner previous probe (`local_lane` from InterlockedMin).
2. Each thread (= one current-frame cell) reads `probe_radiance = g_ScreenProbes_PreviousProbeBuffer[winner_probe + cell]`.
3. Compute the world-space hit point from `(probe_pos, probe_direction, probe_radiance.w)`.
4. Re-aim the hit point as a direction from THIS pixel's world position: `reprojected_dir = (hit_point - world_pos) / reprojected_len`.
5. Map `reprojected_dir` through octahedral inverse → `remap_cell_index` (POSSIBLY different from this thread's source cell).
6. `InterlockedAdd` the quantized radiance into `lds_ScreenProbes_RadianceValues[remap_cell_index]` (NOT into this thread's own cell).

The engine's reprojection skips steps 3-6 entirely. It does:

```hlsl
// RadianceCacheReprojection.comp:211-224, post-eca79947
for (uint i = 0; i < 8; i++) {
    for (uint j = 0; j < 8; j++) {
        uint2 currentPixel = screenCoord_Probe + uint2(i, j);
        uint2 prevPixel = prevScreenCoord_Probe + uint2(i, j);
        float4 historySample = in_RadianceCacheResults_Prev[prevPixel];
        in_RadianceCacheResults[currentPixel] =
            TemporalFilter(currentSample, historySample, reprojectionConfidence);
    }
}
```

The mapping is cell-(i,j)-of-prev → cell-(i,j)-of-current, with no octahedral remapping. Since the atlas IS octahedral (RayGen.hlsl:172 + RayTracingTypes.hlsl:116-120), and the previous probe's normal ≠ current probe's normal in general, the (i,j) cell of prev encodes a DIFFERENT hemisphere direction than the (i,j) cell of current. Direct copy mixes radiance across mismatched hemispheres.

Impact: per-cell history accumulation is structurally absent in the engine. The "sample count per cell" / "visited cell" concept the LDS-backup mechanism feeds on has no analog. Engine reprojection is **whole-probe-binary**: either all 64 cells receive history (success branch, lines 203-225) or none do (fail branch, lines 226-237). There are no per-cell gaps within a successful reprojection to fill.

Resolution needed: out of TASK-226.4 scope. Either (a) port Capsaicin's full octahedral-remap accumulation pipeline as part of a new task, or (b) explicitly acknowledge the engine's whole-probe-binary design as an in-house variant and adjust the LDS-backup port accordingly (see Rows #5-#8 below).

### Rows #5, #6, #7 — LDS-backup mechanism degenerate under engine design

If we port the LDS-backup mechanism literally with the engine's current reprojection design:

- `lds_ScreenProbes_RadianceBackup[local_index]` would seed from "this cell's per-cell accumulated radiance" — but there is no per-cell accumulator in the engine; there's only the InterlockedMin winner-selection scheme. Seeding from `in_RadianceCacheResults_Prev[prevScreenCoord_Probe + (i,j)]` is what the engine already does in the success branch.
- The Hillis-Steele scan would still produce a `backup[0]` value, but as the average of `prev_atlas` cells, not as the average of cells-that-survived-octahedral-remap.
- On whole-probe failure (the case Capsaicin uses backup for), there's no winner → no `prevScreenCoord_Probe` → no source cells to seed the LDS from. Backup degenerates to `(0/0, 0/0)`.

Impact: a literal port produces NaN-or-zero on the disocclusion case the task targets (AC #5). The mechanism does not address "stale ghost cells on long disocclusions" because it has no neighbour data to fall back on.

Resolution needed: the meaningful LDS-backup port for the engine requires ALSO porting Row #9 (the 3×3 cached-probe scan) so that on whole-probe disocclusion, the LDS is seeded from neighbour probes' reprojected radiance. That's a substantial scope expansion beyond "port lines 845-857."

### Row #8 — Engine fail-mode is whole-probe-binary, not per-cell

Capsaicin's `if (sample_count > 0) ... else radiance = backup[0]` (gi1.comp:489-496) operates per-cell — different cells in the same probe take different branches. The engine's fail path (lines 226-237) operates per-probe — all 64 cells take the same branch.

Impact: even after porting LDS-backup, the per-cell sample-count signal needed to drive `if (sample_count > 0)` doesn't exist in the engine's design. We'd be forced to either (a) make all 64 cells take the backup branch when reprojection fails (= what the engine currently does, just with extra LDS arithmetic), or (b) introduce a per-cell sample-count via a new accumulation scheme (= scope expansion per Row #4).

### Row #9 — 3×3 cached-probe neighbour fallback absent

Capsaicin's "cached tile" buffer (`g_ScreenProbes_ProbeCachedTileBuffer` etc) is a separate data structure that retains spawn-tile-aged probes outside the current frame's seed positions. The engine has no equivalent; its only cross-frame radiance source is the previous-frame ping-pong atlas (`in_RadianceCacheResults_Prev`).

Impact: a paper-faithful LDS-backup port that pulls disocclusion-fill from neighbours would need a similar cross-frame neighbour-radiance source. The closest engine analog is reading `in_RadianceCacheResults_Prev[neighbour_probe_screenCoord + cell]` for 3×3 neighbour probes, which differs from Capsaicin's cached-tile buffer in that it's bound to the previous frame's spawn positions, not an aged cache.

Resolution: out of TASK-226.4 scope; would interact with TASK-226.5 (probe-mask MIP chain) which already changes the neighbour-search infrastructure.

## Not audited

- Capsaicin's SH reprojection (gi1.comp:464-479) — out of TASK-226.4 scope (covered by TASK-226.7 bent-cone SH audit).
- Empty-tile spawn redirect (gi1.comp:440-454) — engine has its own in-house Halton-redirect scheme in RayGen; gap-matrix Row #1 covers this and it's TASK-226.6 scope.
- `g_ScreenProbes_EmptyTileBuffer` flagging (gi1.comp:443-453) — engine has no equivalent, related to spawn redistribution (TASK-226.6).

## Conclusion (audit only — adjudication is main-session's job)

The LDS radiance-backup parallel reduction at gi1.comp:407-428 is **not portable to the engine's current reprojection design as a standalone unit**. The mechanism's purpose is per-cell disocclusion-fill within a probe — a problem the engine doesn't have because its reprojection is whole-probe-binary (Row #4).

A literal line-by-line port of lines 407-428 would:

- Add ~40 lines of LDS arithmetic that compute `backup[0]` from data that, under the engine's current design, is either redundant (success case: each cell already has its own valid history value to blend with) or absent (failure case: no source data exists).
- Replace lines 226-237 (whole-probe clear + invalidate) with a code path that writes `backup[0]` to all 64 cells on failure. Under the engine design, `backup[0]` on failure is `(0/0, 0/0)` because none of the source cells have radiance.
- NOT address the "disocclusion ghost cells" failure mode AC #5 targets, because that failure mode requires neighbour-probe fallback (Row #9), which is itself out of scope.

The task design call ("if LDS backup turns out visibly inferior on long disocclusions during impl, the side cache STAYS nuked — avoid keeping a Plan-B redundancy") presupposes the LDS backup is a viable disocclusion-fill mechanism under the engine's design. The audit finds it is NOT — the engine's reprojection design lacks the per-cell-sparse-history structure the LDS-backup feeds on, and a meaningful port requires also addressing Rows #4 and #9.

Three options for main-session adjudication:

**Option A — Expand TASK-226.4 scope** to include Rows #4 and #9 (full octahedral-remap accumulation + 3×3 cached-probe scan). Substantial rewrite of the entire reprojection kernel, not just the LDS-backup add. Closer to a "rewrite-from-scratch" per gap-matrix entry.

**Option B — Close TASK-226.4 with the side-cache nuke alone** and re-file the LDS-backup port as a separate task that bundles with the per-cell remap (Row #4) and neighbour fallback (Row #9). Honest about the per-design call: the engine's current reprojection design is structurally incompatible with Capsaicin's LDS-backup mechanism, and porting it requires more than just the reduction loop.

**Option C — Keep the side-cache nuke and accept the long-disocclusion regression as a known limitation** until TASK-226.5/.6 land (which change neighbour search and spawn density, possibly making a richer LDS-backup viable later). Update AC #5 to acknowledge the regression is intentional pending further port work.

Recommendation: Option B. The audit's structural finding is that the LDS-backup port is not a self-contained unit under the engine's current reprojection design — it depends on prior octahedral-remap accumulation infrastructure the engine doesn't have. Forcing a literal port now produces a degenerate mechanism that doesn't deliver the disocclusion-fill the task description targets.

## Post-implementation update — Option A landed

Main session adjudicated Option A (scope expansion: port Rows #4, #7, #8, #9 together). Implementation landed at TASK-226.4 dispatch 2026-05-16.

### Rows closed

| # | Row | Resolution |
|---|---|---|
| 4 | Per-cell octahedral-remap accumulation | PORTED. Each of the 64 threads (= one cell of the current probe) now reads the winner's previous-frame radiance at the same cell index, computes the world-space hit point, re-projects it onto the current probe's surface, encodes the new direction back to an octahedral cell, and `InterlockedAdd`s quantized RGBA + sample count into a per-cell LDS accumulator. See `Source/Shaders/HLSL/RadianceCacheReprojection.comp:176-200` (winner path) and `Source/Shaders/HLSL/common/RadianceCacheReprojection.hlsl:62-95` (`RemapHistoryCell` + `AccumulateRemappedRadiance`). |
| 5 | LDS-radiance-backup seed | PORTED. `SeedRadianceBackup` in the new header (`common/RadianceCacheReprojection.hlsl:117-126`) packs `(recoveredRGB, hasSample ? 1 : 0)` per cell after the accumulation barrier. |
| 6 | Hillis-Steele up-sweep parallel scan | PORTED. `ReduceRadianceBackup` (`common/RadianceCacheReprojection.hlsl:131-145`) runs 6 strides (1→2→4→8→16→32) with a barrier per iteration; all 64 threads participate in every barrier (shader-standards uniformity rule). |
| 7 | Backup finalize | PORTED. `FinalizeRadianceBackup` (`common/RadianceCacheReprojection.hlsl:147-155`) computes `avgRadiance / emptyCount` at slot [0] per Capsaicin gi1.comp:421-428. |
| 8 | Per-cell unvisited write | PORTED. `RadianceCacheReprojection.comp:245-260` writes per-cell: cells with samples get their averaged remap radiance; cells without get `sharedRadianceBackup[0]`. Whole-probe failure (no cell got any sample) still invalidates the probe mask via the gate at `comp:263-269`. |
| 9 | 3×3 neighbour-probe fallback | PORTED with engine-faithful divergence (see below). |

### Engine-specific design calls — divergences from Capsaicin

Each divergence is acknowledged here as required by `paper-port` skill (commit footer carries `[divergence-acknowledged]`).

**(a) Atlas convention: full-sphere octahedral of world-space directions.** The engine's `EncodeOctahedral` / `DecodeOctahedral` (`RayTracingTypes.hlsl:79-114`) is full-sphere — both hemispheres of the unit sphere map to the [0,1]² atlas quad. Capsaicin's `mapToHemiOctahedron` is hemi-octahedral and uses only the upper hemisphere of a probe's local tangent frame. The engine's `RadianceCacheRayGen.hlsl:172` writes world-space ray directions directly to the atlas, so the cell already encodes a world-space direction and no TBN(probe_normal) transform is needed during reprojection. Trade-off: half the engine atlas (below-hemisphere cells) carries no useful data; the consumer filters (`RadianceCacheFilterVertical.comp:59`) already skip such cells via `dot(direction, normal) <= 0`. The port adopts the engine convention rather than re-architecting the atlas — out of TASK-226.4 scope.

**(b) Row #9 neighbour-fallback source data.** Capsaicin's analog (gi1.comp:780-861, hosted in `SampleScreenProbes`) scans a separate age-tracked `g_ScreenProbes_ProbeCachedTileBuffer`. The engine has no such structure; the dispatch explicitly forbade reintroducing the side-cache scheme. The port reads previous-frame radiance from `in_RadianceCacheResults_Prev` at neighbour probe positions, with previous-frame probe pos/normal from `in_ProbePosition` / `in_ProbeNormal` (already bound via the existing ping-pong). Validity proxy: `prev_radiance.w > 0` distinguishes valid cells from filter-zeroed invalid probes (`RadianceCacheFilterVertical.comp:48`). This source is **locked to last-frame spawn positions, not aged across multiple frames**, so disocclusion fill quality is lower than Capsaicin's when the disoccluded region has been uncovered for many frames. Engine-faithful within the binding constraint.

**(c) Row #9 hosted in Reprojection, not Sample.** Capsaicin runs the neighbour scan inside `SampleScreenProbes`, which executes after Reprojection. The engine consolidates the fallback into Reprojection's failure branch (when InterlockedMin yielded no winner). Reasoning: the engine's `SampleScreenProbes`-equivalent doesn't exist yet (TASK-226.6 scope), and waiting for it would leave disocclusions un-filled in the interim. The fallback runs per-thread (= per cell) at gi1.comp's matching call-site shape, so the host-kernel choice is mechanical — no semantic divergence.

**(d) Winner broadcast via LDS, not re-load from depth/normal buffers.** Capsaicin (gi1.comp:369-371) re-loads the winner's depth + normal from the buffers using the winner's pixel coord. The engine port broadcasts the winner thread's already-computed `positionWS`, `normalWS`, `prevPositionWS_Probe`, `prevScreenCoord_Probe / 8` via four `groupshared` slots written by the unique winner-matching thread. Equivalent semantics, fewer texture loads.

**(e) Quantization scales.** Capsaicin's `ScreenProbes_QuantizeRadiance` definition is not in the audit snapshot. The engine port uses `RADIANCE_QUANT_SCALE = 65536.0` and `HIT_DIST_QUANT_SCALE = 65536.0` (`common/RadianceCacheReprojection.hlsl:34-36`), picked to fit the engine's HDR radiance range and `ray.TMax = 1000` hit-distance ceiling without overflow under 64 accumulations. Capsaicin may pick different scales — bit-equivalent fidelity is not a goal.

**(f) Score precision.** Engine's `(uint)(saturate(dist/cellSize) * 65535.0)` vs Capsaicin's `f32tof16(distance(...))`. Per audit Row #3 (already FAITHFUL): equivalent ordering, minor precision divergence acceptable. Unchanged in this port.

### What is NOT verified

- **AC #4 / AC #5 visual + disocclusion comparison vs TASK-226.2 baseline.** Implementer's launch budget covered build + 30-frame smoke (offscreen, no frame capture). Visual comparison + Sponza-disocclusion gate belong to TASK-226.8 (perf + visual regression). Flagged honest per `visual-validation` layer-1 (Visual Read) — smoke confirms no crashes / D3D12 errors, but I have not seen the rendered frame myself.
- **Long-disocclusion behaviour.** The Row #9 fallback reads from the previous-frame atlas which is locked to last-frame spawn positions. Over multi-frame disocclusions, the source becomes increasingly stale. Quality vs Capsaicin's aged-cache approach is untested here — peer review + TASK-226.8 own this.
- **Partial-tile earlyExit behaviour (mixed sky / non-sky pixels in same 8x8 tile).** The current code's per-thread `earlyExit` flag means sky cells in a partially-sky tile fall back to the LDS-backup average computed from non-sky cells; tile-mask invalidation only fires when thread 0 is the sky thread. This preserves pre-edit behavior in spirit but may produce soft-fill artifacts at silhouettes. Out of TASK-226.4 scope; surface, don't chase.

### Files changed

- `Source/Shaders/HLSL/RadianceCacheReprojection.comp` — kernel orchestration: winner pick (unchanged), broadcast, per-cell remap (winner path), 3×3 neighbour fallback (failure path), per-cell write. Net 240→274 lines.
- `Source/Shaders/HLSL/common/RadianceCacheReprojection.hlsl` (new) — LDS arrays, quantization, `RemapHistoryCell`, reduction helpers, confidence + cellSize helpers. 199 lines.
- `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl` — no net change (the new file took the helpers I'd briefly added here).
- No C++ pass changes — all required bindings (`in_RadianceCacheResults_Prev`, `in_ProbePosition`, `in_ProbeNormal` previous-frame ping-pong slots) were already in place per `RadianceCacheReprojectionPass.cpp:94-98`.
