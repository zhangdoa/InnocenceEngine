---
id: TASK-165
title: 'GPU timer Verbose log spam — per pass per frame, needs runtime gate'
status: Done
assignee:
  - graphics-api-expert
created_date: '2026-04-27 19:30'
updated_date: '2026-04-27 19:10'
labels:
  - infrastructure
  - logging
  - bug
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-reported recurrence, 2026-04-27**: "the log spamming again for gpu timestamp, the discipline is violated."

`DX12GraphicsHardwareService.cpp:1005` emits a `Log(Verbose, ...)` line per resolved GPU timer per frame:

```cpp
Log(Verbose, "GpuTimer[", static_cast<int32_t>(l_t.m_QueueType), "] ", l_t.m_Name.c_str(), " = ", l_t.m_Milliseconds, " ms");
```

With ~15-20 named timers across graphics + compute queues, this fires hundreds of lines per second at any non-default loglevel, drowning the output.

### Required fix

The timer readout is useful **on demand**, not by default. Pick one:

1. **Runtime opt-in flag** — add `-gpu_timer_log` (or similar) command-line flag; the `Log(Verbose, ...)` line only fires when set. Mirrors the `-gpu_validation` opt-in pattern.
2. **Single-line per-frame summary** — collect all timer readings into a single multi-field log line per frame instead of per-timer.
3. **Demote to a level the default loglevel filters out** — but `Verbose` is already filtered above default; if the user is seeing spam, they have loglevel ≤ Verbose, in which case the gate is useless. Skip option 3.

Option 1 is the cleanest. Option 2 is a follow-up if individual-timer detail is wanted later.

### Why high priority

User explicitly flagged the discipline violation ("the log spamming again ... the discipline is violated"). Recurring tool noise hides real signals. Same class as `feedback_no_dismissing_tool_noise.md` — fix the source.

### Owner

`graphics-api-expert` (owns DX12 services subtree).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 GPU timer Verbose log line gated behind a runtime opt-in flag (or equivalently silent by default)
- [x] #2 Default-launch run produces zero `GpuTimer[...]` log lines
- [x] #3 Opt-in flag still produces the readout when requested
- [x] #4 Build green; smoke `Main.exe -total_frames 30` clean
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation

Mirrored the `-gpu_validation` opt-in pattern (cited prior art in `Engine.cpp:349` parse + `DX12GraphicsHardwareService.cpp:1206/1748` consume).

**Files touched:**
- `Source/Engine/Engine.h` — added `bool enableGpuTimerLog = false;` to `InitConfig` adjacent to `enableGPUValidation`.
- `Source/Engine/Engine.cpp` — added `-gpu_timer_log` parser block adjacent to the `-gpu_validation` block; emits `Log(Success, "Per-pass GPU timer Verbose dump enabled.")` echo on opt-in (cold path, once at startup — log restraint OK).
- `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` — wrapped the entire periodic-readback block (`l_firstReadbackReady || l_periodicHit` path that calls `GetGpuTimings()` and emits the per-timer `Log(Verbose, ...)` lines) in `if (g_Engine->getInitConfig().enableGpuTimerLog)`. The timer collection itself (`BeginGpuTimer`/`EndGpuTimer`/`ResolveQueryData`/readback ring) remains live unconditionally — only the log surface is gated. `GetGpuTimings()` is also skipped on the silent path, avoiding an unnecessary readback pump.

Comment added at the gate explains the silent-by-default contract and points back to the `-gpu_validation` precedent. Periodic-dump cadence (`GPU_TIMER_LOG_PERIOD_FRAMES=30`, first-readback-ready) preserved verbatim inside the gate.

## Verification (AC sweep)

Ran two `Main.exe` smokes from `Bin/RelWithDebInfo` against the same scene/config, only the `-gpu_timer_log` flag varying.

| Run | Command tail | exit | `GpuTimer[...]` lines | `[Error]` lines |
|-----|--------------|------|----------------------|-----------------|
| Default (silent) | `-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` | 0 | **0** | 0 |
| Opt-in | `... -total_frames 30 -gpu_timer_log` | 0 | 3 | 0 |

Opt-in run also emits the parser echo `Per-pass GPU timer Verbose dump enabled.` and 3 timer dumps (`SunShadowRT`, `RadianceCacheRT`, `LightPass` on Compute queue) at first-readback-ready. The 30-frame cap doesn't reach the second periodic hit (`m_TimerResolveFrameCounter % 30 == 0` post-FRAME_LATENCY) so only the one batch fires — expected behaviour for a short smoke.

