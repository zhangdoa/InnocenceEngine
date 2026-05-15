---
id: TASK-6.1
title: 'Radiance cache [W.3b-api] Encapsulate world-tile write/read in a helper header'
status: Done
assignee: []
created_date: '2026-04-23 12:17'
updated_date: '2026-05-15'
labels:
  - superseded
dependencies: []
references:
  - Source/Shaders/HLSL/RayTracingTypes.hlsl
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl
parent_task_id: TASK-6
priority: low
---

## Closure (2026-05-15) — superseded by TASK-226

Closed as superseded by the AMD GI 1.0 reference-port effort (TASK-226). The world-tile API would mirror the reference implementation's API directly, not a helper-header abstraction over our existing struct exposure. The W.3b retrospective concern (consumers re-computing `ComputeTileHash` + `ComputeTileFingerprint` + `CellInTile` inline) survives in the new direction by being absorbed into whatever helper the reference impl uses.

If the reference impl's hash/fingerprint/cell API turns out to leave the same exposure problem this ticket flagged, file a fresh task with the reference-impl symbol as the canonical site. Don't reopen this one — its framing (helper-header for our struct) is tied to a port we're abandoning.

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Structural retrospective of the [W.3b] landing (commit 30901a29). The WorldTile struct exposes its cell pyramid directly: consumers (`RadianceCacheRayGen.hlsl`, `RadianceCacheClosestHit.hlsl`) compute `ComputeTileHash` + `ComputeTileFingerprint` + `CellInTile` + the linear-probe loop themselves, and the MIP fan-out lives inline in RayGen. Nothing enforces that a reader computes `cellXY` before indexing `cells[]` — a future consumer that writes `tile.cells[0].radiance` directly would silently get MIP0 (0,0) radiance regardless of actual hit position.

Proposed fix: introduce `common/WorldCacheCommon.hlsl` with:
- `bool WriteWorldCacheCell(pos, dir, shortRay, radiance, frameIndex)` — encapsulates hash, linear probe, reclaim-zero, MIP fan-out, fingerprint + timestamp update. Returns true on successful insert.
- `bool ReadWorldCacheCell(pos, dir, shortRay, distance, out radiance)` — encapsulates hash, linear probe, MIP selection.

Benefits:
- Preserves orthogonality — the tile hash implementation can evolve without touching consumers.
- Makes the eventual [W.3b-cache-the-index] follow-up trivial: the helper can accept an optional `(cachedSlot, cachedFingerprint)` hint and fall back to a fresh hash if it misses.
- Encodes the "clear on ownership change" and "EMA blend" invariants in one place, so a new write site can't accidentally skip them.

Not urgent — current [W.3b] ships working and there is only one writer and one reader today. File now so the API debt doesn't get forgotten when a second writer (multi-bounce, world-cache self-feedback) lands.

Parent: TASK-6 (see Implementation Notes §[W.3b]).
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
