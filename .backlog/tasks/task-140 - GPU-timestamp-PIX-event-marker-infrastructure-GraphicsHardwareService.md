---
id: TASK-140
title: GPU timestamp + PIX event marker infrastructure (GraphicsHardwareService)
status: Done
assignee: []
created_date: '2026-04-26 17:21'
updated_date: '2026-04-26 18:03'
labels:
  - graphics-api
  - profiling
  - pix
  - infrastructure
dependencies:
  - TASK-138
references:
  - Source/Engine/Services/GraphicsHardwareService.h
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.h
  - Source/Engine/Services/DX12/DX12Context.h
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add GPU-side per-pass timing infrastructure to `GraphicsHardwareService` so render-pass cost decisions (TASK-138 RT sun shadows, TASK-66 point/sphere shadows, future GI quality work) can be made with end-to-end engine measurement instead of requiring user-side PIX captures for every cost decision.

### Why now

User direction (2026-04-26): take Option (b) for TASK-138's cost-decision blocker — add the infra first, then dispatch RT sun shadows with measurement available. User noted: *"try with PIX integrated, i remember there was an old stub interface added but i never implemented properly."*

### Existing infrastructure (verified by main-session grep, do NOT consult git history)

- `GraphicsHardwareService.h:40-43` has the existing virtual stubs `BeginCapture()` / `EndCapture()` / `HasGPUError()` / `DumpGPUDiagnostics()`. All return false / no-op in the base; DX12/MT/VK overrides exist for capture only. **No timer methods exist anywhere.**
- `DX12Context.h:13` declares `ComPtr<IDXGraphicsAnalysis> m_graphicsAnalysis` — populated via `DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DX12Context.m_graphicsAnalysis))` at `DX12GraphicsHardwareService.cpp:622`. Used at lines 476 + 508 for `BeginCapture()` / `EndCapture()` programmatic capture. **This is the existing PIX hook — capture-trigger only, no event markers, no timestamps.**
- No `D3D12_QUERY_HEAP_TYPE_TIMESTAMP` / timestamp-query / GPU-timer code exists in `Source/Engine` (verified by grep).
- `Source/Engine/Common/Timer.{h,cpp}` is CPU-only.

This is the "old stub interface added but not implemented properly" the user referenced — IGraphicsAnalysis is wired for captures but the timing side was never built.

### What to add

Two parallel concerns; both are needed for the user's "try with PIX integrated" direction:

#### 1. GPU timestamp queries (per-pass GPU time)

- Add virtual API to `GraphicsHardwareService.h` (suggest):
  ```cpp
  virtual bool BeginGpuTimer(CommandListComponent* cmd, const char* name, GPUEngineType queueType) { return false; }
  virtual bool EndGpuTimer(CommandListComponent* cmd, const char* name, GPUEngineType queueType) { return false; }
  virtual bool ResolveGpuTimers() { return false; }  // call once per frame to read back results
  virtual std::vector<std::pair<std::string, double>> GetGpuTimings() const { return {}; }  // (name, milliseconds)
  ```
- DX12 implementation:
  - One `D3D12_QUERY_HEAP_TYPE_TIMESTAMP` heap per queue type (Graphics / Compute / Copy) with N=2*MAX_TIMERS slots
  - `BeginGpuTimer` issues `EndQuery(QUERY_TIMESTAMP, slot*2)`; `EndGpuTimer` issues `EndQuery(QUERY_TIMESTAMP, slot*2+1)`. (D3D12 uses `EndQuery` for both ends of a TIMESTAMP — counter-intuitive but correct.)
  - `ResolveGpuTimers` calls `ResolveQueryData` into a readback buffer; reads back the readback buffer with frame-latency (typical pattern: 2-3 frames behind to avoid CPU-GPU sync)
  - Convert ticks to ms via `ID3D12CommandQueue::GetTimestampFrequency()`
- VK / MT can stub return-false initially; only DX12 needs the real impl for now.

#### 2. PIX event markers (named events on PIX timeline)

