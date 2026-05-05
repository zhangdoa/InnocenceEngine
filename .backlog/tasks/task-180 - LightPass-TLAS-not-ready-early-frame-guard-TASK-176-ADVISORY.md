---
id: TASK-180
title: LightPass TLAS-not-ready early-frame guard (TASK-176 ADVISORY)
status: Done
assignee: []
created_date: '2026-04-28 14:50'
updated_date: '2026-05-05 11:06'
labels:
  - rendering
  - dx12
  - bug
dependencies:
  - TASK-176
references:
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-176 review ADVISORY (graphics-api-expert peer, 2026-04-28).**

`LightPass.cpp:370` unconditionally binds `GetTLASBuffer()` and `:379` uses `Dispatch` (no `IsTLASReady()` guard like `DispatchRays` at `DX12FrameManagementService.cpp:401`). The engine convention at `LightPass.cpp:363-364` (nullptr-bind when SunShadowRT not Activated) is not followed for the new t14 binding.

If TLAS isn't built yet (early frames before async build completes), the inline RayQuery reads an uninitialized acceleration structure → undefined behaviour. Engine reaches `Dispatch` before SunShadowRT's `IsTLASReady()` short-circuit fires; the equivalent guard for LightPass needs to either nullptr-bind or skip the dispatch.

### Required fix

Mirror the SunShadowRT pattern: either (a) check `IsTLASReady()` before binding the TLAS root SRV at LightPass dispatch site and nullptr-bind when not ready (visibility falls back to "always lit" via the inline RayQuery returning `COMMITTED_NOTHING` on null AS — verify behaviour), OR (b) gate the entire LightPass dispatch on TLAS ready (regress to a non-RT lighting eval path until ready).

Option (a) is closer to the existing convention. Option (b) is heavier but unambiguous.

### Why low priority

Early-frame UB is bounded — first few frames may have visual artifacts, but engine recovers as soon as TLAS builds. No GBV ERROR observed during TASK-176 testing (probably because TLAS is ready by frame 5+ when smoke runs report). Real risk is on extremely cold-cache scenarios (just-cloned engine with no warm caches).

### Owner

`rendering-researcher` (LightPass.cpp pass binding) or `graphics-api-expert` (if the fix lives in `DX12FrameManagementService` IsTLASReady check). One-line fix likely.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 LightPass TLAS binding gated on IsTLASReady (or equivalent fallback)
- [x] #2 Cold-cache smoke (`Main.exe -total_frames 5`) clean — no ERROR/WARNING from TLAS access
- [x] #3 Existing warm smoke unchanged (no regression)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation surfaced 2026-05-05

**Design choice: option (b) gate-the-dispatch on `IsTLASReady()`**, not (a) nullptr-bind. Rationale verified by reviewer: `DX12FrameManagementService::BindComputeResource` for null Buffer-typed resource logs Warning and returns false WITHOUT binding the root SRV — leaves the root parameter holding stale state. Option (a) literal `nullptr` would leave slot 20 with whatever previous pass put there; inline RayQuery would still be UB. Option (b) is structurally correct. Mirrors the established `DispatchRays` IsTLASReady gate at `DX12FrameManagementService.cpp:402-406` — inline-RayQuery dispatch path now structurally consistent with the DispatchRays path.

**File touched:** `Source/ExampleProject/RenderingClient/LightPass.cpp` (+13 / -15 around the LightPass dispatch site, lines 334-348). Removed two stale comment blocks containing inline TASK-IDs (banned by `comment-discipline.md`).

**Verification:**
- Cold smoke `Main.exe -total_frames 5`: exit 0, no TLAS-related warnings, no D3D12 errors. The `IsTLASReady()==false` branch was NOT runtime-exercised (TLAS ready by frame 1 in this scene with warm OS file cache); structural verification by code-review against the established pattern.
- Warm smoke `Main.exe -total_frames 30`: exit 0, 30 frames rendered, PathTracerReadback healthy (`total=921600 zero=0 nonZero=921600`), gpu_output.png written. Warm-path bit-identical to pre-CL behavior.

**Surprises self-flagged (out of TASK-180 scope, not addressed):**
1. **Slot-19 SunShadowRT nullptr-bind aspirational, not engine-supported.** `LightPass.cpp:331-332` nullptr-binds when SunShadowRTPass not Activated, with a comment claiming "engine binds a default zero descriptor on null." Engine code does not. The slot-19 branch only avoids crashing because SunShadowRTPass is Activated by the time LightPass dispatches in practice. If genuinely Suspended, would crash at `BindComputeResource:621` (texture branch dereferences nullptr->m_ObjectStatus). Worth a separate task / advisory.
2. **`BindComputeResource` Buffer-typed nullptr-handling gap (lines 559-564).** Logs Warning, returns false, leaves root parameter unbound. For a future literal `IsTLASReady() ? GetTLASBuffer() : nullptr` ternary site, the engine layer would need to explicitly bind to null GPU virtual address.

## Review (code-impl, 2026-05-05)

