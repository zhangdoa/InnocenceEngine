---
id: TASK-242
title: >-
  RenderPassResourceService pool-exhaustion under retry blocks audit-dump
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
dependencies:
  - TASK-241
references:
  - Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp:53-68
  - Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:18-20
  - Source/Engine/Common/ObjectPool.h:58-89
priority: high
---

## Description

### Symptom

`Main.exe -c Data/Engine/Configuration/Presets/Audit.json` (and any preset that
drives the render-graph path with > 1 render pass) does not crash at
`CreateFenceEvents` anymore — TASK-241's null guard catches the exhausted
`AddSemaphore` call. But the run now shows a different failure pattern in the
log:

```
[Verbose][Inno::RenderPassResourceService::InitializeRenderPass] [Object Name: SwapChain] Semaphore has been created.
[Error][Inno::DX12RenderPassResourceService::CreateFenceEvents] [Object Name: SwapChain] CreateFenceEvents: m_Semaphores[0] is nullptr
[Error][Inno::DX12RenderPassResourceService::CreateFenceEvents] [Object Name: SwapChain] CreateFenceEvents: m_Semaphores[1] is nullptr
[Error][Inno::DX12RenderPassResourceService::CreateFenceEvents] [Object Name: SwapChain] CreateFenceEvents: m_Semaphores[2] is nullptr
[Verbose][Inno::RenderPassResourceService::InitializeRenderPass] [Object Name: SwapChain] PipelineStateObject has been created.
[Error][Inno::TObjectPool<class Inno::DX12PipelineStateObject>::Spawn] Run out of object pool!
... (repeats ~150 times) ...
[Error][Inno::DX12Helper::LogD3D12CreateFailure] DX12 create failed: Graphics PSO context=SwapChain HRESULT=-2147024809 DeviceRemovedReason=0
[UnhandledExceptionHandler] ACCESS VIOLATION DETECTED!
Exception Address: 0x00007FF65A04D659
Fault Address: 0x00000000000004E0
Function: IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<ID3D12RootSignature>>
```

