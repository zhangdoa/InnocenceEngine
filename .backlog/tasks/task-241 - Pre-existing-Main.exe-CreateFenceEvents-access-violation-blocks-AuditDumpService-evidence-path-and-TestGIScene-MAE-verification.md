---
id: TASK-241
title: >-
  Pre-existing Main.exe `CreateFenceEvents:106` access violation — blocks
  AuditDumpService evidence path and TestGIScene MAE verification
status: To Do
assignee:
  - code-impl
created_date: '2026-06-15'
labels:
  - rendering
  - bug
  - dx12
  - render-pass
  - blocker
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:106
  - .omp/rules/audit-dump-for-render-evidence.md
  - .backlog/docs/doc-1-task-227-rfc.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
### Symptom

`Main.exe` (any preset, including `-c Data/Engine/Configuration/Presets/Audit.json` and `-c .../SerializeTest.json`) crashes at startup with:

```
Exception Code: 0xC0000005 (EXCEPTION_ACCESS_VIOLATION)
Exception Address: 0x00007FF7B35C92BB
Fault Address: 0x0000000000000018
Operation: Writing to memory
Function: Inno::DX12RenderPassResourceService::CreateFenceEvents
Source: C:\GitRepo\InnocenceEngine\Source\Engine\Services\DX12\DX12RenderPassResourceService.cpp:106
```

Reproduces via `git stash` pre-TASK-237 (so pre-existing before the C++23 migration), and the same crash happens across every preset I've tried. `Bin/RelWithDebInfo/TestSuite.exe` does NOT trigger it (no Main.exe path; the test suite calls the engine Setup but never starts the rendering pass).

### Root-cause hypothesis (highest-likelihood)

`Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:100-126`:

```cpp
for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
{
    auto l_semaphore = reinterpret_cast<DX12Semaphore*>(renderPass->m_Semaphores[i]);  // line 105
    l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);  // line 106
    ...
}
```

`m_DirectCommandQueueFenceEvent` is the 3rd `std::atomic<HANDLE>` (or `uint64_t`?) member of `DX12Semaphore` (defined in `Source/Engine/Services/DX12/DX12Headers.h`); the offset 0x18 = 3 × 8 bytes matches a 3-`std::atomic<uint64_t>` struct (8 bytes × 3 = 24 = 0x18). **The fault address 0x18 is exactly the offset being written to**, which means `l_semaphore` is `nullptr` and the write is `*(HANDLE*)(nullptr + 0x18) = 0x18`.

`reinterpret_cast<DX12Semaphore*>(renderPass->m_Semaphores[i])` is unsafe: `m_Semaphores[i]` is an `ISemaphore*` interface pointer, and the actual derived type may not be `DX12Semaphore`. If the semaphore pool is empty (no semaphore was created) the pool returns a "no-such-semaphore" sentinel pointer; the cast to `DX12Semaphore*` then dereferences through the vtable to a non-existent `DX12Semaphore`. Or the semaphore was created but the ISemaphore base class is the wrong type for the cast.

The two suspect failure modes:
1. **Empty-semaphore case**: `AddSemaphore()` returns a sentinel `ISemaphore*` (e.g., `nullptr` or a placeholder) when the engine has no semaphore to attach. The for-loop iterates `renderPass->m_Semaphores.size()` (non-zero) and dereferences the sentinel.
2. **Wrong-derived-type case**: `ISemaphore*` is implemented by a different class than `DX12Semaphore` in the new render-graph path (the T-Rex-era imperative pass classes had their own semaphore types; the pure-JSON graph nodes may bind to a different impl).

The bug was present before C++23 (pre-existing, reproduces via `git stash`); the C++23 migration did not introduce it.

### Why this matters

The pre-existing crash blocks TWO critical debug paths:

1. **AuditDumpService evidence path** — `Main.exe -c Data/Engine/Configuration/Presets/Audit.json` is the canonical methodology for diagnosing rendering regressions (per the new `.omp/rules/audit-dump-for-render-evidence.md` rule). The crash fires at engine init, before the audit-dump can trigger at frame 30.
2. **TestGIScene / TestPathTracerThreeScenes autotests** — both launch `Main.exe`; both inherit the crash. The autotest MAE bar cannot be re-verified until the crash is fixed.

This bug has been a standing blocker for the past 4+ CLs that have tried to run the autotest. Filing as a tracked task.

### Reproduction

```bash
MSYS_NO_PATHCONV=1 "Bin/RelWithDebInfo/Main.exe" -c "Data/Engine/Configuration/Presets/Audit.json"
# crash at DX12RenderPassResourceService::CreateFenceEvents:106 within 3 seconds
```

```bash
MSYS_NO_PATHCONV=1 "Bin/RelWithDebInfo/Main.exe" -c "Data/Engine/Configuration/Presets/SerializeTest.json"
# same crash
```

```bash
MSYS_NO_PATHCONV=1 "Bin/RelWithDebInfo/RenderTest.exe" -test draw_instanced
# same crash
```

### Investigation pointers

- Add a guard at the top of the `for` loop in `CreateFenceEvents`: `if (!l_semaphore) { Log(Error, "null semaphore for ", renderPass->m_InstanceName, " index ", i); continue; }`. This will tell us whether the suspect is null-sentinel or wrong-derived-type.
- `renderPass->m_Semaphores.size()` — what value is this? If 0 for every render pass, the loop is skipped and the crash is elsewhere; if non-zero, the semaphore list is being populated with bogus pointers.
- `AddSemaphore()` in the SemaphoreResourceService — does it return a placeholder when the pool is empty, or always return a real `DX12Semaphore*`? The `ISemaphore*` type system might be the wrong abstraction.
- The `DX12Semaphore` struct (in `Source/Engine/Services/DX12/DX12Headers.h`) — does its layout match the ISemaphore impl that's actually returned? If `DX12Semaphore` is the same type as what `AddSemaphore` returns, the cast is safe; if not, this is the bug.
- `reinterpret_cast` is the right cast for type punning here ONLY IF the dynamic type is exactly `DX12Semaphore`. A `dynamic_cast<DX12Semaphore*>(m_Semaphores[i])` would be safer (or an `assert(dynamic_cast` on debug builds).

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause confirmed — record the precise mechanism (null sentinel / wrong derived type / layout mismatch / other) and the call sites involved
- [ ] #2 Fix lands; `Main.exe -c Data/Engine/Configuration/Presets/Audit.json` completes and writes the `audit_*.hdr` files at frame 30; no access violation
- [ ] #3 `Main.exe -c Data/Engine/Configuration/Presets/SerializeTest.json` completes (1 frame, no render)
- [ ] #4 `Main.exe -c Data/Engine/Configuration/Presets/GIScene.json` completes 60 frames; the autotest MAE bar is re-verifiable
- [ ] #5 Build green; TestSuite green (116/114 pass/2-fail-pre-existing, no new fails)
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this is the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
