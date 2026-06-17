---
id: TASK-247
title: >-
  TextureResourceService init-loop: D3D12 DeviceRemoved + unbounded retry + dangling component
status: In Progress
assignee:
  - code-impl
created_date: '2026-06-16'
labels:
  - rendering
  - bug
  - d3d12
  - infra
dependencies:
  - TASK-242
  - TASK-245
priority: high
---

## Description

Surfaced while verifying TASK-246 (live-engine smoke for the `GPUUploadable`
validator). `TextureResourceService::InitializeComponents` infinite-loops on
texture init failures; the engine never reaches the GPU upload phase and
every smoke run produces a 0-byte log. **Pre-existing engine defect, not a
TASK-246 regression.**

### Symptom (captured via lldb attach to `TestSuite.exe` after the engine wedged)

```
[Error] DX12Helper::LogD3D12CreateFailure DX12 create failed: default heap
       buffer context= HRESULT=-2147024809 DeviceRemovedReason=0
[Error] DX12TextureResourceService::InitializeImpl [Object Name: ] Failed to
       create default heap buffer for frame 0
[Warning] TextureResourceService::InitializeComponents entity N no longer has
       TextureComponent, using stored pointer
[Verbose] TextureResourceService::InitializeComponents processing deferred
       init for: [Object Name: ]
```

Cycles entities 1 → 4 → 1 → 4 → ... forever. The engine's `LogService` only
flushes on natural exit or `m_LogFile << std::endl`; an in-loop `Log(Error)`
that never reaches the line terminator leaves the file at 0 bytes. Force-
killing the engine loses the buffered content. Diagnosis required the native
debugger because the live-engine smoke path was unobservable from logs.

### Three coupled defects

1. **D3D12 DeviceRemoved on first texture init.** `HRESULT=-2147024809` is
   `E_INVALIDARG`, but `DeviceRemovedReason=0` means the device was already
   gone when the call landed. The D3D12 device was successfully created
   earlier in `DX12GraphicsHardwareService::CreatePhysicalDevices`, so
   *something* between device creation and the first texture init is
   removing the device. **Root cause unknown — separate investigation.**

2. **Unbounded retry.** Failed inits are re-enqueued without a cap. The
   same `(entity 1..4, frame 0)` failure is processed every iteration.
   Identical pattern to TASK-242 (RenderPassResourceService pool exhaustion
   under retry). A retry-cap and an explicit "init failed; give up" log
   would break the loop and surface the underlying D3D12 error clearly.