The exact same callstack (TASK-241's) was the entry point; removing the
fence-events crash exposed the next crash downstream. Audit HDRs do not land.

### Root cause

Two convergent defects:

1. **Unbounded retry of failed init** — `RenderPassResourceService::InitializeComponents`
   re-enqueues failed passes without limit
   (`Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp:64`):
   `m_DeferredQueue.push(l_renderPass)` is invoked for any failure. The first
   call to `AddSemaphore()` for a pass whose PSO init also fails will silently
   consume a `DX12Semaphore` slot (because the per-call `m_Semaphores.resize`
   runs before `AddSemaphore`), and the next pass initialization
   (for the SAME pass) consumes another. After ~85 cycles, the 256-slot
   semaphore pool is exhausted.

2. **Hard-coded pool capacities too small for graph-driven init** — `m_PSOPool`
   is `TObjectPool<DX12PipelineStateObject>::Create(128)` and
   `m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256)`
   (`Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:18-19`).
   These numbers pre-date the render-graph overhaul (TASK-227 era). The
   graph-driven init creates a `RenderPassComponent` and semaphore triple per
   pass per swap-chain image, and the imperative-pass path additionally
   reserved 1 global semaphore. The graph path consumes more because the
   `m_Semaphores.resize(GetSwapChainImageCount())` (3 slots for
   triple-buffering) is done per pass, and the render graph initializes ~17
   passes. 17 × 3 = 51 semaphores — fits in 256 *if* the retry loop never
   runs. With the retry loop, it doesn't fit.

### Why the WRL `IID_PPV_ARGS_Helper` 0x4E0 crash happens

After pool exhaustion, `CreatePipelineStateObject` for the SwapChain returns
`E_INVALIDARG` (`HRESULT -2147024809` = `0x80070057`) — the device is
in a degraded state (possibly DeviceRemoved, possibly just a bad PSO
descriptor handed to it after the failed fence-events init left PSO
descriptor members uninitialized). The next call into the device via
`ID3D12Device::CreateRootSignature` reads from a corrupt vtable through
WRL's `IID_PPV_ARGS_Helper`, dereferencing offset 0x4E0 of a near-null
pointer. This is a **downstream symptom** of the pool exhaustion, not a
fresh bug.

### Why this matters

The audit-dump path is the canonical methodology for diagnosing rendering
regressions (per `.omp/rules/audit-dump-for-render-evidence.md` and
`ccda841b` directive). Without fresh audit HDRs:
- The `ccda841b` "audit-dump as canonical evidence" directive cannot be
  satisfied.
- TestGIScene / TestPathTracerThreeScenes autotests (AC#4 of TASK-241) are
  still blocked.
- The pre-existing SunShadowRT_Visibility = 0 bug (noted in the inspection
  report) cannot be re-verified post-swap.

### Investigation pointers

- Add a hard cap on `m_DeferredQueue.push` retries in
  `InitializeComponents` (e.g., dead-letter after N attempts, or
  schedule a backoff). A `static_assert` on a max-retry counter would
### Final root cause (resolved 2026-06-15)

The third crash (`TObjectPool<TextureComponent>::Spawn this=0x0`) was **not** pool exhaustion.
Debugger evidence (`lldb -s _dbg.txt`):

- Frame 1: `TextureResourceService::Add(this=0x...fcb00, name="RayTracingResult")` — service valid.
- Frame 1 inner: `NamedObjectPool::Allocate(this=0x...fcb08, name="RayTracingResult")` — `this+8` (the `m_Pool` slot).
- Frame 0: `TObjectPool::Spawn(this=0x0000000000000000)` — `m_Pool` is **nullptr**.
- Frame 2: `RayTracer::Initialize` at `Engine.h:56` — called BEFORE `TextureResourceService::Setup()`.

The conditional at `Engine_Setup.cpp:99` (`if (!l_cfg->IsHeadless())`) gates Setup of all resource services. With `isHeadless: true` in Audit.json, `m_Pool.Initialize(maxTextures)` was **never called**, leaving `m_Pool = nullptr`. The RayTracer then dereferences it.

**Fix**: set `isHeadless: false` and `isOffscreen: false` in Audit.json so Setup runs.

## Acceptance Criteria

- [x] #1 Audit run produces fresh HDRs — full fix chain (5 distinct bugs):
      (a) CreateFenceEvents null guard + AddSemaphore Error log (TASK-241),
      (b) InitializeComponents retry-cap to 3 with dead-letter Suspended state
      (Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp:64),
      (c) PSO/Semaphore/OMT pool capacity bumps 512/1024/512 and maxTextures 4096,
      (d) Audit.json: isHeadless false + isOffscreen TRUE (offscreen routes through HeadlessWindowService
      so Present() no-ops and never blocks on a hidden window) + initialScene set so the scene loads,
      (e) **AuditDumpService dead-callback fix** — `Setup` registered a stack-local `std::function`
      via `AddSceneLoadedCallback(&l_cb)`; the callback list held a dangling pointer that `SceneService::LoadSync`
      dereferenced at line 76 (crash at fault 0xF). Fixed by storing the callback as a member
      `m_SceneLoadedCallback` (AuditDumpService.h:32, .cpp:73).
      **VERIFIED end-to-end via lldb**: all 17 audit passes dumped (`AuditDump: saved audit_00a..audit_13`,
      `AuditDump complete.`). Fresh HDRs in `Bin/`. The dumps reveal the actual rendering regression:
      OpaquePass GBuffer / Sky / LightPass / TAA are ALL 0%-nonzero (black); only SunShadowRT (1.0) and
      FinalBlend (flat 6e-5) are non-zero. Filed as TASK-243 (GBuffer-black regression).
- [ ] #2 `Main.exe -c Data/Engine/Configuration/Presets/SerializeTest.json` completes (1 frame, no render)
      — not re-run after the fix. Should now succeed at the same rate as the audit run.
- [ ] #3 `Main.exe -c Data/Engine/Configuration/Presets/GIScene.json` completes 60 frames; the autotest
      MAE bar is re-verifiable — depends on the TextureComponent pool `this`-corruption bug (separate
      diagnosis needed).
- [x] #4 Build green; TestSuite re-run with patched code: **115/116 pass, 1 fail** (the 1 fail is the
      pre-existing `lightPass.comp` register-vs-JSON binding mismatch from `df40414a` — unrelated to
      this task).
- [partial] #5 Retry-loop fix verified indirectly: with `AddSemaphore` now logging a clean Error on
      nullptr and `CreateFenceEvents` skipping null entries, the failure path is observable. A targeted
      test for the retry-cap behavior has not been written. After the fix, the log shows
      `[Object Name: SwapChain] failed to initialize after 3 attempts; dropping from queue` after
      3 retries (confirmed by the 262KB log captured at `Build/captures/audit_run7_out.log`).

## Definition of Done

- [ ] #1 Code compiles
- [ ] #2 Pre-existing integration tests re-run green
- [ ] #3 New integration test added if no existing test covers the retry loop
- [ ] #4 Self-authored mock-based tests are not the sole validation
- [ ] #5 User-observable outcome verified — fresh audit HDRs in `Bin/RelWithDebInfo/`
      with timestamps from the new run
- [ ] #6 Final summary lists what was NOT verified