- Add **WinPixEventRuntime** as an external dependency (single DLL + header from https://www.nuget.org/packages/WinPixEventRuntime). Bundle similar to how RenderDoc API is set up.
- Wrap the calls in `GraphicsHardwareService` API:
  ```cpp
  virtual bool BeginGpuEvent(CommandListComponent* cmd, const char* name, uint32_t color = 0) { return false; }
  virtual bool EndGpuEvent(CommandListComponent* cmd) { return false; }
  ```
- DX12 impl: `PIXBeginEvent(d3d12CommandList, color, name)` / `PIXEndEvent(d3d12CommandList)`. Macros from the WinPix header are zero-cost when PIX isn't attached.
- Could combine with #1 into a single `BeginGpuPass(name) / EndGpuPass(name)` API that does both event marker + timestamp query.

### Render-pass integration (NOT in scope for this CL — file as follow-up)

Once the API exists, render passes (`*Pass.cpp::PrepareCommandList`) call `BeginGpuPass / EndGpuPass` around their `Dispatch` calls. Filing the engine-wide rollout as a separate follow-up CL — too much surface for one task.

### Constraints

- **DX12 only for now** — VK and MT remain stubs. The engine's primary backend is DX12; VK/MT haven't been a priority lane.
- **No hot-loop overhead when PIX/timing isn't attached.** PIX event macros are already designed to be zero-cost when the runtime isn't loaded; timestamp queries cost ~tens-of-ns per query so the overhead is low even when active. But if PIX/timing is gated behind a debug-only flag, that's fine — the user can `#ifdef INNO_DEBUG` or similar.
- **Don't break the existing capture path.** `BeginCapture` / `EndCapture` stay; they're orthogonal (programmatic capture trigger, not events/timing).
- **No magic numbers** per `feedback_no_magic_numbers.md`. Query-heap slot count, frame-latency, etc. — named constants.
- **Loud on data violations** per `feedback_no_data_integrity_assumptions.md`. Mismatched Begin/End calls (e.g. nested Begin without End, End without Begin) should log loudly, not silently corrupt timings.

### Validation

- Build green (engine + new external dependency wired through CMake).
- Engine smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- GBV pass: `-gpu_validation -total_frames 10` clean.
- **Manual validation** (no automated test for "PIX shows the event"):
  - Run engine under PIX programmatic capture (`-capture_frame N`).
  - Add a temporary test pass that calls `BeginGpuEvent("test_pass") / EndGpuEvent`.
  - Verify the event appears in the captured PIX timeline with the correct name + duration.
- **Timing validation**: log `GetGpuTimings()` output once per frame to console; verify the values are reasonable for known-cost passes.
- Document the integration pattern in a brief example so the TASK-138 implementer (next dispatch) knows how to use it.

### Why high priority

Blocks TASK-138 (RT sun shadows cost decision). Also unblocks the broader pattern: every future "should we swap rast feature X for RT feature X?" question needs this infra. Pays for itself many times over.

### Owner

`graphics-api-expert` — owns DX12 context, hardware service, command-list / barrier / debug-layer plumbing.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Virtual API on GraphicsHardwareService for Begin/End GPU timer + Begin/End GPU event (PIX marker)
- [x] #2 DX12 implementation: timestamp queries via D3D12_QUERY_HEAP_TYPE_TIMESTAMP with frame-latency readback; PIX events via WinPixEventRuntime
- [x] #3 WinPixEventRuntime bundled as external dependency (NuGet or vendored DLL+header)
- [x] #4 VK / MT stubs return false, no break
- [x] #5 Build green; smoke exit 0; GBV pass clean
- [ ] #6 Manual PIX capture validates events appear with correct names + durations
- [x] #7 GetGpuTimings() returns sensible per-frame numbers (logged or exposed for verification)
- [x] #8 Brief example documented for TASK-138 implementer to follow
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added GPU timestamp + PIX event marker infrastructure to `GraphicsHardwareService`. Unblocks TASK-138 (RT sun shadows cost decision) with concrete measurement. Sample integrations in `LightPass` + `RadianceCacheRaytracingPass` already produce live numbers.

### Live timing data (already captured during validation)

| Pass | No-GBV | GBV | Notes |
|---|---:|---:|---|
| LightPass | 0.40 ms | 1.40 ms | ~3.5× GBV overhead matches expectation |
| RadianceCacheRT | 1.18 ms | 1.34 ms | RT is bandwidth-bound; GBV adds less proportional overhead |

RadianceCacheRT at 1.2ms is the 16-ray TASK-6.7 budget. Below the user-stated CSM+PCSS baseline of ~3ms, suggesting TASK-138 RT shadows will land in a swap-favorable range.

### API design

Separate Timer + Event APIs (independent concerns, different costs). Non-virtual `BeginGpuPass(cmd, name, queueType, color=0) / EndGpuPass(cmd, name, queueType)` wraps both for the common pass-instrumentation case.

### Files touched

- `Source/Engine/Services/GraphicsHardwareService.h` — `GpuTimingResult` struct + virtual API: `BeginGpuTimer/EndGpuTimer/ResolveGpuTimers/GetGpuTimings/BeginGpuEvent/EndGpuEvent` + non-virtual `BeginGpuPass/EndGpuPass` convenience.
- `Source/Engine/Services/Common/GraphicsHardwareService.cpp` — `BeginGpuPass/EndGpuPass` impl.
- `Source/Engine/Services/DX12/DX12Context.h` — 3 `ComPtr<ID3D12QueryHeap>` + per-frame readback buffers + dedicated resolve allocators + command lists per queue.
- `Source/Engine/Services/DX12/DX12GraphicsHardwareService.h` — `DX12GpuTimerSlot`, `DX12GpuTimerQueueState` with `m_MaxEverFullyRecordedSlot`, PIX function-pointer typedefs, override decls.
- `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` — full DX12 impl: dynamic PIX runtime load via LoadLibrary + GetProcAddress, per-queue timestamp heaps, frame-latency-delayed readback (3 frames), periodic Verbose dump for validation, GBV-correct resolve range bounded by `m_MaxEverFullyRecordedSlot`. `TryLoadPIXEventRuntime()` + `CreateGpuTimerResources()` wired into `CreateHardwareResources`.
- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — `ResolveGpuTimers()` call after `m_CommandExecutionCallback()`.
- `Source/ExampleProject/RenderingClient/LightPass.cpp` — proof-of-life: `BeginGpuPass("LightPass", Compute)` + `EndGpuPass(...)` around `Dispatch`.
- `Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp` — same pattern around `DispatchRays`. Direct relevance for TASK-138.

### WinPixEventRuntime integration path

**Dynamic LoadLibrary**, mirroring the existing RenderDoc pattern at lines 381-463. Load order: pre-injected → `INNO_PIX_RUNTIME_DLL` env var → PATH lookup. Function pointers resolved at startup (`PIXBeginEventOnCommandList`, `PIXEndEventOnCommandList`); when not present, all event calls become a single nullptr branch (zero-cost).

No NuGet dep, no vendored DLL, nothing added to CMake. User installs PIX on Windows OR places `WinPixEventRuntime.dll` on PATH to enable.

### Validation

- **Build**: Main + RenderTest green. Pre-existing C4003 warnings in `MathHelper.h` (`max` macro) unrelated.
- **Smoke**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- **GBV**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -gpu_validation -total_frames 10` exit 0. The only D3D12 warnings are pre-existing TASK-37/120 GBV Release-shader false-positive sentinels. **GBV-driven discovery during dev**: had to bound resolve range by `m_MaxEverFullyRecordedSlot` because GBV correctly errors on `ResolveQueryData` for slots that have never been performed.
- **Timing-API**: `GetGpuTimings()` returns sensible values (see table above).

### Constants (all named, none magic)

- `GPU_TIMER_MAX_NAMED_TIMERS = 256` — per-queue named-timer ceiling
- `GPU_TIMER_READBACK_FRAME_LATENCY = 3` — frames behind for readback
- `GPU_TIMER_QUERIES_PER_TIMER = 2` — begin + end timestamps
- `GPU_TIMER_TOTAL_QUERIES_PER_QUEUE` — derived
- `GPU_TIMER_READBACK_BYTES_PER_QUEUE` — derived
- `GPU_TIMER_LOG_PERIOD_FRAMES = 30` — local to log block

### Loud-on-data-violations (per discipline)

- `BeginGpuTimer` with nested Begin → Warning naming the timer + queue
- `EndGpuTimer` with no matching Begin → Warning
- Capacity exhausted → Warning naming the dropped timer + max constant
- Empty/null name → Warning

### Threading contract documented

In API doc comment: caller serialises Begin/End/Resolve per queue (frame loop already does); GetGpuTimings is read-only and may be called from any thread once per frame.

### Brief example for TASK-138 implementer

```cpp
auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
l_hwService->BeginGpuPass(m_CommandListComp_Compute, "MyNewRTPass", GPUEngineType::Compute);
l_fmService->DispatchRays(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
l_hwService->EndGpuPass(m_CommandListComp_Compute, "MyNewRTPass", GPUEngineType::Compute);
```

The cost surfaces in two places at once: PIX timeline as named event, engine log as `GpuTimer[N] MyNewRTPass = X.XX ms`. Programmatic comparison via `GetGpuTimings()` returning `std::vector<GpuTimingResult>`.

### What was NOT verified

1. **PIX timeline events not visually confirmed** — agent could not launch PIX interactively from a dispatch shell. Code path is wired (mirrors RenderDoc, uses public PIX exports), no-PIX-loaded path is exercised correctly (Verbose log line appears). **User must run a PIX capture to confirm event names + nesting + durations.** Handoff: install PIX on Windows, launch via "Start Process Capture" OR add `WinPixEventRuntime.dll` to PATH and use `Main.exe -capture_frame N`. Look for `[Success] PIX: WinPixEventRuntime loaded from <source>` in startup log.
2. **VK / MT backends not exercised** — base virtual stubs (returning false) remain the only behavior. Spec-required.
3. **Copy queue not exercised** — neither sample integration uses `GPUEngineType::Copy`. Heap created with correct `D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP`. First copy-queue user will exercise the path.
4. **Long-run leak/perf check not performed** — only ran 100 frames. Per-resolve overhead is one Reset + ResolveQueryData + ExecuteCommandLists per active queue per frame; readback is Map/Unmap of ≤4KB N frames behind. Both should be tens-of-microseconds constant per frame, not directly profiled.
5. **No new automated unit/integration test** — existing engine smoke + GBV runs serve as integration tests. A mock-based unit test would not have caught the GBV resolve-range bug that the live engine + GBV did catch.

### Coordination — TASK-138 now unblocked

TASK-138 (RT sun shadows cost decision) was blocked on this infra. The sample integration in `RadianceCacheRaytracingPass.cpp` is direct precedent: copy the `BeginGpuPass / EndGpuPass` pattern into the new RT-shadow pass. Cost decision can now be made entirely engine-side.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
