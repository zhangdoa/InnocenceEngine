---
id: TASK-38
title: >-
  Make TextureUsage → WriteState mapping explicit and fail loud for
  compute/graphics mismatch
status: Done
assignee: []
created_date: ''
updated_date: '2026-04-18 11:53'
labels: []
dependencies: []
priority: medium
---

## Context

TASK-36 uncovered 7 textures across 5 compute-only passes that were declared with `TextureUsage::ColorAttachment` while being bound as UAVs in compute dispatches. `DX12Helper::GetTextureWriteState` silently mapped `ColorAttachment → D3D12_RESOURCE_STATE_RENDER_TARGET`, producing RENDER_TARGET-layouted resources that compute UAV binding then rejected.

The bug was invisible in normal runs — only GPU-based validation surfaced it. That means the contract between a pass's engine type (Compute/Graphics) and its output textures' `Usage` was implicit and unenforced.

## Structural retrospective (per CLAUDE.md)

1. **Violated implicit contract**: "A compute-only pass's UAV output textures must use `TextureUsage::ComputeOnly`, not `ColorAttachment`." Nothing in the type system or runtime enforced it.
2. **Structural weakness**: `TextureUsage` conflates binding intent (RTV vs UAV) with lifecycle/layout policy. A single misdeclaration produces wrong barriers three layers away, with no error at the declaration site.
3. **Improvement direction**: the pass's engine type and the texture's Usage are co-dependent — make the relationship explicit. Either derive WriteState from how the texture is *bound* (not how it is declared), or validate Usage against the owning pass's `GPUEngineType` at `TextureResourceService::Initialize`.

## Acceptance criteria

- A compute pass binding a texture as UAV output with `Usage == ColorAttachment` produces an immediate error (assertion, log + fail-closed, or compile-time rejection) — not silent wrong barriers.
- Either: (a) `GetTextureWriteState` takes binding context rather than `Usage` alone, OR (b) `RenderPassComponent` initialization validates each output texture's Usage matches its pass's engine type.
- Existing passes continue to work without regressions — full integration test + GBV-enabled run both clean.
- Documented invariant in code (comment at `TextureUsage` enum or `GetTextureWriteState`) describing the contract.

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Landed option (b) from the task's fix-direction list: validate at `RenderPassResourceService::InitializeOutputMergerTargets` (commit f7fa67ff). When the pass's `GPUEngineType` and its `RenderTargetDesc.Usage` disagree:

- Compute + ColorAttachment → error + refuse init (the exact silent-RENDER_TARGET pattern from TASK-36)
- Graphics + ComputeOnly → error + refuse init (the symmetric case)

Scope guarded to passes that actually have color outputs and no custom `m_RenderTargetsInitializationFunc`, so passes with their own init take responsibility for the invariant themselves instead of tripping a false positive.

Invariant documented inline in the check; full RenderTest + scene reload + Main 10-frame integration all exit 0 with no existing pass tripping the assertion (the initial BRDFLUTPass false-positive hit was the scope guard missing; fixed before landing).

Did not expand `TextureUsage` with new variants per the task's guidance. The WriteState mapping in `DX12Helper_Texture.cpp` still reads `Usage` directly; now it's safe because the invariant is enforced at init time.
<!-- SECTION:FINAL_SUMMARY:END -->

## Notes

- See TASK-36 for the seven sites already corrected and the GBV error pattern.
- Do not expand `TextureUsage` with new variants before deciding whether the enum itself is the right abstraction.
