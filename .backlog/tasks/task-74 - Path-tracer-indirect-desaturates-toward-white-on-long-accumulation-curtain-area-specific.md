---
id: TASK-74
title: >-
  Path tracer: indirect desaturates toward white on long accumulation
  (curtain-area specific)
status: Done
assignee: []
created_date: '2026-04-18 19:04'
updated_date: '2026-04-18 19:23'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Root-caused: `GPUPathTracerPass::RebuildGeometryBuffers` was a one-shot — ran when the scene loaded, resolved material `TextureIndices` from their names, wrote them into the material CB. But the textures it tried to resolve were still in the deferred-init queue at that moment (`TextureResourceService::Initialize` enqueues; `InitializeComponents` processes NEXT frame). Activated-guard skipped them → every albedo slot stayed INVALID → shader fell back to the scalar (1,1,1) that glTF assigns when baseColorFactor is absent → every Sponza surface untextured white. Compounded by a separate real bug I found en route (ClosestHit was reading metallic-roughness from `.r` instead of `.b`/`.g` — fixed in 61a19d9b) which kept the scalar metalness=1 even when the MR texture eventually resolved.

Fix (dd926672): re-resolve every material's texture indices in `Update()` every frame and re-upload the material CB, mirroring DrawCallService's per-frame material pattern. Any texture that finishes deferred init on frame N gets its bindless index picked up on frame N+1.

First attempt (reverted before commit) tried to gate the one-shot rebuild on an `AreTexturesGPUReady()` check. Permanently blocked because at least one Sponza texture (`col_head_2ndfloor_02_Normal`) stays stuck at ObjectStatus::Created forever — filed as a follow-up.

Verified: 30-frame GISponza now shows distinct curtain colors with the lace pattern resolved (cyan/gold variants visible) where previously the curtains were flat white. RenderTest + Main regression green. Remaining colorspace issue (curtains appearing cyan/gold rather than the expected red/green/blue from the source PNGs) is a follow-up about sRGB view format or channel ordering.
<!-- SECTION:FINAL_SUMMARY:END -->
