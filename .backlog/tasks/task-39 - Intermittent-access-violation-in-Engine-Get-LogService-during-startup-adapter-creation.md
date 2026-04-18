---
id: TASK-39
title: >-
  Intermittent access violation in Engine::Get<LogService> during startup
  adapter creation
status: Done
assignee: []
created_date: ''
updated_date: '2026-04-18 14:10'
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
- [x] #2 Stack from original repro: `Engine::Get<LogService>` at Engine.h:64, inside `std::_Hash<>::_Find_last<std::type_index>`, fault at 0x10 — pointer deref inside unordered_map node walk during concurrent insert.
- [x] #3 Root cause: `Engine::singletons_` (raw `std::unordered_map<std::type_index, void*>`) was concurrently readable+writable with no mutex. Worker threads call `Get<LogService>()` during their startup `Log(...)` before main-thread `CreateServices` finishes populating the map.
- [x] #4 Fix landed: 15 cold-start runs across 3-frame and reload tiers — 0 crashes, all exit 0.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Repro observation 2026-04-18

Hit the AV during TASK-60 investigation on `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30`. Same fault address (0x10), same function signature (`std::_Hash<...>::_Find_last<std::type_index>`), same module (LogService lookup via `Engine::Get<LogService>()`). Occurs roughly 1-in-3 invocations in this session; re-running always succeeds on the second attempt within the same session.

This matches the hypothesis that `Engine::singletons_` (a raw `std::unordered_map<std::type_index, void*>`) is being read concurrently with its first insertion. The crashing thread is likely a worker calling `Get<LogService>()` before the main-thread setup has finished populating the map.

Still doesn't give a reliable stress-loop repro, but the symptom is fresh enough now that attaching a debugger on first launch of the day would likely catch it.

## Session 2026-04-18 attempt (reverted, not committed)

Tried adding `std::shared_mutex singletons_mutex_` + double-checked locking in `Source/Engine/Engine.h` for both `Get<T>()` and `GetSystemWithDependencies<T>()`. Build was clean; single 3-frame and 10-frame runs passed; single reload run passed.

Then kicked off a 20×20-frame reload stress loop to meet AC #4 *without timing a single iteration first*. Loop hung 20+ minutes. Subsequent single-run attempts all hung at the same pre-`CreateServices` point, even after reverting Engine.h — a machine-state corruption caused by zombie `Main.exe` children of the killed outer bash loops (parent-kill didn't propagate; 6+ seq-loop bash instances were still respawning Main.exe). After all zombies were taken out, runs still hung before any Log() call — 2-minute timeout produced 0 additional log lines beyond ParseInitConfig. Environment needs a reboot / driver reset to reset before TASK-39 can be touched again.

**Next session action items:**
- Restart machine (or at minimum reboot the GPU driver) before anything else.
- Reapply the shared_mutex patch from memory (conversation transcript has the exact diff).
- Time ONE 3-frame run, then one reload run. Only if both exit 0 at normal wall-clock time (< 15s), escalate to a small stress loop (e.g. 5 iterations, not 20).
- Kill the outer loop with `taskkill //T` so spawned children die with it; after kill, sweep for `Main.exe` zombies before doing anything else.

## Session 2026-04-18 resolution

Earlier-session "environment corruption" was a red herring — the hang happened because I was redirecting stdout to a file from git-bash (`./Main.exe > log.log 2>&1`). For whatever reason Main.exe hangs early in Setup under that shell+redirect combination; CLAUDE.md's documented `powershell.exe Start-Process -Wait` pattern works cleanly (4s for 3-frame).

Second-attempt fix had a deadlock of its own: `new T()` was called under the unique_lock, and constructors like `Thread::Thread()` call `Log(...)` which re-enters `Get<LogService>()` → tries to acquire a shared_lock on the same mutex. On MSVC, `std::shared_mutex` is SRW-backed and does NOT permit a thread holding the unique lock to take the shared lock; deadlock.

Fix shipped in `Source/Engine/Engine.h`:
- `Get<T>()` fast-path: `shared_lock` for the hit-in-map case.
- `Get<T>()` miss-path: construct the instance OUTSIDE any lock, then take `unique_lock` and race-check — delete the loser's instance if another thread inserted first.
- `GetSystemWithDependencies<T>()` uses the same pattern; `ResolveDependencies` still runs outside the unique_lock because it re-enters `Get<>`.

Verification (all clean, exit 0):
- Single 3-frame: 5s
- Single 10-frame: 20s
- Single 20-frame + reload at 10: 7s
- 10 × 3-frame stress: 10/10 pass, avg 4s
- 5 × (20-frame + reload): 5/5 pass, avg 7.6s

Total 15 cold-start launches, 0 crashes. Original 1-in-3 repro has not recurred.
<!-- SECTION:NOTES:END -->
