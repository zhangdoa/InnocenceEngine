---
id: TASK-74
title: >-
  Path tracer: indirect desaturates toward white on long accumulation
  (curtain-area specific)
status: To Do
assignee: []
created_date: '2026-04-18 19:04'
labels:
  - path-tracer
  - rendering
  - bug
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

Interactively in GISponza, the first few path-tracer frames after a camera move show clear colors (direct lighting dominates). As the accumulator runs longer, the image washes toward white-ish / desaturated — especially curtains.

## Findings so far (this session)

**Debug albedo-only output** (temp code in RayGen that writes `payload.albedo` at the primary hit): stone walls / floor show their masonry textures correctly (bindless texture sampling is working for those materials), but the curtain area renders near-black, NOT red/green/blue. So the issue is curtain-specific and upstream of BRDF/lighting.

**Attempted fixes that didn't move the needle** (reverted, not committed):
- `pDiffuse` clamp to [0.1, 0.9] — actually made it worse by forcing specular (white F0=0.04) on dielectrics
- Per-bounce throughput clamp to 2.0 — numbers unchanged; fireflies aren't the dominant cause

**Real bug found & fixed**: metallic-roughness texture channels. glTF packs metalness in `.b` and roughness in `.g`; path tracer was sampling `.r` for both. Material files store scalar fallback `Metallic=1 Roughness=1 Albedo=(1,1,1)` because glTF defaults to those when factors aren't specified — the texture is supposed to override per-pixel. With `.r` sampling it read occlusion/zero → every pixel still read "fully metallic fully rough." Fixed to `.b` / `.g`. Committed separately.

## Remaining hypotheses (ranked)

1. **sRGB view-format mismatch for BC1 albedo.** AssimpTextureProcessor marks BASE_COLOR as `IsSRGB=true` and compresses to BC1. If `TextureResourceService` creates the SRV as `BC1_UNORM` (linear) instead of `BC1_UNORM_SRGB`, the shader reads gamma-encoded values as linear → dark. This would explain why the debug albedo view shows curtains dark — the raw sampled value is e.g. (0.2, 0.05, 0.05) in linear space, which is approximately (0.5, 0.2, 0.2) sRGB. Stonework happens to be mostly desaturated grey so the gamma error isn't as visible there.
2. **Curtain material's `TextureIndices[1]` points at the wrong texture.** Earlier we found that the empty-name collapse caused Sponza curtains to share one asset; that's fixed post-import, but maybe a stale handle or lookup issue remains. Diagnostic would be: in `GPUPathTracerPass::RebuildGeometryBuffers`, log the resolved SRV index per curtain material and verify against the expected red/green/blue BaseColor texture's index.
3. **Indirect throughput concentration at grazing specular angles.** For metallic surfaces at grazing, Fresnel→1 (white); repeated grazing bounces off rough metal carry white throughput forward. The metallic-default bug amplifies this because every non-textured surface registers as metal. Partly mitigated by the MR-channel fix, but worth re-measuring convergence after.

## Verification after each fix

- Debug-albedo shader path should show curtains in red/green/blue (not black).
- 30/100/300-frame convergence means should stabilise and not show a channel-imbalance drift.
- Direct comparison against the rasterizer (same angle, cleaner lighting) for the same materials.

## Out of scope

Firefly clamp / variance reduction (denoiser, MIS with balance heuristic) — separate improvements that don't address the curtain-specific desaturation.
<!-- SECTION:DESCRIPTION:END -->
