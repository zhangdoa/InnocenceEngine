---
id: TASK-165
title: 'GPU timer Verbose log spam — per pass per frame, needs runtime gate'
status: To Do
assignee: []
created_date: '2026-04-27 19:30'
labels:
  - infrastructure
  - logging
  - bug
dependencies: []
priority: high
references:
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
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
- [ ] #1 GPU timer Verbose log line gated behind a runtime opt-in flag (or equivalently silent by default)
- [ ] #2 Default-launch run produces zero `GpuTimer[...]` log lines
- [ ] #3 Opt-in flag still produces the readout when requested
- [ ] #4 Build green; smoke `Main.exe -total_frames 30` clean
<!-- AC:END -->
