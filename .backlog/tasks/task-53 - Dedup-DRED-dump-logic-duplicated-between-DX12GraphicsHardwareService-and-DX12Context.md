---
id: TASK-53
title: >-
  Dedup DRED dump logic duplicated between DX12GraphicsHardwareService and
  DX12Context
status: To Do
assignee: []
created_date: '2026-04-16 20:37'
labels:
  - refactor
  - DX12
  - diagnostics
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?** "No magic numbers, no copy-paste; extract shared logic into helpers."

**What structural weakness allowed it?** DRED (Device Removed Extended Data) breadcrumb walking + page-fault printing is implemented twice:

- `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp::DumpDRED` — static free function, used from `DumpGPUDiagnostics` and `WaitOnFenceWithDiagnostics`.
- `Source/Engine/Services/DX12/DX12Context.cpp::CreateReadBackHeapBuffer` — inline DRED dump when device-removed is detected during read-back buffer creation.

Both walk the same `ID3D12DeviceRemovedExtendedData1` structure, print identical fields (command list name, queue name, LastCompleted, per-op status + breadcrumb op code, page-fault VA), but with minor stylistic drift (`"OK/>>LAST>>/.."` vs `"DONE/>>LAST>>/pending"`).

**What improvement moves the engine toward orthogonality?**

1. Move `DumpDRED` to a shared location (e.g. `DX12Helper_Common.h` as `inline void DumpDRED(ID3D12Device*)`, or a dedicated `DX12Diagnostics.{h,cpp}` TU if `DX12Helper_Common.h` should stay header-only).
2. Replace the inline DRED block in `DX12Context::CreateReadBackHeapBuffer` with a call to the shared helper.
3. Unify the per-op status string — pick one of `DONE/>>LAST>>/pending` (preferred; already used by `DumpDRED`).

**Acceptance:**
- One DRED dump implementation in the DX12 backend.
- Output format is identical across all call sites.
- No behavior change in normal operation; verified by the existing integration test (TASK-52 TDR repro still produces DRED output).

**Context:** Finding from TASK-34 / TASK-41 implementation — centralizing `LogD3D12CreateFailure` in `DX12Helper_Common.h` revealed the DRED dump was next in line for the same treatment but out of scope for that CL.
<!-- SECTION:DESCRIPTION:END -->
