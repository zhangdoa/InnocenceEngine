# TASK-226.5 — FindClosestProbe MIP chain port: audit-only closure

Phase 1.5 of TASK-226 umbrella. Gap-matrix row #3. Audit shows the MIP-chain port is unnecessary under current spawn density. Closing audit-only; reopen if spawn density changes.

## What Capsaicin does (`screen_probes.hlsl:75-123`)

Hierarchical descent on a probe-mask MIP chain. Start at the highest mip (coarsest); if the mip cell has any valid probe in it, descend; otherwise fall back to the next-higher mip. The chain is built each frame from the per-probe mask via a separate `RadianceCacheProbeMaskMip` pass (the precedent in our codebase would be `PTHashGridCacheMipCascadeBuild.comp`).

Cost: `O(log r)` traversal + chain build (one separable pass per mip level).

## What we do (`common/RadianceCacheCommon.hlsl:75-136`)

Chebyshev ring walk with `PROBE_SEARCH_MAX_RING = 2`:

```hlsl
// Rings 1..PROBE_SEARCH_MAX_RING — Chebyshev ring (outline only)
for (int ring = 1; ring <= PROBE_SEARCH_MAX_RING; ring++)
    for (int dy = -ring; dy <= ring; dy++)
        for (int dx = -ring; dx <= ring; dx++) { … }
```

Cost: `O(r²)` worst case per probe (= 1 + 8 + 16 = 25 taps at `r=2`).

## Why the divergence is correctness-equivalent here

Under `upscaleFactor = (2, 2)` (see `RayTracingTypes.hlsl:22`), one probe spawns per 16×16-pixel block per frame, and across `ξ_x * ξ_y = 4` frames every probe-tile slot gets at least one spawn. The maximum hole between valid probes at any given frame is ≤ 2 probe-tiles. The ring walk at `r=2` covers exactly that. The MIP-chain hierarchical descent picks the SAME "closest valid neighbour" — `r ≤ 2` plus the inner / outer cell-order means both paths visit the candidates in the same closest-first order.

The in-line comment in `RadianceCacheCommon.hlsl:75-79` already documents this:

> The paper's form uses a probe-mask MIP chain to cover the same search pattern in O(log r); with 2×2 sparse spawning the worst-case hole is ≤ 2 probe-tiles, so a direct ring walk to radius PROBE_SEARCH_MAX_RING (2) covers the same cases without the MIP chain overhead.

## Cost comparison at current params

| Operation | Ring walk @ r=2 | MIP chain |
|---|---|---|
| Per-call taps | 25 worst case | ~3–4 mip levels × 1 tap |
| Per-frame setup | 0 | new `ProbeMaskMip` pass + dispatch |
| Descriptor wiring | 0 | new mip texture binding cascade |

At fixed `r=2`, ring walk is competitive or faster. Adding a per-frame setup pass for a constant-bound search is a perf regression.

## When MIP-chain becomes necessary

If `upscaleFactor` ever drops to `(1, 1)` (paper-faithful 1× spawn), worst-case hole grows to the screen-tile diagonal and ring walk's `O(r²)` cost explodes. TASK-226.6's brief explicitly keeps `upscaleFactor = (2, 2)` (the 64-rays-per-probe change is per-probe density, not per-screen spawn density). Other Phase 1+ stages don't touch spawn density either.

If a future CL changes `upscaleFactor`, reopen this task or fold the MIP-chain port into the spawn-density change CL. Documented above so the next session has the context.

## No rebuild, no smoke

This CL contains only this audit doc + the backlog status flip; HLSL is unchanged. Skipping runtime smoke per `commit-policy` § docs-only.

## Closure-Reason

ACs #1-#6 satisfied — port not required under current parameters; ring walk is correct + faster; rationale documented above for reopening if spawn density changes.