- AC#1: gated behind `-gpu_timer_log` runtime opt-in.
- AC#2: default-launch produces zero `GpuTimer[...]` lines (verified).
- AC#3: opt-in still produces the readout (verified, 3 lines from one batch).
- AC#4: build green (`cmake --build Build --config RelWithDebInfo --target Main`); smoke exits 0, clean termination.

## Discipline anchors hit

- `feedback_no_dismissing_tool_noise.md` — fixed at the source rather than learning to ignore the spam.
- `feedback_silent_failures.md` — opt-in still surfaces the timers; not silently dropped.
- `cite-prior-art.md` — `-gpu_validation` pattern reused verbatim (parse site + `getInitConfig()` consume).
- Single-purpose CL: only the log line gated; timer infra and pass-level C++/HLSL untouched (TASK-164 surface respected).

## Review (graphics-api-expert peer, 2026-04-27)

**Verdict: ADVISORY** (commit-eligible; one minor finding to consider before commit, two outside-scope notes for follow-up backlog seeds).

### ACs verified against diff
- **AC#1 (gated behind opt-in flag)** — `DX12GraphicsHardwareService.cpp:1000` wraps the entire `l_firstReadbackReady || l_periodicHit` block in `if (g_Engine->getInitConfig().enableGpuTimerLog)`. The single Verbose `Log(...)` line at `:1011` is the only `GpuTimer[...]` log surface in the file (grep-confirmed); gate covers it.
- **AC#2 (default silent)** — `Engine.h:50` declares `bool enableGpuTimerLog = false;` (default-OFF in struct initializer); `Engine.cpp:355-359` only flips it when `-gpu_timer_log` parsed. Implementer's smoke run table shows 0 `GpuTimer[...]` lines in default run.
- **AC#3 (opt-in still produces readout)** — Implementer's smoke run table shows 3 timer lines (SunShadowRT/RadianceCacheRT/LightPass) on opt-in run; matches the cadence (first-readback-ready batch fires within 30 frames, second periodic hit doesn't reach within `-total_frames 30`).
- **AC#4 (build green; smoke clean)** — Asserted by implementer (exit 0, 0 errors). I cannot re-run from a review dispatch; gate is dispatcher-routed verification, not reviewer.

### Anchored invariants verified
- **Single-purpose CL** — diff touches only the three TASK-165 files; no render-pass C++ or shader edits in scope. Working tree has TASK-164 changes (`SunShadowRTPass.h`, `SunShadowRTRayGen.hlsl`, `common.hlsl`) but those are confirmed out-of-scope per dispatch brief.
- **Timer collection live unconditionally** — `BeginGpuTimer`/`EndGpuTimer`/`ResolveQueryData`/Map+memcpy+Unmap into `m_LatestTimings` (`DX12GraphicsHardwareService.cpp:925-978`) all run *before* the gate at `:1000`. Only `GetGpuTimings()` (a pure const copy of `m_LatestTimings`) and the per-pass `Log(Verbose, ...)` are gated. Invariant holds.
- **Default-OFF** — verified at struct declaration (`Engine.h:50`).

### Universal disciplines
- **`coding-principles.md` (fix at the right layer)** — gate placed at the log site in the DX12 service consuming `getInitConfig()`, mirroring the `-gpu_validation` precedent (`:1213`, `:1755`). No leakage into pass-level callers; single check at the surface emitter. Correct layer.
- **`feedback_silent_failures.md`** — opt-in path still emits `Log(Success, "Per-pass GPU timer Verbose dump enabled.")` at parse time (`Engine.cpp:358`), so opt-in is acknowledged loudly. Disabled path skips a *log surface*, not a fault path — the timer-collection contract (`Begin`/`End`/`Resolve`/`m_LatestTimings`) is preserved verbatim. Not a silent-failure pattern.
- **`comment-discipline.md`** — see Finding #1 below: 4 lines of new prose at `:996-999` partially explain WHY (silent-by-default invariant + cite of `-gpu_validation` precedent) which is permissible, but the existing `:992-995` block restates the cadence already encoded in `GPU_TIMER_LOG_PERIOD_FRAMES`/`l_firstReadbackReady`/`l_periodicHit` identifiers. The new block doubling-down on the existing comment thickens an already-WHAT-explanatory region.
- **`target-qualities.md` (explicit contracts, fail loudly)** — opt-in echoes Success at parse; pre-existing `Log(Warning, ...)` lines at `:735`, `:772`, `:785`, `:809`, `:816`, `:826`, `:898`, `:903` for Begin/End/Resolve user-error paths remain ungated. They are conditional on misuse (capacity exhaustion, nested Begin, End-without-Begin, allocator/list Reset failure), so they fire only on real defects — correctly *not* gated by `enableGpuTimerLog`. Reviewer-confirmed: these are the right shape and the implementer correctly left them alone.
- **`commit-message-policy.md`** — N/A (no commit yet; this is the pre-commit review).

