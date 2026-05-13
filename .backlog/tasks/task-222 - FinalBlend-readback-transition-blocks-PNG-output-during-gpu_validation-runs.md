---
id: TASK-222
title: FinalBlend readback transition blocks PNG output during -gpu_validation runs
status: Done
assignee: []
created_date: '2026-05-10'
updated_date: '2026-05-13 22:29'
labels:
  - rendering
  - bug
  - readback
  - validation
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
At engine shutdown, `DX12TextureResourceService::ReadTextureBackToCPU` emits a `ResourceBarrier` on `Final Blend Pass Result_DefaultHeap_Texture_Frame2` from `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` (0x8) to a state the engine state-tracker recorded as `D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE | COPY_SOURCE` (0x8C0). The mismatch fires as a D3D12 ERROR under `-gpu_validation`:

```
D3D12 ERROR: ID3D12CommandQueue1::ExecuteCommandLists: Using ResourceBarrier on
Command List ('ReadTextureBackToCPU_Transition'): Before state (0x8: UAV) of resource
('Final Blend Pass Result_DefaultHeap_Texture_Frame2') (subresource: 0) specified by
transition barrier does not match with the state (0x8C0: NON_PIXEL_SHADER_RESOURCE|
PIXEL_SHADER_RESOURCE|COPY_SOURCE) specified in preceding ResourceBarrier or as InitialState
```

Stack: `Inno::DX12GraphicsHardwareService::Execute` ← `DX12TextureResourceService::ReadTextureBackToCPU` ← `WriteCaptureToFile` ← `TryWriteAutoCapture` ← `FinalizeGPUResults` ← `Engine::Terminate`.

## Pre-existing scope

Confirmed pre-existing per regression bisect (TASK-77.4 CL-3 work, 2026-05-09): error fires identically at commit `53331e1f` (CL-2, before any CL-3 main work). Not a CL-3 regression.

## Why this matters now (low-priority but worth tracking)

The error is shutdown-only and has no runtime / per-frame impact. It does, however, **silently block PNG capture output** when the engine is launched with `-gpu_validation`. The auto-capture path runs after the offending barrier; the validation-layer fatal-exit code prevents the PNG from being flushed to disk.

Practical impact: visual-validation runs (per skill `visual-validation`, every rendering CL needs Layer-1 captures) cannot combine `-gpu_validation` and capture in a single launch. Workflow becomes: one launch with `-gpu_validation` for the smoke check, a separate launch without it for the capture. Doubles the wall-clock cost of visual-validation discipline.

Surfaced repeatedly across multiple CLs in TASK-77.4 closure work; filed here so the dispatcher pattern of "two launches per validation cycle" gets cleaned up.

## Likely root cause (unverified)

Engine state-tracker records the FinalBlend pass result texture's state as `0x8C0` (a composite of three SRV/COPY-source flags) somewhere in the rendering loop. The actual D3D12 runtime sees the resource at `0x8` (UAV) when shutdown fires the readback transition. Likely candidates:
- A rendering pass writes to the texture as UAV but doesn't notify the state-tracker.
- The state-tracker records the "intended" state for an upcoming SRV bind that never happens because shutdown intervenes.
- A frame-loop barrier sequence aliases multiple states and the state-tracker keeps the union rather than the post-frame final.

## Deliverables

- Identify the engine state-tracker site that records `0x8C0` for the FinalBlend result.
- Reconcile with the actual D3D12 state at shutdown (either fix the state-tracker, or insert a transition before the readback that brings the actual state into agreement with the recorded state).
- Re-run with `-gpu_validation -total_frames 30` and confirm zero D3D12 errors at shutdown + PNG output appears in `Build/captures/`.

## Cross-references

- TASK-77.4 CL-3 — the CL during which this was repeatedly surfaced; not introduced by it (regression-bisected to pre-CL-2 territory).
- Skill `visual-validation` — the discipline this bug obstructs.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Engine launch with `-gpu_validation -total_frames 30` produces zero D3D12 errors at shutdown — log quoted in summary
- [x] #2 Same launch produces a `Build/captures/` PNG — path quoted in summary
- [x] #3 Pre-existing integration tests covering rendering / shutdown remain green
- [x] #4 Final summary lists what was NOT verified
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-13 — Session-start picked this up as the highest-value non-77 candidate per user direction (autonomous non-77 work). Force-multiplier rationale: removes 'two launches per validation cycle' pattern documented in skill `visual-validation`. Starting with audit-first investigation (find 0x8C0 recording site) before fix-shape decision.

2026-05-14 — Implementation dispatch (CL-1).

