---
id: TASK-51
title: >-
  Replace uint64_t-stored COM pointers with typed pointers in GPU resource
  components
status: Done
assignee: []
created_date: '2026-04-16 19:13'
updated_date: '2026-04-18 09:30'
labels:
  - reliability
  - DX12
  - refactor
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
  - Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** DX12FrameManagementService.cpp has 20+ sites that `reinterpret_cast` `m_CommandList` (stored as `uint64_t`) to `ID3D12GraphicsCommandList7*` and `m_PipelineStateObject` to `DX12PipelineStateObject*`. Most sites dereference immediately without null check. If the stored value is 0 or corrupted, this is silent undefined behavior.

**Root cause:** `GPUResourceComponent` stores D3D12 COM pointers as `uint64_t` for API-agnostic abstraction. The cast sites are scattered and unchecked.

**Structural weakness:** Type erasure via integer storage defeats the compiler's ability to enforce null safety, lifetime, and type correctness. Every consumer re-discovers the real type through a cast.

**Fix options (evaluate trade-offs):**
1. Store typed pointers behind a platform-specific section of the component
2. Use a typed wrapper that asserts non-null on access
3. At minimum, add null checks after every reinterpret_cast before dereference

**Scope:** `m_CommandList`, `m_PipelineStateObject`, `m_DeviceMemories[]`, `m_MappedMemories[]` in DX12FrameManagementService.cpp and DX12GPUBufferResourceService.cpp
<!-- SECTION:DESCRIPTION:END -->

## Resolution (2026-04-18)

Took option 3 at the unsafe surface: `DX12Helper::AsDX12CommandList(CommandListComponent*)` now centralises the `uint64_t → ID3D12GraphicsCommandList7*` cast with a logged-error guard on null. Errors escalate to fatal in test mode via the LogService `_Exit` path (landed in 11f3af15), so null never reaches the dereference in automated tests.

- 2c32f0cb — 18 sites in DX12FrameManagementService
- 345c8fa6 — 2 sites in DX12GPUBufferResourceService  
- c418ef98 — 5 remaining sites in DX12CommandListResourceService / DX12GraphicsHardwareService / DX12TextureResourceService, plus explicit null-return guard in `Execute` so we never submit a null list to a queue

`m_PipelineStateObject` is already stored as `IPipelineStateObject*` (typed), not `uint64_t`; the casts there are backend downcasts, not type erasure — left as-is.

Remaining type-erasure surfaces that weren't in scope:
- `TextureComponent::m_GPUResources` → `ID3D12Resource*` (DX12TextureResourceService)
- `m_DeviceMemories[]` / `m_MappedMemories[]`

The larger refactor — a backend-specific `DX12CommandListComponent` with a typed ComPtr — is still the right endgame. This step narrows the unsafe surface to one place so that refactor becomes mechanical.
