---
id: TASK-65
title: >-
  Move PathTracerPayload to shared .hlsli to eliminate hand-synced struct across
  raygen/miss/closesthit
status: To Do
assignee: []
created_date: '2026-04-18 14:36'
labels:
  - refactor
  - shader
  - pathtracer
  - structural
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Structural weakness exposed by TASK-64

The `PathTracerPayload` struct is duplicated verbatim in three shader files: `GPUPathTracerRayGen.hlsl`, `GPUPathTracerMiss.hlsl`, `GPUPathTracerClosestHit.hlsl`. DXR treats the payload as one flat ABI across all three stages — if any copy diverges, writes in one stage clobber unrelated fields in another stage's view. TASK-64 was exactly this failure mode.

The current guardrail (a comment that says "keep in sync") is a social contract and decays; comments don't fail the build.

## Proposed fix

Extract the struct into `Source/Shaders/HLSL/common/pathTracerPayload.hlsli`:

```hlsl
#ifndef PATH_TRACER_PAYLOAD_HLSLI
#define PATH_TRACER_PAYLOAD_HLSLI

struct PathTracerPayload { /* fields */ };
struct ShadowPayload { bool isShadowed; };

#endif
```

`#include` it in all three shaders. Delete the three local copies.

Any future field added there automatically shows up in all stages; layout divergence becomes impossible.

## Acceptance

- One definition of `PathTracerPayload`, included by all three path tracer shaders.
- `MaxPayloadSizeInBytes` in `DX12RenderPassResourceService.cpp` still matches the struct size (pick an explicit `static_assert`-equivalent check in HLSL if the compiler supports it — otherwise document the tie).
- `gpu_path_tracer` auto-test still renders Sponza correctly.
- Three-tier regression green.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Extract PathTracerPayload + ShadowPayload to shared hlsli
- [ ] #2 All three shaders include the hlsli; no local duplicates remain
- [ ] #3 MaxPayloadSizeInBytes constant is traceable to the struct size (comment references the hlsli)
- [ ] #4 gpu_path_tracer auto-test renders Sponza geometry
- [ ] #5 RenderTest / Main 10-frame / reload all exit 0
<!-- AC:END -->
