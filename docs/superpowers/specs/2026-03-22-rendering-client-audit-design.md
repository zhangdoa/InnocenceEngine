# Default Rendering Client Audit — Design Spec
**Date:** 2026-03-22
**Status:** Approved

## Problem

Main.exe shows a black screen and spams `FENCE_ZERO_WAIT` warnings. The default rendering client has never been systematically validated after the ECS overhaul. Each pass needs both a clean D3D12 log and correct visual output confirmed by image readback.

## Scope

**In scope:** All 17 active passes from BRDF LUTs through Final Blend.
**Deferred:** Radiance Cache passes (7–11) — DXR ray tracing system, separate effort.

## Mechanism: Audit Dump (`-audit` flag)

Add a one-shot dump path to `DefaultRenderingClient` triggered by a `-audit` command-line flag parsed in `Engine::ParseInitConfig`. On frame 5 (after scene load settles), call `ReadTextureBackToCPU` on each active pass's primary output render target and save as HDR to `Bin/`. The app continues normally after the dump.

Filenames encode pass order and slot: `audit_01_BRDFLUTPass.hdr`, `audit_04a_OpaquePass_albedo.hdr`, etc.

## Pass Inventory and Outputs

| # | Pass | Output(s) to dump |
|---|------|-------------------|
| 1 | BRDFLUTPass | BRDF LUT |
| 2 | BRDFLUTMSPass | BRDF MS LUT |
| 3 | SunShadowGeometryProcessPass | Shadow depth map |
| 4 | OpaquePass | Albedo, Normal, MRA, Depth |
| 5 | SSAOPass | AO buffer |
| 6 | TiledFrustumGenerationPass | Frustum debug |
| 7 | LightCullingPass | Light index list |
| 8 | LightPass | HDR color |
| 9 | SkyPass | HDR color (post-sky) |
| 10 | TAAPass | Resolved color |
| 11 | FinalBlendPass | Final SDR output |

## Per-Pass Workflow

For each pass in order:
1. Build and run with `-audit`
2. Read and interpret HDR dump(s)
3. If GPU log clean and image meaningful → advance to next pass
4. If broken → systematic debugging, fix, rebuild, re-dump, confirm before advancing

`FENCE_ZERO_WAIT` warnings investigated and fixed as the responsible pass is reached.

## Success Criteria

A pass is done when:
- No D3D12 errors or unexpected warnings during its execution
- Output image is visually correct for what the pass should produce (not all-black, not garbage)

## Non-Goals

- Radiance Cache / DXR raytracing (deferred)
- Disabled passes (VXGI, volumetric, transparency, animation)
- Performance optimisation
