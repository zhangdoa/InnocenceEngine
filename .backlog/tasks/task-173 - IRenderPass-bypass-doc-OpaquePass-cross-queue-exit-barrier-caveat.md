---
id: TASK-173
title: 'IRenderPass bypass doc: OpaquePass cross-queue exit-barrier caveat (TASK-171 ADVISORY)'
status: Done
assignee: []
created_date: '2026-04-28 07:30'
labels:
  - rendering
  - documentation
dependencies: []
priority: low
references:
  - Source/Engine/Interface/IRenderPass.h
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-171 review #2 ADVISORY (graphics-api-expert peer)**, 2026-04-28.

The `IRenderPass.h` bypass-contract comment doesn't document the interaction with TASK-161's `m_PostCLState = CrossQueueExit::ToCommon` flag on OpaquePass. Bypassing OpaquePass elides its cross-queue exit barrier; downstream compute consumers see the GBuffer in implicit-promoted COMMON state.

**This is GPU-safe** under DX12 implicit-promotion-from-COMMON semantics (verified by reviewer + iteration 2 flag-flip test — no GBV ERROR), **but** the safety predicate ("OpaquePass must have run at least once before being bypassed; first-frame bypass leaves GBuffer at INITIAL state, also COMMON, so still safe by implicit promotion") is not stated in the contract. A future bypass user might not know to think about it.

### Required fix

Add a one-paragraph caveat to `IRenderPass.h:32-46` (the bypass-contract comment block), documenting:

- Bypass elides barrier emission for the bypassed pass.
- Specifically: `m_PostCLState = CrossQueueExit::ToCommon` (TASK-161) is not emitted when the producing pass is bypassed.
- Safety: DX12 implicit-promotion-from-COMMON makes this work for the typical "bypass during normal operation" pattern. First-frame bypass on a CrossQueueExit-flagged pass also works because INITIAL_STATE is implicitly COMMON.
- Caveat: if a future render pass uses a non-COMMON exit state, bypassing it would leave the resource in an indeterminate state for cross-queue consumers.

### Why low priority

Documentation only. No correctness change. The current shipped behaviour is GPU-safe; this just makes the safety predicate auditable for future contributors.

### Owner

`rendering-researcher` (header comment in render-pass interface).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 IRenderPass.h bypass-contract block extended with the cross-queue exit-barrier caveat
- [x] #2 No code change; comment-only CL
- [x] #3 Reviewed-By: any peer (small CL — could even be Review-Skipped: docs-only since the change is non-closing pure documentation)
<!-- AC:END -->

## Implementation Notes

Added a `Cross-queue exit-barrier caveat (TASK-161 / TASK-173)` paragraph inside the `m_Bypassed` comment block in `Source/Engine/Interface/IRenderPass.h` (lines 46-57). The paragraph states:

- Bypass elides every barrier the pass would have emitted, including `m_PostCLState = CrossQueueExit::ToCommon`.
- Safe today via DX12 implicit-promotion-from-COMMON (steady-state and first-frame both land on COMMON).
- Forward contract: a future CrossQueueExit pass that adopts a non-COMMON exit state must re-evaluate or gate the bypass before introducing the pass.

Build (RelWithDebInfo) succeeded — header-comment-only edit, no translation-unit churn observed beyond the standard rebuild.