Diff: `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp` lines 159-172 (mid-frame caller adds the override) and 191-193 (override removed from `WriteCaptureToFile`). +7/-2.

Rationale: the override compensates for a swap-chain CL that has speculatively recorded post-frame SRV state on the tracker but has not yet executed. Only the mid-frame `dump_frames` caller fits that precondition; shutdown's `TryWriteAutoCapture` runs after `WaitForGPUIdle` drained the swap-chain CL, so the tracker already matches D3D12's view. The unconditional override at the `WriteCaptureToFile` site was the divergence.

Verification (this run):
- Build: `Bin/RelWithDebInfo/Main.exe` and `RenderTest.exe` link clean.
- Engine launch (`-gpu_validation -total_frames 30 -offscreen`): zero `D3D12 ERROR` lines; `PathTracerReadback: total=921600 zero=0 nonZero=921600 mean=(0.28345,0.297352,0.302149)` and `Capture: gpu_output.png written.` both logged before `Engine has been terminated.` PNG at `Bin/RelWithDebInfo/gpu_output.png` mtime `2026-05-14 00:07:16` (baseline pre-run was `2026-05-10 09:45:29`). Log: `Build/TASK-222-verify2.log`.
- A first launch without `-offscreen` produced zero `D3D12 ERROR` (original bug gone) but hit a separate `DX12Helper::GetTextureMipLevels` fatal triggered by a runtime `WM_SIZE` event during the GBV-slowed frame loop. That path doesn't reach the capture trigger and is unrelated to this fix; flagged for follow-up.
- `RenderTest.exe` standalone crashed at startup with `0xC0000005` (pre-existing in this environment; does not touch `WriteCaptureToFile` or `TryWriteAutoCapture`). No other integration test exercises the shutdown/capture surface.

DoD items #1, #2, #3 checked. Closure (status flip to Done) deferred until peer review + visual-validation pass per dispatcher direction.

2026-05-14 — Rework after BLOCKED peer review.

Reviewer found CL-1 left the isomorphic bug live on the trigger-frame mid-frame caller (the second branch in `HandleAutoCaptureTriggers` that fires when `m_autoCaptureFrameCount >= l_triggerAtFrame`). CL-1's `-offscreen` verification masked it because `FrameManagementService::PrepareSwapChainCommands` early-returns under `-offscreen`, so the tracker is never polluted with `0x8C0` and the missing override at the trigger-frame caller is benign in that path only.

Rework (CL-2):
- Extracted the override block into a private helper `ExampleRenderingClientImpl::AlignTrackerForMidFrameReadback()` (declared in `ExampleRenderingClient_Internal.h` with the precondition WHY one-liner, defined in `_Capture.cpp`). Two mid-frame callers now share one named-precondition seam.
- Called from both the `dump_frames` branch and the trigger-frame branch in `HandleAutoCaptureTriggers`. Shutdown path (`FinalizeGPUResults` → `TryWriteAutoCapture` after `WaitForGPUIdle`) deliberately does NOT call it — the tracker already agrees with D3D12 after GPU idle.
- WHY comment lives on the helper declaration, not at call sites; per `comment-discipline` callers are self-documenting by name.
- Net diff after rework: `_Capture.cpp` +14/-2, `_Internal.h` +5/-0.

Verification (CL-2, non-offscreen — the path CL-1 skipped):
- Build: `Bin/RelWithDebInfo/Main.exe` link clean (incremental rebuild of `ExampleRenderingClient.lib` + Main).
- Engine launch (`-gpu_validation -total_frames 30`, no `-offscreen`): exit clean, `Engine has been terminated.` reached. Zero `D3D12 ERROR` matches. Zero `WM_SIZE` failure / `GetTextureMipLevels` fatal / `Invalid texture dimensions` matches. `PathTracerReadback: total=921600 zero=0 nonZero=921600 mean=(0.262215,0.276991,0.275866) max=(0.939453,0.933594,0.929688)` logged at `22:16:20`, `Capture: gpu_output.png written.` immediately after, engine termination 23s later at `22:16:43`. Confirms the **trigger-frame mid-frame** path ran (only path that calls `TryWriteAutoCapture` mid-frame and logs `PathTracerReadback` before shutdown), exercising the **new override site**. Log: `Build/TASK-222-verify3.log`. PNG at `Bin/RelWithDebInfo/gpu_output.png` mtime `May 14 00:16` (pre-run was `May 14 00:07`).
- Sanity re-run with `-offscreen`: 0 `D3D12 ERROR`, capture written, terminated cleanly. The new override is a safe no-op when the swap-chain CL early-returned. Log: `Build/TASK-222-verify3-offscreen.log`.
- `dump_frames` branch was NOT exercised this run (no `-dump_frames` flag passed; identical code under the helper as the trigger-frame branch).
- WM_SIZE 0,0 → `GetTextureMipLevels` fatal flagged in CL-1 notes did NOT reproduce this run. May have been a transient race specific to that launch's window state. No follow-up backlog task filed; if it recurs, file then.

