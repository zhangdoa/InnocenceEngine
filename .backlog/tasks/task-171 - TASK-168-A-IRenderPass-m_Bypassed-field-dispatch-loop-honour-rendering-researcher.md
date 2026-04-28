---
id: TASK-171
title: 'TASK-168-A: IRenderPass m_Bypassed field + dispatch-loop honour (rendering-researcher)'
status: To Do
assignee: []
created_date: '2026-04-26'
labels:
  - rendering
  - tooling
  - diagnostic
dependencies: []
parent_task_id: TASK-168
priority: high
references:
  - Source/Engine/Interface/IRenderPass.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-168 (runtime render-pass bypass toggle).** Owner: `rendering-researcher`.

Engine-side foundation. Lands the per-pass bypass field and the dispatch-loop check, so subsequent editor work (TASK-172) has a real surface to bind against.

### What this subtask delivers

1. **`std::atomic<bool> m_Bypassed { false }` (or equivalent runtime-mutable field)** on `IRenderPass` (`Source/Engine/Interface/IRenderPass.h`). Atomic because the editor IPC writes from a non-render thread; the dispatch loop reads from the render thread.

2. **Dispatch-loop honour** in `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (`PrepareCommandList` call sites at lines 303–388). Bypass-when-true must:
   - Skip the `PrepareCommandList()` call (zero GPU work), AND
   - Leave the pass's owned resources in a defined state (last-frame content is acceptable; downstream consumers must not read undefined memory). If a pass clears its RTV at the top of `PrepareCommandList`, that clear is also skipped — document the contract.

3. **Per-pass bypass-state log** on toggle change: `LogService::Log(LogLevel::Verbose, "RenderPass: <PassName> bypass = ON|OFF")`. Edge-triggered, not per-frame, so the log isn't spammy. The pass-name string lookup is whatever the pass already exposes via `GetRenderPassComp()->m_InstanceName` (or equivalent).

4. **Pass-listing entry point** for the editor: a single function that returns the list of `IRenderPass*` currently dispatched in `ExampleRenderingClient::ExecuteCommandList`, so TASK-172 can iterate without touching every pass header. Minimum-viable shape is fine — a `std::vector<IRenderPass*> GetDispatchedPasses()` on `ExampleRenderingClient` that the editor IPC calls. **Do NOT build a full pass registry** — that is deferred per the parent task spec.

### Project invariants (anchor — read before implementing)

- **60-FPS bar (rendering-researcher manifest, 2026-04-27)**: rendering changes must keep Sponza at ≥60 FPS. The bypass field reads-when-false must cost zero — an `if (m_Bypassed) return;` at the top of `PrepareCommandList` (or a check before the dispatch-site call) is fine. Do not add per-pass overhead beyond the single atomic load.
- **Threading**: the editor IPC writes the flag from a worker thread; render thread reads it. `std::atomic<bool>` with default memory order is the minimum correct primitive. Do NOT use a mutex.
- **Owner-mode**: this subtask owns `Source/Engine/Interface/IRenderPass.h` and `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (rendering-researcher subtree per `Source/ExampleProject/RenderingClient/CLAUDE.md`). Do not edit editor or IPC code — that's TASK-172.

### What this subtask does NOT do

- No editor inspector UI (TASK-172).
- No IPC GET/UPDATE wiring (TASK-172).
- No pass registry refactor.
- No measurement of bypass perf delta (TASK-169 consumes this).

### Validation

- Engine builds clean.
- Smoke test: launch GISponza, confirm baseline render unchanged (m_Bypassed defaults false everywhere).
- Manual flag flip via debugger or temporary code: set `PointShadowGeometryProcessPass::Get().m_Bypassed = true`, confirm the pass is skipped (RenderDoc capture or a temporary log line confirms `PrepareCommandList` not entered).

### Peer review

