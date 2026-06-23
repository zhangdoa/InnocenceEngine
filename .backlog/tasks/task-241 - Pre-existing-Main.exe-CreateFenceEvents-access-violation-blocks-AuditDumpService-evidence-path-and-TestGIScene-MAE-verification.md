---
id: TASK-241
title: >-
  Pre-existing Main.exe `CreateFenceEvents:106` access violation — blocks
  AuditDumpService evidence path and TestGIScene MAE verification
status: Done
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

### Root cause (confirmed 2026-06-15)

`m_Semaphores[i]` is a `nullptr`, not a wrong-derived-type. The actual mechanism is
a pool-exhaustion / no-semaphore-attached scenario traced end-to-end:

1. `RenderPassResourceService::InitializeRenderPass` (Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp:83-87) populates `renderPass->m_Semaphores[i] = AddSemaphore()` for every swap-chain image slot (3 slots for triple-buffering).
2. `DX12RenderPassResourceService::AddSemaphore` (Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:41-44) calls `m_SemaphorePool->Spawn()`.
3. `TObjectPool<DX12Semaphore>::Spawn` (Source/Engine/Common/ObjectPool.h:58-89) returns `nullptr` when the pool is exhausted — and **silently** in the no-error case. No log line, no return-false, just `nullptr` propagated up.
4. `CreateFenceEvents:106` then `reinterpret_cast<DX12Semaphore*>(nullptr)` and writes to offset 0x18 of nullptr. With EBO on the empty `ISemaphore` base, `DX12Semaphore::m_DirectCommandQueueFenceEvent` lands at offset 0x18 (the task's "3 × 8" arithmetic is right; the vtable-pointer-on-an-empty-base concern that I considered was a red herring). The fault address 0x18 matches the field offset exactly.

**Type theory check** (the second suspect in the original hypothesis): `ISemaphore` is an empty struct (`Source/Engine/Common/GraphicsPrimitive.h:372`). `DX12Semaphore` is the *only* class that implements it in the DX12 path. `AddSemaphore` always returns a `DX12Semaphore*` cast to `ISemaphore*`. The reinterpret_cast back to `DX12Semaphore*` is type-correct modulo nullptr. So the bug is *not* a wrong-derived-type — it's a sentinel-not-surfaced case.

### What was actually triggering the pool exhaustion in the audit preset

The audit run's stderr (Build/captures/audit_2026-06-15_run.log) shows the SwapChain
pass being re-initialized ~150+ times. Each iteration calls
`m_Semaphores.resize(GetSwapChainImageCount())` and `AddSemaphore()` again. Two
convergent causes:

- **`RenderPassResourceService::InitializeComponents` (RenderPassResourceServiceImpl.cpp:53-68) re-enqueues failed inits forever**: `m_DeferredQueue.push(l_renderPass)` on any failure path. A first-time init that fails (e.g., for any reason — shader missing, PSO invalid arg, transient device issue) gets retried on the next pass, which allocates fresh semaphores, which never get freed because the first attempt's bookkeeping is also still live.
- **PSO pool (128) and semaphore pool (256) are hard-coded capacities** that pre-date the render-graph overhaul. The graph-driven path initializes many more passes (one per render-graph node) and re-init floods both pools.

The first WRL `IID_PPV_ARGS_Helper` access violation at 0x4E0 that the run produced
*after* the original crash is a **downstream symptom** of the same exhaustion:
`CreatePipelineStateObject` returned E_INVALIDARG (HRESULT -2147024809) for SwapChain
(the swapchain PSO descriptor was malformed because the InitializeRenderPass had
been called with no semaphore data backing the FenceEvents member), and the next
device call read a corrupt vtable.
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
## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Root cause confirmed
- [x] #2 CreateFenceEvents no longer crashes on null `l_semaphore` — guard added (Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:111-116) and `AddSemaphore` now surfaces its failure with an Error log (lines 46-49). Verified via `Main.exe -c Audit.json` log: the `0x18` access violation is gone; instead the log shows 150+ "Run out of object pool!" / "m_Semaphores[0..2] is nullptr" Error lines for `[Object Name: SwapChain]`, and then a downstream WRL crash at `IID_PPV_ARGS_Helper` (wrl/client.h:916) reading 0x4E0 — which is a SECOND, independent bug surfaced by removing the first crash. **Audit HDRs do NOT land yet.** Filed as TASK-242 (follow-up).
- [x] #3 `SerializeTest.json` completes (1 frame + serialize round-trip):
      exit 0, graceful, no orphan (2026-06-23, fresh build, via
      `StartEngineWin.ps1 -Preset Engine/Configuration/Presets/SerializeTest.json`).
- [x] #4 `GIScene.json` completes 60 frames: exit 0, graceful, no orphan
      (2026-06-23, same launcher). TASK-242 (the dependency) is Done.
      MAE number not computed this session — local ImageMagick is broken
      (`magick -version` exits 5), so `TestGIScene.ps1`'s MAE compare
      can't run; the MAE *path* is unblocked (the run completes and the
      captures the harness needs are produced). Numeric MAE re-check
      deferred to a working magick.
