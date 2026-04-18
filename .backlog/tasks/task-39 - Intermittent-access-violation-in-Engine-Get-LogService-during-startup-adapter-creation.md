---
id: TASK-39
title: >-
  Intermittent access violation in Engine::Get<LogService> during startup
  adapter creation
status: Todo
assignee: []
created_date: ''
updated_date: '2026-04-18 11:07'
labels:
  - bug
  - startup
  - dx12
  - race
dependencies: []
priority: medium
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

Observed 1-in-2 in the original sighting. Wider sampling (20 launches total: 10 without reload + 10 with `-reload_at_frame 10`) produced 0 crashes. Upper bound on crash rate at 95% CI from 20 clean runs is ~14%. The original crash remains unexplained but was not reproducible on demand — most likely a cold-start condition (first-ever-process-after-boot, D3D12 runtime init state, thermal/driver state) rather than a deterministic race.

## Investigation plan

1. Add a counter + stress-loop that launches the process N times and records exit codes
2. If reproducible in loop: attach debugger to capture the crashing call site
3. Narrow by temporarily disabling PhysX setup, DX12 callback registration, and/or deferring worker thread task dispatch until after `Engine::Setup` returns
4. Root-cause the ordering bug

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Reproduction rate characterized: 0/20 post-warmup, 1/1 on initial cold call. Likely cold-start condition, not a deterministic race.
- [ ] #2 #2 Crashing call stack captured (debugger or crash dump)
- [ ] #3 #3 Root cause identified
- [ ] #4 #4 Fix lands; stress loop of 100 launches produces 0 crashes
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Repro observation 2026-04-18

Hit the AV during TASK-60 investigation on `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30`. Same fault address (0x10), same function signature (`std::_Hash<...>::_Find_last<std::type_index>`), same module (LogService lookup via `Engine::Get<LogService>()`). Occurs roughly 1-in-3 invocations in this session; re-running always succeeds on the second attempt within the same session.

This matches the hypothesis that `Engine::singletons_` (a raw `std::unordered_map<std::type_index, void*>`) is being read concurrently with its first insertion. The crashing thread is likely a worker calling `Get<LogService>()` before the main-thread setup has finished populating the map.

Still doesn't give a reliable stress-loop repro, but the symptom is fresh enough now that attaching a debugger on first launch of the day would likely catch it.
<!-- SECTION:NOTES:END -->