Per `peer-review-required.md`: after rendering-researcher implements, dispatch a second agent (graphics-api-expert is the natural reviewer — they own the GPU command-recording side and will catch resource-state hazards) to peer-review before commit.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `IRenderPass::m_Bypassed` atomic bool field; default false; zero-cost read when false
- [ ] #2 `ExampleRenderingClient::ExecuteCommandList` honours the flag — bypassed passes skip `PrepareCommandList`
- [ ] #3 Edge-triggered log line on toggle change ("RenderPass: <Name> bypass = ON|OFF")
- [ ] #4 Pass-listing entry point exposed for editor consumption (e.g. `GetDispatchedPasses()`)
- [ ] #5 No regression: GISponza baseline render visually unchanged with all m_Bypassed=false
- [ ] #6 No FPS regression on Sponza (≥60 FPS bar from rendering-researcher manifest)
- [ ] #7 Peer review by graphics-api-expert before commit
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Implementation 2026-04-28** (rendering-researcher / Opus 4.7 1M):

Three files staged (not committed — peer review pending):

1. `Source/Engine/Interface/IRenderPass.h` — added `std::atomic<bool> m_Bypassed { false }` (public, edited from any thread) and `bool m_BypassedPrev { false }` (render-thread-only, used to detect ON↔OFF transitions for edge-triggered logging). `<atomic>` include added.

