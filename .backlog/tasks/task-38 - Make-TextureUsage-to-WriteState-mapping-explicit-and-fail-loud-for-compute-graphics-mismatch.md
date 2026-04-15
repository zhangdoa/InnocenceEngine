---
id: task-38
title: Make TextureUsage → WriteState mapping explicit and fail loud for compute/graphics mismatch
status: Todo
priority: medium
type: structural
created: 2026-04-15
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

## Notes

- See TASK-36 for the seven sites already corrected and the GBV error pattern.
- Do not expand `TextureUsage` with new variants before deciding whether the enum itself is the right abstraction.
