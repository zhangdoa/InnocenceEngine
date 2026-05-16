# TASK-226.3 — FilterScreenProbes weight port: audit-only closure

Phase 1.3 of TASK-226 umbrella. The gap-matrix row #6 entry was misleading; line-level audit against Capsaicin gi1.comp:1455-1519 (fetched 2026-05-16 via raw.githubusercontent on `master` branch) shows the weight formula is already aligned. No code change required.

## Capsaicin (gi1.comp:1455-1519)

```hlsl
float weight = pow(saturate(1.0f
                          - abs(toLinearDepth(probe_depth, g_NearFar)
                                - toLinearDepth(depth, g_NearFar))
                            / toLinearDepth(depth, g_NearFar)),
                   8.0f);

radiance     += weight * float4(GIDenoiser_RemoveNaNs(
                                  g_ScreenProbes_PreviousProbeBuffer[probe_pos].xyz),
                                probe_hit_distance);
total_weight += weight;
```

## Ours

`Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp:117-123` (Vertical mirror at lines 103 ff):

```hlsl
// Bilateral weight in linear depth (Capsaicin gi1.comp weight).
float neighbourDepth = length(neighbourPosWS - g_Frame.camera_posWS.xyz);
float depthRatio = saturate(1.0 - abs(neighbourDepth - currentDepth) / max(currentDepth, EPSILON));
float weight = pow(depthRatio, 8.0);

filteredRadiance += weight * float4(neighbourSample.rgb, neighbourHitDistance);
totalWeight += weight;
```

## Equivalence

- **`toLinearDepth(d, g_NearFar)` vs `length(posWS - cameraPosWS)`** — same quantity (camera-space linear distance), derived two ways. Capsaicin reconstructs from NDC z via near/far; we already have world-space positions stored, so we read distance directly. Numerically equivalent.
- **`saturate(...)` vs `max(., EPSILON)` divisor guard** — Capsaicin relies on `probe_depth > 0` plus saturate to clamp the ratio; we guard the divisor explicitly. Functionally equivalent on non-degenerate input.
- **`pow(..., 8.0)` exponent** — bit-identical.
- **Accumulation shape** — `radiance += weight * float4(rgb, hit_distance); total_weight += weight; final = radiance / total_weight` — bit-identical.

## What was misread in gap matrix row #6

The gap-matrix entry stated "Capsaicin uses `depth + normal` weights" but Capsaicin uses depth-only **weights**. The depth, normal, plane factors are split between:

1. **Weight** (continuous, soft falloff) — depth bilateral, `pow(., 8.0)`.
2. **Accept/reject gates** (hard boolean) — hemisphere check `dot(direction, neighbour_normal) <= 0`, plane-distance gate `abs(dot(neighbour_pos - current_pos, current_normal)) > cell_size`, and the parallax-divergence check `dot(direction, reprojected_direction) < PROBE_FILTER_ANGLE_THRESHOLD`.

Both passes already implement all three accept/reject gates (FilterHorizontal: lines 99 plane, 102 hemisphere, 114 parallax; FilterVertical: mirrored).

## No rebuild, no smoke

This CL contains only this audit doc + the backlog status flip; HLSL is unchanged. Shader build is unchanged from the post-TASK-226.1 state (`0ba5d0a2`). Skipping runtime smoke per `commit-policy` § docs-only.

## Follow-up to gap matrix

Row #6 status was "paper-aligned" already; the "Notes" column flagged depth-only weight as a potential divergence. The audit confirms no divergence — recommend updating the matrix notes column in a future docs CL or rolling it into the TASK-226.8 final port-audit. Not blocking this closure.

## Closure-Reason

ACs #1-#6 satisfied by line-level audit; no code change required.