2. `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — added a single anonymous-namespace template helper `DispatchOrBypass(pass, args...)` that:
   - Loads `m_Bypassed` once with `std::memory_order_relaxed` (single MOV when uncontended).
   - On state change, updates `m_BypassedPrev` and emits `Log(Verbose, "RenderPass: <name> bypass = ON|OFF")` exactly once per transition.
   - Skips `PrepareCommandList` when bypassed; calls it normally otherwise (variadic forward — supports the `(IRenderingContext*)` overloads).
   All `PrepareCommandList` dispatch sites in `ExampleRenderingClientImpl::PrepareCommands` routed through the helper.

3. `Source/ExampleProject/RenderingClient/ExampleRenderingClient.h` + cpp — added `std::vector<IRenderPass*> GetDispatchedPasses() const` returning a flat snapshot in dispatch order. Includes both rasterizer-fork passes and the path-tracer pass (the bypass flag persists across the active toggle, so the editor inspector wants to reach every pass the client owns). One-shot bootstrap passes (BRDFLUT*) included for the same reason.

**Contract documented inline (IRenderPass.h header comment)**: when bypassed, last-frame contents persist; any RTV clear at the top of `PrepareCommandList` is also skipped, so downstream consumers must tolerate stale reads.

**Verification evidence**:
- Build: `MSBuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo` clean (no warnings touching the new code, `Main.exe` + `RenderTest.exe` + `TestSuite.exe` produced).
- Smoke (defaults all-false): `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30` → engine terminated cleanly, zero `RenderPass: ... bypass` log lines emitted (AC #5 + log-spam invariant satisfied).
- Flag-flip test (temporary verification block, since reverted): set `PointShadowGeometryProcessPass::Get().m_Bypassed = true` at frame 5, false at frame 10 → log captured exactly two lines:
  ```
  [Verbose][...DispatchOrBypass] RenderPass: PointShadowGeometryProcessPass bypass = ON
  [Verbose][...DispatchOrBypass] RenderPass: PointShadowGeometryProcessPass bypass = OFF
  ```
  Confirms AC #1, #2, #3 (edge-triggered, no per-frame spam, name resolved correctly from `m_InstanceName`).

**Performance posture (AC #6)**: zero-cost-when-false is structurally guaranteed — single `std::atomic<bool>::load(memory_order_relaxed)` + one branch per dispatch site (≤22 atomic loads per frame). User's downstream perf measurement (TASK-169) consumes this; no measurement done here per scope.

**AC #7 (peer review by graphics-api-expert) is pending.** Per `peer-review-required.md`, dispatcher will dispatch the reviewer; commit happens only after PASS.

### Iteration 2 fix (2026-04-28, rendering-researcher / Opus 4.7 1M)

Addresses both BLOCKED findings from the review-#1 block.

**Finding 1 — `ExecuteCommands` lifecycle defect.** Iteration 1 routed only `PrepareCommands` through `DispatchOrBypass`. `ExecuteCommands` independently iterates passes and unconditionally called `Execute` + `SignalOnGPU` + downstream `WaitOnGPU`, so a bypassed pass would submit a CL whose per-frame `Reset/Close` cycle was skipped — DX12 EXECUTION_ERROR, plus downstream consumers blocked forever on never-emitted fences.

Fix: option (A) per the reviewer's recommendation. Two new helpers in the same anonymous namespace as `DispatchOrBypass` (cite-prior-art — same shape, same memory ordering, no new abstractions):

- `IsBypassed(IRenderPass& pass)` — relaxed atomic load of `m_Bypassed`. Used to gate every `if (Pass::Get().GetStatus() == ObjectStatus::Activated)` block in `ExecuteCommands` so bypassed passes also skip `Execute` / `SignalOnGPU`. Silent (no logging — that stays in `DispatchOrBypass` to keep edge-trigger single-source).
- `WaitIfActive(pass, queueType, semaphoreType)` — gates `WaitOnGPU` on both `GetStatus() == Activated` AND `!IsBypassed(pass)`. A wait on a bypassed producer would block forever; this elides it. Argument order mirrors `GraphicsHardwareService::WaitOnGPU` exactly. Subsumes the existing `if (UpstreamPass::Get().GetStatus() == Activated) WaitOnGPU(...)` patterns at LightPass / GI fan-in / GIDenoise / GIFilter / GIFilterVertical and uniformly extends to all 24 cross-pass `WaitOnGPU` call sites (the in-block `WaitOnGPU(l_renderPass, ...)` self-waits don't need the helper — they're already inside the now-gated block).

`IRenderPass.h` contract comment expanded to reflect "no record AND no execute" — was previously phrased as "skip dispatch" which the reviewer correctly flagged as ambiguous about CL lifecycle vs resource content.

**Finding 2 — count-off.** Implementation Notes claimed "22 dispatch sites." Actual count from grep is 26 (eight Radiance/GI passes alone, plus 18 others). Phrasing updated to "all `PrepareCommandList` dispatch sites in `ExampleRenderingClientImpl::PrepareCommands`" — removes the brittle hard-coded number.

**Verification.**

- Build: `MSBuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo` clean (Main.exe + RenderTest.exe + TestSuite.exe produced, no warnings on touched code).
- Defaults-all-false smoke (`Main.exe -gpu_validation -total_frames 30`): zero `RenderPass: ... bypass` log lines, the only D3D12 messages are the pre-existing PointShadow ExecuteIndirect GBV warning and the TASK-163 readback ResourceBarrier ERROR — both pre-existing and acceptable per brief. No NEW errors.
- Flag-flip test (temporary `PointShadowGeometryProcessPass::Get().m_Bypassed.store(true)` at frame 0, since reverted): exactly one `RenderPass: PointShadowGeometryProcessPass bypass = ON` log line at startup; engine ran 30+ frames without hanging (proves `LightPass`'s `WaitOnGPU` on PointShadow's renderpass became a no-op — otherwise the queue would have blocked indefinitely on a fence that was never signalled); no NEW D3D12 errors; no DRED device removal. The previous iteration's EXECUTION_ERROR hazard is gone.

**Files staged (uncommitted)**:
- `Source/Engine/Interface/IRenderPass.h` — contract comment updated.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — added `IsBypassed` + `WaitIfActive` helpers; gated all `ExecuteCommands` per-pass blocks; replaced all cross-pass `WaitOnGPU` calls with `WaitIfActive`.

Ready for review #2.

## Review (graphics-api-expert peer, 2026-04-27)

**Verdict: BLOCKED**

### Blocking findings

- **`ExecuteCommands` ignores `m_Bypassed`; bypassing any pass that records on Graphics or Compute submits an unrecorded command list.** `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:416–823`. The diff routes only `PrepareCommands` through `DispatchOrBypass`. `ExecuteCommands` independently iterates passes (`OpaquePass:495`, `LightPass:703`, `LuminanceHistogramPass:777`, et al.) and unconditionally calls `l_hwService->Execute(commandList, …)` + `SignalOnGPU` + downstream `WaitOnGPU`. `PrepareCommandList` is what calls `CommandListBegin → Open → ID3D12GraphicsCommandList::Reset()` (verified in `Source/Engine/Services/DX12/DX12FrameManagementService.cpp:21–77`) and `CommandListEnd → Close()` (verified in `OpaquePass.cpp:157, 178`). When bypass is true the per-frame `Reset/Close` cycle is skipped, but `Execute` still submits the CL — which is either still in the closed-but-stale state from the previous frame, or in an indeterminate state because the per-frame command allocator was rotated out from under it. D3D12 explicitly forbids resubmitting a CL whose allocator has been recycled; debug layer will fire `EXECUTION_ERROR` and the runtime is allowed to remove the device. This violates anchored invariant #3 ("Resources stay defined") at the GPU-state-machine level — the brief's "last-frame contents are acceptable" was a *resource-content* statement, not a *command-list lifecycle* statement, and the implementer extended it incorrectly. Discipline citation: `target-qualities.md` (explicit contracts, fail loudly), graphics-api-expert role (`threading-contracts.md`, command-list lifecycle). **Fix shape**: the helper has to either (a) be lifted to wrap both record-and-execute paths so bypass cleanly elides the entire pair, or (b) keep `PrepareCommandList` always firing (it is internally a no-op on `m_Bypassed`) and bypass only the GPU-side dispatch inside the recording — but per the brief invariant "read-when-false costs zero" this defeats the design intent. The cleanest layer is (a): a per-pass bypass check that elides both the recording site *and* the matching `Execute/Signal/Wait` block in `ExecuteCommands`. Either approach also has to handle downstream consumers that `WaitOnGPU` on the bypassed pass's renderpass — those waits will block forever if `Signal` is also elided. The `OpaquePass m_PostCLState = CrossQueueExit::ToCommon` exit-barrier (`OpaquePass.cpp:38`) compounds this: bypassing OpaquePass leaves the GBuffer color/depth in whatever state the *previous frame* left them on the graphics queue, but the next-frame compute consumers were architected (post-TASK-161) to assume the producer emitted the cross-queue transition this frame. The brief's invariant #3 effectively cannot hold for any pass with `m_PostCLState != Default` without explicit handling.

- **`GetDispatchedPasses()` lists 26 passes, Implementation Notes claim "22 dispatch sites".** `ExampleRenderingClient.cpp:1183–1228` and helper-call grep yield 26 sites (lines 326, 336, 337, 340, 344, 345, 347, 348, 350-357 = 8, 359, 361, 363, 365, 367, 369, 375, 405, 407, 411). The Implementation Notes paragraph and the implementer's enumeration ("the eight Radiance/GI passes" — there are eight, plus 18 others) miscount to 22. Not a code defect, but it is the kind of count-off the implementer is least primed to see (`peer-review-required.md` § "Defects implementer is least primed to see") and the count appears in the Implementation Notes as a self-validation claim. Discipline citation: implementer self-validation gap.

### Advisory findings

- **`m_BypassedPrev` lives on `IRenderPass` itself, mutated by the dispatch helper.** `Source/Engine/Interface/IRenderPass.h:45` and `ExampleRenderingClient.cpp:69–73`. The header comment ("Render-thread-only mirror … never read or written by any other thread") is a thread-affinity contract baked into the interface. Acceptable, but the field's owner is *the dispatch loop*, not the pass — a side map keyed on `IRenderPass*` inside the anonymous namespace would keep the interface clean. Not blocking; the comment makes the contract explicit and the cost is one byte per pass. Discipline citation: `coding-principles.md` (fix at the right layer) — borderline, surfaces for awareness.

- **Bypass is silent for required-by-contract passes.** A user who bypasses `FinalBlendPass` or `OpaquePass` from the editor inspector will see an empty / corrupted screen and no diagnostic beyond the edge-triggered Verbose log (which is below the default log level in many configurations). Not a defect of this CL — the toggle is by design — but worth surfacing as a TASK-172 concern: the editor should either tag certain passes "structural" or render an on-screen badge when any pass is bypassed. Discipline citation: `feedback_silent_failures.md` adjacent.

- **Atomic ordering choice is correct.** `IRenderPass.h:41` (`std::atomic<bool> m_Bypassed`) + `ExampleRenderingClient.cpp:70` (`load(std::memory_order_relaxed)`). On x64 a relaxed load of a naturally-aligned aligned `bool` compiles to one MOV (no fence, no LOCK prefix). Editor-IPC writes (worker thread) propagate to the render thread within ~1 cache-line refresh (~tens of ns); the "next frame is bypassed" UX expectation holds. No `compare_exchange`, no pair-load patterns. Anchored invariants #1 and #2 satisfied at the primitive level (the lifecycle defect above is the actual problem). Discipline citation: `threading-contracts.md` — this part is correct.

- **`GetDispatchedPasses()` order matches `PrepareCommands` dispatch order (verified line-by-line) and includes one-shot passes (`BRDFLUT*`) per the inline comment.** `ExampleRenderingClient.cpp:1183–1228`. AC #4 met by shape. Discipline citation: AC #4 satisfied.

- **Edge-triggered logging fires on toggle, not on ON-only.** `ExampleRenderingClient.cpp:71–77`. The condition `l_bypass != pass.m_BypassedPrev` correctly catches both ON→OFF and OFF→ON; smoke-test evidence in Implementation Notes shows two log lines for a frame-5 ON / frame-10 OFF cycle, which matches. Discipline citation: AC #3 satisfied at the recording layer (but see blocking finding — the actual behaviour the user observes will be wrong).

### Items checked clean

- Universal disciplines: `comment-discipline.md` — header comments on `m_Bypassed` and `m_BypassedPrev` document *why*, not *what*; no explanatory comments masking unclear code. `coding-principles.md` — helper in anonymous namespace at the dispatch layer is the right home. `target-qualities.md` — *was* explicit-contract for content; failed for command-list lifecycle (see blocking).
- Role-specific: x64 single-MOV atomic load confirmed; `relaxed` ordering is the correct primitive. `m_InstanceName.c_str()` resolution is type-correct (`FixedSizeString<128>::c_str()` exists at `FixedSizeString.h:26`). No magic numbers, no hardcoded pass-name strings, no stale references to `SunShadowGeometryProcessPass` (TASK-138-removed) in the listing.
- ACs structural shape: #1 atomic field default false ✓; #3 edge-triggered log shape ✓; #4 flat vector entry point ✓; #6 zero-cost-when-false at the recording site ✓ (lifecycle defect changes the actual cost when bypassed, but that's the blocking issue, not the FPS bar).

### Path forward

The lifecycle defect is the only true blocker. Options the implementer can choose between:

- **(A)** Move the bypass check into a per-pass wrapper that wraps both `PrepareCommandList` *and* the matching `Execute`/`Signal`/downstream-`Wait` block in `ExecuteCommands`. Cleanest. Requires touching `ExecuteCommands` and reasoning about each pass's downstream consumers. The graphics-api-expert reviewer prefers this.
- **(B)** Fold bypass into `PrepareCommandList` itself: have it always run, but become a no-op record (open + immediately close, no work between). `Execute` still submits a valid empty CL. Resource state contract still holds (no clears, no transitions); cross-queue exit barriers from prior frames persist. Cheaper to implement but defeats the "zero-cost when bypassed" intent and trades the defect for a harder-to-reason-about state-machine edge.
- **(C)** Document the lifecycle constraint and gate the editor inspector to only allow bypass on passes that are safe to bypass under the current architecture (no `m_PostCLState`, no downstream `WaitOnGPU` consumers). This pushes the constraint up to TASK-172 and constrains the toggle's surface; the foundation here would still be wrong.

Recommend (A). Re-dispatch review after the implementer picks.

## Review #2 (graphics-api-expert peer, 2026-04-27)

**Verdict: ADVISORY**

Both BLOCKED findings from review #1 resolved. One advisory raised on the cross-queue exit barrier (OpaquePass `m_PostCLState = CrossQueueExit::ToCommon`) — GPU-safe under DX12 implicit-promotion-from-COMMON semantics, but the bypass contract in `IRenderPass.h` and the iteration-2 notes do not call it out. Recommend a one-line caveat addition before commit; the dispatcher may also choose to accept-as-is given the editor surface is the user (zhangdoa) only and TASK-172 owns the inspector UX.

### Finding 1 — `ExecuteCommands` lifecycle defect: **RESOLVED**

Verified line-grounded:

- Helpers `IsBypassed(IRenderPass&)` and `WaitIfActive(IRenderPass&, GPUEngineType, GPUEngineType)` added in the same anonymous namespace as `DispatchOrBypass` at `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:89` and `:98`. Same shape, same `std::memory_order_relaxed` load — cite-prior-art satisfied.
- Every `Activated`-checked block in `ExecuteCommands` is gated with `!IsBypassed(...)`. Grep tally:
  - `GetStatus() == ObjectStatus::Activated` appears 28 times in the dispatch path: BRDFLUT one-shot (lines 450–453, gated on both BRDFLUT + BRDFLUTMS), GPUPathTracer dispatch (476), the 23 main-pass blocks 495–827, the LightPass GIFilterVertical fallback (738), plus the audit-dump SunShadowRT branch at 1060 which is outside the dispatch path.
  - 27 dispatch-path occurrences (28 minus the audit dump); every one of those has `&& !IsBypassed(...)` adjacent (verified by grep matching `IsBypassed\(` 27 times in dispatch context: lines 452–453, 476, 495, 503, 514, 522, 536, 552, 570, 589, 607, 625, 643, 660, 672, 684, 700, 708, 727, 738, 755, 763, 780, 799, 818, 827).
- All cross-pass `WaitOnGPU` calls replaced with `WaitIfActive`. Verification: the 19 remaining raw `WaitOnGPU(l_renderPass, ...)` calls in the file (lines 461, 484, 545, 562, 580, 599, 617, 635, 653, 667, 679, 693, 719, 748, 773, 790, 811, 840) all reference the *outer* pass's own `l_renderPass` local — these are the graphics-compute ping-pong self-waits inside the now-gated block. Correct per the brief: a self-wait inside a gated block elides automatically with the block. The specific cross-pass sites flagged in review #1 — LightPass (lines 733–741), GIDenoise (645–646), GIFilterHorizontal (662), GIFilterVertical (674) — all use `WaitIfActive` now.
- The conditional `if (UpstreamPass::Get().GetStatus() == Activated) WaitOnGPU(...)` patterns previously at GIDenoise / GIFilterHorizontal / GIFilterVertical / LightPass are subsumed: `WaitIfActive` does the status-check internally, so the surrounding `if` blocks collapsed cleanly.
- LightPass GIFilterVertical fallback (lines 738–741) preserves the original semantics: if vertical filter is live, wait on it; otherwise wait on denoise. The outer condition uses `&& !IsBypassed(GIFilterVerticalPass::Get())` to drive the fork — minor stylistic redundancy with the inner `WaitIfActive` (which would also no-op on bypass) but functionally correct.
- Bypass-on-producer no-op behaviour: when a producer is bypassed, its `Execute` / `SignalOnGPU` block is elided AND every consumer's `WaitIfActive(producer, ...)` returns without queuing the wait. The CL `Reset/Close` lifecycle stays consistent (the pass's command list is neither opened nor submitted), and consumer queues advance without blocking on a never-emitted fence. The implementer's flag-flip test (PointShadowGeometryProcessPass = ON, 30+ frames without hang) is the right shape of evidence — `LightPass`'s wait on PointShadow's renderpass is line 734, and that path runs every frame.

### Finding 2 — Count-off: **RESOLVED**

Implementation Notes iteration-2 block (line 119 of the task md) states `"all PrepareCommandList dispatch sites in ExampleRenderingClientImpl::PrepareCommands"` — the brittle "22" claim is gone, replaced with the correct collective phrasing. The "26" actual count is acknowledged in the iteration-2 fix narrative (line 119) — accurate.

### Cross-queue caveat (`OpaquePass m_PostCLState = CrossQueueExit::ToCommon`): **ADVISORY**

GPU-safe analysis:

- The TASK-161 cross-queue exit barrier is recorded inside `DX12FrameManagementService::CommandListEnd` (`Source/Engine/Services/DX12/DX12FrameManagementService.cpp:520–524`). `CommandListEnd` runs only as part of `PrepareCommandList`, which bypass elides — so when OpaquePass is bypassed, the `RENDER_TARGET → COMMON` transition is never recorded.
- DX12 implicit promotion rules save us: when the previous active frame ran OpaquePass, the GBuffer RTs were transitioned to `D3D12_RESOURCE_STATE_COMMON` at end-of-CL. They stay in COMMON across any number of bypassed frames. When a downstream non-bypassed compute consumer (RadianceCacheReprojection / SunShadowRT / SSAO / GIDenoise / LightCulling / LightPass) reads them, DX12 implicitly promotes from COMMON to `NON_PIXEL_SHADER_RESOURCE` for non-pixel shader reads. No barrier needed; no debug-layer error.
- Engine-side `m_CurrentState` tracker: `ChangeRenderTargetStates` is also gated by `PrepareCommandList`, so when bypass elides recording it also elides the engine-side state mutation. Tracker stays at COMMON, GPU stays at COMMON — consistent.
- The brief's "stale reads tolerated" invariant therefore holds for OpaquePass: downstream consumers see last-frame's GBuffer pixels (in COMMON state, auto-promoted on read). No EXECUTION_ERROR class hazard.

Why this is still an advisory:

- The header comment at `Source/Engine/Interface/IRenderPass.h:32–46` documents resource-content staleness ("downstream consumers must tolerate stale reads") and CL lifecycle ("neither Reset nor Closed nor submitted"), but does NOT mention the cross-queue exit-barrier interaction. A future contributor (or a future agent) reading the contract could reasonably ask "does bypass break TASK-161's design?" — the answer is "no, because of DX12 implicit promotion from COMMON, but only if the bypassed pass had run at least once before so the GBuffer is currently in COMMON" — and the contract should say so.
- Edge case: if OpaquePass is bypassed *from frame 0* (engine-startup default flag-flip), GBuffer is in whatever-state the resource creation left it (usually COMMON for committed resources, but architectures should not rely on this). Practical impact: zero (no one bypasses OpaquePass at startup — TASK-172 will gate that), but the contract is silent.
- Recommendation: add one paragraph to the `m_Bypassed` header comment at `IRenderPass.h:33–43` along the lines of: *"Note: when a pass with `m_PostCLState = CrossQueueExit::ToCommon` (TASK-161) is bypassed, the producer's end-of-CL transition to COMMON is also elided. Cross-queue consumers rely on the resource already being in COMMON from the last active frame; DX12 implicit promotion handles the read auto-transition. Bypassing such a pass before it has ever run is undefined."*
- Alternative: document this in the TASK-171 Implementation Notes as a known-caveat subsection, and let TASK-172 gate the editor inspector to disallow bypassing structural passes.

This is below the BLOCKED bar because: (1) the GPU is safe in the realistic startup-state-then-flip scenario, (2) the user is the only consumer of this feature, (3) TASK-172 owns the surface that exposes the toggle. But it's above the "items checked clean" bar because the contract documentation in the IRenderPass header is the natural home for this caveat and it's missing.

### Items re-verified clean (no regression of review #1 passes)

- Logging single-sourced: edge-triggered transition log lives only in `DispatchOrBypass` (lines 70–77). `IsBypassed` and `WaitIfActive` are silent. No spam, no double-firing.
- Atomic memory ordering: `m_Bypassed.load(std::memory_order_relaxed)` at lines 70 and 91. No fences, no `compare_exchange`. On x64 a single MOV. Editor-IPC writes from a worker thread propagate within ~tens of ns — "next frame is bypassed" UX expectation holds.
- Helper placement: all three helpers (`DispatchOrBypass`, `IsBypassed`, `WaitIfActive`) live in the anonymous namespace inside `ExampleRenderingClient.cpp` (lines 59–106). No additions to pass bodies, no additions to `IRenderPass` interface beyond the two fields.
- Resource-state contract: bypass content invariant ("last-frame contents persist; RTV clear at top of `PrepareCommandList` is also skipped") is preserved. The new invariant ("no Execute = no barrier emission") is correct at the GPU level; the missing piece is documenting it in the IRenderPass header (see advisory above).

### Path forward

- If the dispatcher wants strict closure: ask the implementer to append the cross-queue caveat paragraph to the `m_Bypassed` header comment in `IRenderPass.h` (one-line edit, no logic change), then commit.
- If the dispatcher wants to ship now and defer documentation: accept the advisory, commit iteration 2 as-is, file a follow-up task to document the caveat in IRenderPass.h *and* in the TASK-172 inspector UX brief (so the editor surface knows which passes are "structural" and warns on bypass). Lower friction; same effective safety given the single-user surface.

No code edits performed in this review.
<!-- SECTION:NOTES:END -->