- [x] #5 Build green (DX12RenderPassResourceService.cpp + RenderPassResourceServiceImpl.cpp + RenderingConfigurationService.cpp rebuilt clean; Main.exe + RenderTest.exe both produced). TestSuite re-run with the patched code: **115/116 pass, 1 fail**. The 1 fail is `RenderGraph: lightPass.comp registers covered by live LightPass JSON bindings` — a pre-existing test failure citing the `lightPass.comp` shader's t69/t70/t71 register usage that has no matching JSON binding (the `df40414a` LightPass swap fallout). It is **not** caused by these changes. The previous AC wording claimed 116/114 + 2-fail-pre-existing; the current count is **115/116 with 1 pre-existing fail** (the 1 fail = lightPass binding mismatch).
<!-- AC:END -->
## Follow-up (TASK-242 — In Progress)

Removing the CreateFenceEvents crash exposed a deeper, separate bug class. **The audit
path is still blocked.** Two distinct issues remain:

1. **Pool exhaustion under retry** — `RenderPassResourceService::InitializeComponents`
   re-enqueues failed inits without bound. **Fixed in this CL**: retry loop capped to 3
   attempts, dead-letter after that. Pool capacities raised to 512/1024/512/4096.
2. **WRL `IID_PPV_ARGS_Helper` 0x4E0 read** — was a downstream symptom of pool exhaustion.
   After the fix above, this symptom is no longer reachable from the audit path.

A *new* unrelated bug emerged from the now-fully-instantiated engine: `TObjectPool<TextureComponent>::Spawn`
access violation at offset 0x20 during UnitTest scene load. This is NOT pool exhaustion
(4096 slots, only ~200 textures actually used). The fault at 0x20 = `m_CurrentFreeChunk`
at `this+0x18`, where `this` is a corrupt pointer (probably stack/use-after-free in the
scene-load path). **Not in scope for TASK-242** — needs a separate task to diagnose.
## Logs / artifacts

- `Build/captures/audit_2026-06-15_run.log` — pre-fix crash (1.7K lines, ACCESS_VIOLATION
  at 0x18 in DX12RenderPassResourceService::CreateFenceEvents:106)
- `Build/captures/audit_2026-06-15_run2.log` — post-fix run with absolute path (silent
  IOService path resolution failure, `Data/Engine/...` resolved from CWD = C:/GitRepo/InnocenceEngine,
  not from Bin/. Audit.json not loaded; defaults used)
- (No new audit HDRs in `Bin/RelWithDebInfo/`; still 2026-06-01 baseline.)

<!-- DOD:BEGIN -->
- [x] #1 Code compiles — `BuildWin.ps1` green; `Main.exe` + `RenderTest.exe` both produced (2026-06-23).
- [x] #2 Pre-existing integration runs re-run green on the fresh build (2026-06-23, all via `StartEngineWin.ps1`): Audit.json (35 frames, all 17 pass HDRs dumped to `Bin/audit_*.hdr`, exit 0); SerializeTest.json (exit 0); GIScene.json (60 frames, exit 0). No D3D12 crash, no orphan.
- [x] #3 N/A — the pre-existing Audit / SerializeTest / GIScene presets cover the path end-to-end; no new test needed.
- [x] #4 Not mock-based: validation is real engine integration runs.
- [x] #5 User-observable outcome: the Audit dump produced all 17 render-graph pass HDRs (`Bin/audit_*.hdr`, 2026-06-23 18:04); sampled sizes 280–666 KB = non-black geometry + lighting (a black/uniform HDR RLE-compresses to a few KB). Consistent with the 2026-06-22 byte-verified baseline (TASK-252 closure). `magick`-based pixel stats were unavailable (broken locally).
- [x] #6 NOT verified: (a) numeric GIScene MAE — local `magick` broken; (b) the intermittent init/shutdown hang did NOT manifest in 3 graceful runs this session but is NOT proven gone — it is a separate failure mode from the now-fixed CreateFenceEvents AV, filed as **TASK-255**; (c) the `TObjectPool<TextureComponent>` scene-load AV noted in the TASK-242 follow-up — not re-observed this session.
<!-- DOD:END -->

## Closure (2026-06-23)

All 5 ACs met. The titular blocker is fully resolved: the
`CreateFenceEvents:106` access violation is fixed (AC#2), the
AuditDumpService evidence path is unblocked (Audit run dumps all 17
pass HDRs), and the TestGIScene path runs 60 frames to graceful exit.
TASK-242 (the pool-exhaustion dependency for AC#4) is Done. Verified
on a fresh `BuildWin.ps1` build via three sanctioned `StartEngineWin.ps1`
runs (Audit / SerializeTest / GIScene), all exit 0, no orphan.
The residual intermittent init/shutdown hang (a different failure mode,
mitigated by `Invoke-EngineBounded`) carries forward as **TASK-255**.
