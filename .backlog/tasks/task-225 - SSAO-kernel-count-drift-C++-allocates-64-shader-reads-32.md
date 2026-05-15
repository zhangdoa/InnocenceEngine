---
id: TASK-225
title: SSAO kernel-count drift — C++ allocates 64 samples, shader reads only 32
status: Done
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
- [x] #1 Decide intent: 64 or 32 samples. Picked 32 — zero visual change, current effective AO sample count stays at 32 (shader-side `sampleCount`). Conservative drift-fix; expansion to 64 left as a separate quality-uplift CL gated on user eye-test.
- [ ] #2 `SSAOConstants.h` declares the canonical value with HLSL-anchor comment; both `SSAOPass.cpp` and `SSAONoisePass.comp` read it (via include or define-injection). Deferred — this CL does the minimal drift-fix only; full constants-header consolidation belongs under TASK-136.
- [ ] #3 `SSAOPass.h::m_kernelSize` removed; the only knob lives in the constants header. Deferred with AC#2.
- [x] #4 No visual regression at -total_frames 30 offscreen: with pick-32 the C++ kernel buffer now contributes the same first-32 entries the shader already read. Bit-identical AO output by construction (the post-32 entries were dead memory). Engine ran clean to `Engine has been terminated.`
- [x] #5 No remaining bare `64` literal in `SSAOPass.cpp`; all kernel-size sites go through `m_kernelSize`. HLSL cbuffer-array dimension `float4 SSAO_Kernels[64]` left at 64 (untouched per dispatch brief).
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Engine compiles (shader + C++) — `Main.vcxproj -> C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo\Main.exe`.
- [x] #2 `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` terminated cleanly (`Engine has been terminated.`). Pre/post visual capture not required — pick-32 is bit-identical to pre-fix by construction (shader already only read the first 32 entries; the latter half was dead memory).
- [x] #3 Final summary lists what was NOT verified: (a) GISponza scene visual capture skipped — bit-identical-by-construction reasoning above; (b) HLSL cbuffer-array binding-size mismatch surfaced below as a known follow-up.
<!-- DOD:END -->

## Implementation Notes

- `SSAOPass.h:28-29` — `m_kernelSize` flipped 64 → 32 with an anchor comment pointing at `SSAONoisePass.comp` `sampleCount`. `SSAOPass.cpp` already routed all size-dependent sites through `m_kernelSize`; no bare-64 literals existed there, no further edits required.
- HLSL `SSAONoisePass.comp` untouched per the dispatch brief. Canonical `sampleCount = 32` (line 32) and cbuffer-array dimension `float4 SSAO_Kernels[64]` (line 48) preserved.
- Follow-up surfaced (not folded in): C++ now uploads a 32-element (512-byte) buffer into a cbuffer slot whose HLSL declaration `float4 SSAO_Kernels[64]` reserves 1024 bytes. The shader only reads indices [0..31], so no OOB; runtime ran clean with the D3D12 debug layer disabled. With `-gpu_validation` enabled the validator may flag the under-bound cbuffer. The natural follow-up is the TASK-136-style constants-header consolidation (AC#2/#3) where the HLSL cbuffer-array dimension also moves to 32.

## Closure summary

- Decision: pick 32 (conservative drift-fix per dispatch brief).
- C++: `SSAOPass.h` `m_kernelSize = 64;` → `m_kernelSize = 32;` with HLSL anchor comment. `SSAOPass.cpp` unchanged (no bare 64 literals).
- HLSL: untouched.
- Build: green (`Main.exe` rebuilt at RelWithDebInfo).
- Runtime: `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` → `Engine has been terminated.` No DX12 errors related to the SSAO_Kernel buffer in the log.
- Visual: bit-identical to pre-fix by construction (shader read-window unchanged at [0..31]; the trimmed entries were dead memory).
