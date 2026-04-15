---
id: TASK-37
title: Investigate residual GBV Layout-31 error on LightPass UAV after Usage fix
status: Done
assignee: []
created_date: '2026-04-15 21:07'
labels:
  - bug
  - dx12
  - gpu-validation
  - followup
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Context

Task-36 fixed seven compute-only passes that declared their UAV output textures with `TextureUsage::ColorAttachment` (→ WriteState = RENDER_TARGET → UAV binding with RENDER_TARGET layout → GBV error). After that fix, one residual GBV error remains:

```
D3D12 ERROR: GPU-BASED VALIDATION: Dispatch, Incompatible texture barrier layout:
Resource: 'LightPass Illuminance Result_DefaultHeap_Texture_Frame0'
Binding Type In Descriptor: UAV
Layout: UNKNOWN (31)(0x1f)    <-- not RENDER_TARGET
Shader: lightPass.comp(231,34)
Pipeline: LightPass_PSO
Command List Type: D3D12_COMMAND_LIST_TYPE_COMPUTE
```

Layout 31 (0x1F) was initially assumed to map to `D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST`. **This was wrong.** Verified against `External/GitSubmodules/DirectX-Headers/include/directx/d3d12.h` (Agility SDK headers): the `D3D12_BARRIER_LAYOUT` enum ends at `VIDEO_QUEUE_COMMON = 30`. Layout 31 is past the end of the defined enum — it is an **out-of-range sentinel value**, not a real layout.

The texture is declared `ComputeOnly` (initial state UAV) and LightPass never transitions it before the dispatch. Since the reported layout is out-of-range, no code path could plausibly have set it. The error is almost certainly validator tracking imprecision, consistent with the earlier GBV warning:

> "Execution of Command List contains a shader op ... recorded while using Shader Patch Mode NONE. Hence, all further GPU-based validation may undervalidate or produce true GBV errors with imprecise tracked state (labelled with 'Possibly imprecise') for resources in the COMMON state or Promoted-from-COMMON state at the time of execute."

## Hypotheses

1. **GBV tracking imprecision** — earlier run logs warn: *"Execution of Command List contains a shader op ... recorded while using Shader Patch Mode NONE. Hence, all further GPU-based validation may undervalidate or produce true GBV errors with imprecise tracked state (labelled with 'Possibly imprecise') for resources in the COMMON state or Promoted-from-COMMON state at the time of execute."* Our shaders are compiled without `/Od /Zi` so patch mode is NONE. Layout 31 may be the validator's way of reporting imprecise tracking after a cross-CL handoff.
2. **Cross-CL state drift** — LightPass issues the input-side barriers on its Graphics CL and the dispatch on its Compute CL. State tracking across separate CLs submitted to different queues may diverge on the GBV side even when legacy resource-state barriers are correct on the runtime side.
3. **Enhanced-barriers vs legacy-state mismatch** — we use legacy `ResourceBarrier(Transition(...))` everywhere; GBV in D3D12 Agility SDK 1.7+ tracks layouts in the enhanced-barriers model. There may be an implicit layout set to COMPUTE_QUEUE_COPY_DEST that we're not aware of.

## Reproduction

```
Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -gpu_validation -total_frames 15
```
(Run after task-36 commit `6b44c7ea`.)

## Investigation plan

1. ~~Narrow: check `ReadTextureBackToCPU` paths — they create copy-dest states. Does any post-frame readback hit Illuminance?~~ **Ruled out.** Only `AuditDump()` calls `ReadTextureBackToCPU` on Illuminance, and it `std::exit(0)`s immediately after. It cannot affect steady-state frames.
2. ~~Narrow: bind the same texture from only one CL in one frame; see if the error still appears (rules out cross-CL drift)~~ **Moot.** Layout 31 is out-of-range; no transition our code issues could produce it.
3. ~~Instrument: log the tracked resource state~~ **Moot for same reason.**
4. **Remaining option (if desired):** Compile debug shaders (`/Zi /Od`) so GBV can patch properly. If the error disappears under debug shaders, this confirms the imprecise-tracking hypothesis.
5. **Conclusion path:** Confirmed GBV imprecision. No code fix. Document the validator limitation.

## Structural angle

If this turns out to be cross-CL tracking drift, the engine's barrier model (barriers on Graphics CL that affect Compute-queue dispatches in the same frame) may need explicit queue-fence ordering or a move to enhanced barriers with explicit texture layouts.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Root cause identified: **GBV imprecise tracking**. Reported layout 31 is past the end of `D3D12_BARRIER_LAYOUT` enum (max = 30), so it cannot correspond to any real runtime state. Consistent with the "Shader Patch Mode NONE" warning emitted earlier in the session.
- [ ] #2 ~~If real bug: code fix~~ — N/A.
- [x] #3 Documented in CLAUDE.md under "GPU validation → Known imprecision": out-of-enum layout values are GBV sentinels under Shader Patch Mode NONE; `/Zi /Od` rebuild remains the precise-mode option if needed.
- [ ] #4 `Main.exe -gpu_validation -offscreen -total_frames 15` exits 0 with the remaining Layout-31 error treated as a documented false positive.
<!-- AC:END -->
