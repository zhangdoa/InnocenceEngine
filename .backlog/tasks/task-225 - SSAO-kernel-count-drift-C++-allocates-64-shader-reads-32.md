---
id: TASK-225
title: SSAO kernel-count drift — C++ allocates 64 samples, shader reads only 32
status: To Do
assignee: []
created_date: '2026-05-14'
labels:
  - rendering
  - bug
  - constants
  - ssao
  - drift
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

The SSAO kernel-size constant is duplicated between C++ and HLSL with **two different values**. C++ allocates and uploads a 64-element Vec4 kernel buffer; the shader iterates only the first 32 entries. The bottom half of the kernel is dead GPU memory and effective AO sample count is half of what the C++ side declares.

This is the same drift class as TASK-127 (RadianceCache SH_TILE_SIZE 2/3 mismatch — silent half-truncation of the output).

## Sites

- C++ `Source/ExampleProject/RenderingClient/SSAOPass.h:28` — `uint32_t m_kernelSize = 64;`
- C++ `Source/ExampleProject/RenderingClient/SSAOPass.cpp:107,109,114,127` — kernel buffer allocated, populated, uploaded at 64 entries.
- HLSL `Source/Shaders/HLSL/SSAONoisePass.comp:32` — `static const int sampleCount = 32;`
- HLSL `Source/Shaders/HLSL/SSAONoisePass.comp:48` — `float4 SSAO_Kernels[64]` (cbuffer-sized to match C++; loop bound disagrees).
- HLSL `Source/Shaders/HLSL/SSAONoisePass.comp:125` — `for (int i = 0; i < sampleCount; ++i)` (reads only first 32 of 64).
- HLSL `Source/Shaders/HLSL/SSAONoisePass.comp:169` — `occlusion = 1.0f - (occlusion / float(sampleCount));` (running mean divides by 32, matching loop bound).

## Why it is silent

- The cbuffer array size matches C++ allocation (64), so no binding error.
- The loop bound (32) is smaller than the array size (32 < 64), so HLSL doesn't OOB.
- The C++ side runs `m_kernelSize = 64` random sample generation per cold setup once; cost is invisible.
- AO output looks plausible (some occlusion, gradient roughly right) — just under-sampled. Noise pattern would be visibly noisier than a 64-sample SSAO baseline.

## Likely root-cause

A historical bisect of `SSAOPass.h`/`SSAONoisePass.comp` would reveal one side was changed (likely from 32→64 to upgrade quality) without updating the other. Effective behaviour is "what the shader reads", i.e. 32 — the C++ upgrade is a no-op.

## Fix

Out of scope for TASK-136 (which is audit-only). Consolidate per TASK-136 AC #3: a single `SSAOConstants.h` declaring `KERNEL_SIZE` (mirroring the canonical HLSL value), consumed by both C++ (kernel buffer allocation) and the shader (loop bound via a shader header or compile-time define). Pick the intended value:

- If 64 is the intent: update HLSL `sampleCount` and the cbuffer array to 64. Re-test visual quality.
- If 32 is the intent: update C++ `m_kernelSize` to 32 and trim the cbuffer footprint. Re-test.

Per TASK-136 constraint: "Behavior-preserving consolidation only" — but here the C++ and HLSL behaviors already diverge, so the consolidation **must** pick a target value and verify the resulting output against a captured baseline. The fix CL files the visual comparison.

## Repro

Run any scene with SSAO active and observe AO term in the lit output. The drift is silent (no log, no crash), so direct visual repro is "AO is noisier than 64 samples would produce." A RenderDoc capture inspecting the bound `SSAO_Kernels` cbuffer would show 64 entries populated but only the first 32 sampled.

## Screenshot

Screenshot deferred to consolidation CL (audit dispatch did not run the engine).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Decide intent: 64 or 32 samples. Justify pick (perf budget, visual quality target).
- [ ] #2 `SSAOConstants.h` declares the canonical value with HLSL-anchor comment; both `SSAOPass.cpp` and `SSAONoisePass.comp` read it (via include or define-injection).
- [ ] #3 `SSAOPass.h::m_kernelSize` removed; the only knob lives in the constants header.
- [ ] #4 Pre/post visual comparison captured at the chosen value: GISponza or UnitTest scene, AO term isolated (DebugViewMode if available, otherwise the AO RT).
- [ ] #5 No remaining `sampleCount` / `m_kernelSize` literal in either subsystem file.
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Engine compiles (shader + C++) — build output quoted in the closure note.
- [ ] #2 GISponza captured at -total_frames 30 pre/post with AO term visible — both screenshots attached or path-linked.
- [ ] #3 Final summary lists what was NOT verified — honestly and specifically.
<!-- DOD:END -->