3. **Dangling component.** The `entity N no longer has TextureComponent,
   using stored pointer` warning is a stale pointer to a removed
   `TextureComponent`. The 0-dim placeholder fails every iteration. The
   `EntityRegistry` is replacing components under the service's feet
   (similar to TASK-72's `EntityRegistry` invalidation pattern).

### Reproduction

1. Build: `Scripts/BuildWin.ps1 -SkipShaderCompile`
2. Run from `Bin/RelWithDebInfo`:
   `timeout 5 Main.exe -c Data/Engine/Configuration/Presets/PT.json -total_frames 3 -offscreen 1`
3. The engine hits the init-loop and never exits; `timeout` fires after
   5s. Resulting `*.Log` is 0 bytes because the engine never reached
   `~LogService()`.

### Live-engine evidence path (lldb recipe)

Per the `native-debugger` skill — attach lldb to the wedged `Main.exe`,
break on `TextureResourceService::InitializeComponents`, and `frame
variable` to read the per-entity TextureComponent state. The native
debugger sidesteps the LogService buffering issue entirely.

### Proposed approach

- **Step 1 (this task)**: cap the retry to N attempts (start with 3,
  surface each retry at `Warning`, and `Error` on the final failure).
  Single change; should land in one commit. Mirrors the fix for
  TASK-242.
- **Step 2 (separate)**: investigate the D3D12 DeviceRemoved root cause.
  The D3D12 debug layer (`-gpu_validation`) and PIX captures are the
  primary tools. The DeviceRemovedReason=0 is the smoking gun — whatever
  removed the device is doing it between `CreatePhysicalDevices` and
  the first texture init.
- **Step 3 (separate)**: harden the `EntityRegistry` callbacks so a
  removed `TextureComponent` invalidates the service's stored pointer
  rather than leaving it dangling. Pattern from TASK-72.

### Related

- TASK-242 (RenderPassResourceService pool exhaustion under retry — same
  unbounded-retry pattern)
- TASK-245 (TestSuite C++23 build + texture init-loop — Issue 2 is the
  same defect; this task supersedes it)
- TASK-246 (the live-engine smoke for the GPUUploadable validator is
  blocked by this defect; standalone unit test in `d990770e` works around
  it)

### Acceptance Criteria

- [x] #1 Engine does not loop on init failure: on `InitializeImpl` failure
      the texture's `m_ObjectStatus` is set to `Suspended`, the task is
      dropped, and an Error log names the texture. Mirrors the TASK-242
      `RenderPassResourceService::InitializeComponents` pattern (commit
      `14cd6476`) — first failure suspends; no retry.
      (NB: shipped as suspend-on-failure rather than the 3-attempt retry
      cap originally described; the Suspended state is the existing
      engine idiom for this case and retrying a likely-deterministic
      failure is wasted work.)
- [x] #2 Dangling-component warning no longer appears: when the entity no
      longer has a `TextureComponent`, the deferred-init task is dropped
      with a Verbose log. The stale stored pointer is no longer used.
      Verified: TestSuite unit log shows 8 hits of "no longer has
      TextureComponent, dropping deferred init" (entities 1-5, 17-19)
      with NO Error / NO "using stored pointer" warning. Engine
      terminates cleanly: `========== UNIT TESTS COMPLETE ==========`.
- [x] #3 Live-engine smoke: `Main.exe -c Audit.json` reaches steady state
      at frame 4 (TLAS stable, 56 instances, deferred queue empty).
      Log size 257,994 bytes (was 0). Pre-existing `Final Blend Pass
      Result` import-not-found and pre-existing fence-init crash are
      unrelated and remain (TASK-163, TASK-241).
- [ ] #4 D3D12 DeviceRemoved root cause diagnosed (separate sub-task).
      NOT IN SCOPE for this task. The init-loop class of bug is fixed;
      the D3D12 `E_INVALIDARG` + `DeviceRemovedReason=0` is a
      separate investigation (the fix would break the LOOP, not the
      HRESULT — once we surface the cause, the new dead-letter
      behavior will log the failure clearly instead of looping).

## Session log

- **2026-06-16**: discovered via lldb while verifying TASK-246. Symptom
  documented above. `Main.exe` accumulates 4000+ CPU-seconds and 430+ MB
  before being killed. lldb attach to `TestSuite.exe` (the test harness
  hits the same init-loop) revealed the full error chain.
- **2026-06-17 (this session)**: landed the fix in two AST rewrites of
  `Source/Engine/Services/Common/TextureResourceServiceImpl.cpp:136-170`:
  (1) the dangling-component `else Log(Warning, ...)` branch becomes
  `Log(Verbose, ...) + continue` (drop the task, do not use stale
  pointer); (2) the `else m_DeferredQueue.push(std::move(l_task))` on
  init failure becomes `Log(Error, ...) + m_ObjectStatus = Suspended`
  (drop, do not retry). Both rewrites use disjoint AST patterns; build
  green; TestSuite unit complete exit 0; `Main.exe -c Audit.json`
  reaches steady-state at frame 4. The retry-cap approach (the
  original AC) was abandoned in favor of the suspend-on-failure
  pattern that the engine already uses in `RenderPassResourceService`
  (TASK-242, commit `14cd6476`) — same idiom, less code, no wasted
  retries on a likely-deterministic failure. The D3D12 DeviceRemoved
  root cause is filed as AC#4, separate investigation.