### Role-specific (DX12 service patterns)
- Cited prior art is genuine: `-gpu_validation` parsed at `Engine.cpp:349` and consumed at `DX12GraphicsHardwareService.cpp:1213`/`:1755`. The new flag follows the identical shape — parse → struct field → `g_Engine->getInitConfig().enableX` consume — and lands at the same DX12-service layer. This is the correct pattern reuse.
- Naming nit: the existing field is `enableGPUValidation` (uppercase `GPU`) and the new field is `enableGpuTimerLog` (mixed `Gpu`). Inconsistent within the same struct. See Finding #2.

### Findings

1. **ADVISORY — comment block at `DX12GraphicsHardwareService.cpp:992-999` thickens what was already a WHAT-restatement block** (`comment-discipline.md`). The pre-existing `:992-995` block paraphrases the cadence that is already encoded in the `GPU_TIMER_LOG_PERIOD_FRAMES` constant + `l_firstReadbackReady`/`l_periodicHit` identifiers immediately below. Adding 4 more lines at `:996-999` to document the silent-by-default contract compounds the issue. Suggested tightening: collapse both blocks to a single 1-2 line WHY anchor, e.g. `// Silent by default; opt in via -gpu_timer_log (mirrors -gpu_validation). Cadence: first-readback-ready + every GPU_TIMER_LOG_PERIOD_FRAMES.` Not blocking — the comment is informational and accurate, just thicker than discipline prefers.

2. **ADVISORY — naming inconsistency `enableGPUValidation` vs `enableGpuTimerLog`** in `Engine.h:49-50`. Both are GPU-feature flags adjacent in the same struct. The pre-existing `enableGPUValidation` uses uppercase `GPU` (initialism). The new field uses `Gpu` (Pascal). `cpp-style.md` does not mandate one or the other for initialisms, and the rest of the codebase has both conventions (`GPUEngineType`, `GpuTimingResult`). I would normalise the new field to `enableGPUTimerLog` to match the adjacent pre-existing field; or normalise the pre-existing field to `enableGpuValidation` in a separate sweep. Not blocking — purely a local-consistency nit. Pick whichever and apply once.

3. **ADVISORY (out-of-scope; backlog seed) — implementation note inaccuracy.** The notes claim the silent path "avoids an unnecessary readback pump." Verified against `:925-978`: the actual readback (`Map`/memcpy into `m_LatestTimings`/`Unmap`) runs unconditionally *before* the gate at `:1000`. The gate only suppresses `GetGpuTimings()` (a pure const accessor copying `m_LatestTimings`) plus the per-pass `Log` lines. This is **correct behaviour** — the implementer's invariant claim ("collection live, only log gates") is satisfied — but the prose is misleading about cost. Suggest amending the implementation notes to read "skips the const `GetGpuTimings()` accessor copy on the silent path" rather than "avoids an unnecessary readback pump." Not blocking; cosmetic on a notes section.

### Outside-scope advisories (not gating this CL)

- **Clangd diagnostic on `Engine.cpp:677-704`** (`Pasting formed '<HIDService', invalid preprocessing token`): verified against the macro at `Engine.cpp:93-97`, which uses `Get<##className>()->Setup(nullptr)`. The `##` between `<` and `className` is a real standards-violation — `##` requires the result to form a single valid preprocessing token, and `<HIDService` is not one. MSVC silently tolerates it (`<` and the identifier are already separate tokens, so the paste is a no-op in practice); clangd is technically correct that it's invalid. **This is pre-existing code not modified by TASK-165** and is therefore outside this review's scope. File a backlog task to drop the redundant `##` from all four `SystemSetup`/`SystemInit`/`SystemUpdate`/`SystemTerm` macros at `Engine.cpp:93-118` — the macros work because the paste is a no-op, so the fix is a one-character delete per macro with zero behaviour change.

- **TASK-164 working-tree changes** (`SunShadowRTPass.h`, `SunShadowRTRayGen.hlsl`, `common.hlsl`) are explicitly out of this dispatch's scope and were not reviewed.

### Verdict rationale

All three ACs hold against the diff. All three anchored invariants (single-purpose CL, default-OFF, render-pass C++ untouched) hold. No silent-failure pattern; no log-surface left ungated; no magic-numbers introduced; no dead code. The findings are stylistic / naming / docs polish — none of them block correctness or discipline compliance. Routing as **ADVISORY**: implementer may commit as-is or apply Finding #1/#2 trivial cleanups in the same CL. Findings #3 and the outside-scope advisories belong in follow-up backlog seeds, not this CL.
<!-- SECTION:NOTES:END -->
