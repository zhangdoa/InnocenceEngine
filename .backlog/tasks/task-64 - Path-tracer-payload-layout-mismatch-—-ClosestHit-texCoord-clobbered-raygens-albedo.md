---
id: TASK-64
title: >-
  Path tracer payload layout mismatch — ClosestHit texCoord clobbered raygen's
  albedo
status: Done
assignee: []
created_date: '2026-04-18 14:35'
updated_date: '2026-04-18 14:35'
labels:
  - bug
  - pathtracer
  - shader
  - regression
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Bug

User report (interactive Sponza): "path tracer is broken — the texture sampling works but funnily sampled the position G-buffer as materials."

What they saw was UV coordinates being interpreted as albedo RGB — visually indistinguishable from a position G-buffer (both are smooth gradients across surfaces).

## Root cause

Commit `5e77b493` added a `float2 texCoord` field to `PathTracerPayload` in `GPUPathTracerClosestHit.hlsl` between `normal` and `albedo`, but did NOT add the same field to the payload struct in `GPUPathTracerRayGen.hlsl` and `GPUPathTracerMiss.hlsl`. DXR payloads are one flat blob shared across all stages, so:

- ClosestHit wrote `texCoord.xy` at byte offset 24 (after hitPos+normal).
- RayGen read `albedo.rgb` at byte offset 24.
- Every `payload.albedo` in raygen was actually `(texCoord.x, texCoord.y, metalness)` — a UV gradient across the mesh instead of the material's albedo.

Symptom only became visible once the path tracer rendered geometry (with Sponza loaded); pure-sky/miss frames showed nothing wrong because miss never writes albedo.

## Fix

`Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl`: remove `texCoord` from the payload struct, make it a local. `texCoord` was only being plumbed for the TASK-19 bindless sampling TODO, but that sampling will happen inside ClosestHit itself — the payload never needs it.

`Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`: `MaxPayloadSizeInBytes` 64 → 48, matching the restored layout.

Added a structural comment on both shader payload structs warning that any field added must be mirrored across all three stages (raygen/miss/closest-hit).

## Verification

- `gpu_output.png` at frame 8 on GISponza: before = smooth UV-like gradient; after = Sponza geometry visible (dark/noisy due to low sample count, but architecturally correct).
- RenderTest: 0.
- Main 10-frame: 0.
- Reload 20-frame @10: 0.

## Structural follow-up

The three payload structs being hand-synchronized is the failure mode itself. Long-term fix (separate task if it grows): move `PathTracerPayload` to a shared `.hlsli` and `#include` it in all three shaders so the struct exists in exactly one place.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Remove texCoord from ClosestHit payload; keep it as a local
- [x] #2 Restore MaxPayloadSizeInBytes to 48
- [x] #3 gpu_path_tracer auto-test produces Sponza-shaped output (not UV gradient)
- [x] #4 Three-tier regression green (RenderTest, 10-frame, reload)
<!-- AC:END -->
