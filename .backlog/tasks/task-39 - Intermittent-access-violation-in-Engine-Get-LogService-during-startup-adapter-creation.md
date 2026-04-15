---
id: task-39
title: Intermittent access violation in Engine::Get<LogService> during startup adapter creation
status: Todo
priority: medium
type: bug
labels:
  - bug
  - startup
  - dx12
  - race
created: 2026-04-15
---

## Bug

On the first invocation of `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10` in a session, the process crashed at adapter creation with:

```
ACCESS VIOLATION DETECTED!
Exception: 0xC0000005 (EXCEPTION_ACCESS_VIOLATION)
Fault Address: 0x0000000000000010 (Reading)
Function: Inno::Engine::Get<Inno::LogService>
Source: C:\GitRepo\InnocenceEngine\Source\Engine\Engine.h:64  (+0x46)
```

The crash occurred right after:

```
Success: DX12GraphicsHardwareService::CreatePhysicalDevices: Adapter for: NVIDIA GeForce RTX 3070 Laptop GPU has been created.
```

The immediate retry (same binary, same args) completed the full 20-frame run cleanly (exit 0).

## Hypothesis

A callback or background task fires during/immediately after `CreatePhysicalDevices` and calls `Engine::Get<LogService>()` before the service registry has fully populated, or after a transient teardown/reinit during adapter enumeration. Candidates:
- DXGI / D3D12 driver callback (though debug layer was disabled in the crashed run)
- PhysX background task scheduled at setup time
- A worker thread dequeuing a log-producing task before LogService is ready

## Reproduction

```
Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10
```

Observed 1-in-2 so far. Needs wider sampling to characterize.

## Investigation plan

1. Add a counter + stress-loop that launches the process N times and records exit codes
2. If reproducible in loop: attach debugger to capture the crashing call site
3. Narrow by temporarily disabling PhysX setup, DX12 callback registration, and/or deferring worker thread task dispatch until after `Engine::Setup` returns
4. Root-cause the ordering bug

## Acceptance criteria

- [ ] #1 Reproduction rate characterized (crashes / N launches)
- [ ] #2 Crashing call stack captured (debugger or crash dump)
- [ ] #3 Root cause identified
- [ ] #4 Fix lands; stress loop of 100 launches produces 0 crashes