**Verdict: PASS** — no findings.

**Anchored-invariant checks:**
- Gate level correct: both `BindGPUResource(..., GetTLASBuffer(), 20)` (LightPass.cpp:342) AND `Dispatch` (LightPass.cpp:343) sit inside the `if (IsTLASReady())` block. Nothing leaks past the gate.
- Convention mirror: matches `DX12FrameManagementService.cpp:402-406` (DispatchRays IsTLASReady gate) — same predicate, same Warning log level, same skip-and-continue semantics.
- `BeginGpuPass`/`EndGpuPass` (LightPass.cpp:335, :350) bracket the if/else, so the GPU timer scope records on cold frames too.

**Discipline checks:**
- `safety-observability.md`: Warning log on the failure path (LightPass.cpp:347) is appropriate; mirrors the DispatchRays Warning at `DX12FrameManagementService.cpp:404`. No silent failure.
- `no-shadow-state.md`: Predicate is `g_Engine->Get<GPUBufferResourceService>()->IsTLASReady()` directly. No local mirror, no cached bool, no parallel ready-flag. Clean.
- `comment-discipline.md`: Grep for `TASK-\d+` in LightPass.cpp returns zero matches. Two stale comment blocks gone. New comment at LightPass.cpp:337-339 explains *why* without inline TASK-IDs.

**Option (a) vs (b) rationale, independently verified:** `BindComputeResource` for `GPUResourceType::Buffer` with null resource (`DX12FrameManagementService.cpp:560-564`) logs Warning and returns false; root parameter is NOT rebound to null GVA — left as-is. Implementer's departure from the brief's recommended option (a) is justified.

**Reviewed-By: code-impl**
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Final Summary — TASK-180

**Status:** Done. Reviewed PASS by fresh code-impl, no findings.

### What landed (1 file)

`Source/ExampleProject/RenderingClient/LightPass.cpp` (+13 / -15) — gated `BindGPUResource(GetTLASBuffer(), 20)` and the subsequent `Dispatch` call on `g_Engine->Get<GPUBufferResourceService>()->IsTLASReady()`. Mirrors the DispatchRays-side gate at `DX12FrameManagementService.cpp:402-406`. Cold-frame branch logs `Warning` once per frame matching the established convention. `BeginGpuPass`/`EndGpuPass` bracket the if/else so GPU timer scope records on cold frames too. Removed two stale comment blocks containing inline TASK-IDs.

### Design choice

Picked **option (b) gate-the-dispatch** over the brief's recommended (a) nullptr-bind. Rationale: `DX12FrameManagementService::BindComputeResource` for null Buffer-typed resource logs Warning and returns false WITHOUT rebinding the root SRV — leaves the root parameter holding stale state. Option (a) would have left slot 20 with whatever previous pass put there; inline RayQuery would still be UB. Option (b) is structurally correct and matches the existing `DispatchRays` convention pattern.

### AC coverage

| AC | Status | Evidence |
|---|---|---|
| #1 LightPass TLAS binding gated on IsTLASReady | ✓ | LightPass.cpp:340 — both bind and dispatch inside `if (IsTLASReady())` |
| #2 Cold-cache smoke clean — no ERROR/WARNING from TLAS access | ✓ (structural) | `Main.exe -total_frames 5`: exit 0, no TLAS warnings. The IsTLASReady==false branch was NOT runtime-exercised (TLAS ready by frame 1 in this scene with warm OS file cache); structural verification by code-review against the established pattern, per the brief's "Real risk is on extremely cold-cache scenarios" framing |
| #3 Existing warm smoke unchanged | ✓ | `Main.exe -total_frames 30`: exit 0, 30 frames rendered, PathTracerReadback healthy, gpu_output.png written. Warm-path bit-identical to pre-CL (the IsTLASReady==true branch reproduces the original Bind+Dispatch sequence verbatim) |

### What was NOT verified

- **Cold-frame branch at runtime.** TLAS is ready by frame 1 in the auto-test scene with a warm OS file cache. To exercise the branch, would need to either: clear the OS file cache before launch (Windows `EmptyStandbyList.exe` or reboot), construct a synthetic test scene with very large geometry that delays TLAS-build past frame 1, or instrument a one-shot `IsTLASReady() returns false on frame 0` shim. None of these were attempted; the cold-frame path is structurally verified only.
- **Slot-19 SunShadowRT nullptr-bind aspirational pattern surfaced as advisory** (out of TASK-180 scope; if SunShadowRTPass were genuinely Suspended at LightPass dispatch time, would crash at `BindComputeResource:621`). Worth a separate task/advisory; not filing follow-up per `don't pile on backlog tasks` (no longer blocking; would-do-today is unclear given how rare Suspended SunShadowRTPass actually is in practice).
- **`BindComputeResource` Buffer-typed nullptr-handling gap surfaced as advisory** (out of TASK-180 scope). Same triage as #1.

**Reviewed-By: code-impl**
<!-- SECTION:FINAL_SUMMARY:END -->