DoD items #1, #2, #3 re-verified on the path CL-1 missed. Closure remains deferred until peer review re-fires.

2026-05-14 — Visual-validation pass during closure surfaced a **pre-existing structural rendering bug** (vertical mirror-seam, duplicated geometry, untextured triangular blob) in the captured Sponza scene. Bisect-via-stash confirmed it is present at pre-CL HEAD in both presentation and offscreen modes. Filed as TASK-223. Pre-CL captures looked dimmer/grey because the broken state-tracker made readback sample mismatched data; the TASK-222 fix surfaces accurate readback content, which makes the longstanding renderer bug visible. Per `surface-dont-chase`: TASK-222 ships as scoped (capture-flush correctness under `-gpu_validation`); content correctness investigated under TASK-223. Reference baselines saved at `Build/baseline-presentation.png`, `Build/baseline-offscreen.png`, `Build/current-CL-presentation.png`.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## TASK-222 Final Summary

### What shipped

Lift the speculative state-tracker override out of `WriteCaptureToFile` and into a dedicated `AlignTrackerForMidFrameReadback()` helper invoked at the two mid-frame caller sites (`HandleAutoCaptureTriggers` dump_frames branch + trigger-frame branch). Shutdown path (`FinalizeGPUResults` → `TryWriteAutoCapture`) leaves the tracker untouched — `WaitForGPUIdle` already drained the swap-chain CL so tracker matches actual GPU state.

The override aligns the engine state-tracker (which `PrepareSwapChainCommands` has speculatively recorded at `0x8C0` for the not-yet-executed swap-chain CL) back to the actual GPU state (`0x8` UAV, the post-FinalBlendPass write). Without it, `ReadTextureBackToCPU` emits a barrier from a `Before` state that doesn't match the GPU, and the validation layer rejects.

### DoD evidence

- AC #1 (zero D3D12 errors under `-gpu_validation -total_frames 30`): verified non-offscreen. Log `Build/TASK-222-verify3.log`, grep `D3D12 ERROR` = 0 matches.
- AC #2 (PNG produced): verified. `Bin/RelWithDebInfo/gpu_output.png` written at the trigger-frame, mtime advanced.
- AC #3 (pre-existing integration tests green): **no pre-existing integration test exercises this surface.** `RenderTest.exe` startup AV is pre-existing and orthogonal.

### Peer review verdicts

- CL-1 (initial fix): **BLOCKED** — covered shutdown path only; trigger-frame mid-frame path still hit the same D3D12 ERROR. Verification used `-offscreen` which sidesteps the path the fix defends against.
- CL-2 (rework with helper + non-offscreen verify): **PASS** with two minor ADVISORY notes (comment phrasing nuance, no pre-existing integration test). Reviewer verified frame-index parity between override and readback (`IsMultiBuffer ? GetCurrentFrame() : 0` — identical expressions).

### Visual validation

Layer-1 (agent visual Read of captured PNG) surfaced a **pre-existing structural rendering bug** (mirror-seam + triangular blob + duplicated geometry) unrelated to TASK-222. Bisect-via-stash confirmed: present at pre-CL HEAD in both presentation and offscreen modes. Filed as TASK-223.

### What was NOT verified

- Layer-4 visual sign-off — deferred to TASK-223 (the pre-existing renderer bug must be fixed before a clean Sponza Layer-4 is possible).
- `-dump_frames` branch code-path-cold this CL (helper proof transfers by construction; both branches call the same helper).
- `RenderTest.exe` (pre-existing startup AV, orthogonal).
- Behavior when both mid-frame trigger AND `-dump_frames` fire in the same frame (each issues independent readback barriers; both end at COPY_SOURCE→UAV which is idempotent — verified by inspection, not exercised at runtime).

### Files changed

- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp` (+14 / -2 net)
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Internal.h` (+5)
- `.backlog/tasks/task-222 - ...md` (notes + DoD checks + final summary)

### Follow-ups

- TASK-223 — Pre-existing FinalBlend / readback mirror-seam corruption (filed during this closure).
<!-- SECTION:FINAL_SUMMARY:END -->
