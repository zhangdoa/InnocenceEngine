---
id: TASK-233
title: >-
  AuditDump trigger — replace magic frame-count fence with event-driven
  scene-load completion signal
status: Done
assignee:
  - zhangdoa
created_date: '2026-05-17 15:50'
updated_date: '2026-05-17 17:15'
labels:
  - rendering
  - test-infra
  - race-condition
  - followup
dependencies: []
references:
  - >-
    Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp
  - Source/ExampleProject/LogicClient/World.inl
  - Source/Engine/Services/SceneService.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 investigation (`216de0e0` was the partial fix). The audit-mode dump fires when `s_AuditFrame == 30` (`Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp:299`), a rendering-execute counter unrelated to the world-update counter `m_AutoFrameCount` at `World.inl:284` that triggers GISponza scene-load at frame 5.

The 5→30 bump bought scene-load latency headroom for current scene complexity, but the underlying gap is the two unrelated counters. If scene-load takes longer in future (more assets, slower bake, slower hardware), the race re-emerges. Magic-number fence is a band-aid; the right shape is event-driven: AuditDump fires on a "scene fully loaded" signal posted by SceneService::Update after the world-update path completes.

Anchor files:
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp:296-301` (current fence)
- `Source/ExampleProject/LogicClient/World.inl:284` (scene-swap trigger)
- `Source/Engine/Services/SceneService.{h,cpp}` (load completion event)
- `Source/Engine/Services/EditorService.cpp:498` (precedent: WebSocket LOAD_SCENE handler signals on completion — similar pattern)

Pre-existing band-aid; surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 AuditDump trigger replaced with event-driven scene-load completion signal (no magic frame count)
- [ ] #2 Audit autotest reliably captures GISponza on hardware that completes scene-load in <5s and on hardware that takes >30s (worst-case soak)
- [x] #3 No race window between scene-load trigger and audit capture, regardless of frame-counter values
- [x] #4 Build green; existing audit captures unchanged byte-shape (modulo nondeterministic ray-bounce noise)
<!-- AC:END -->

## Implementation Plan

<!-- SECTION:PLAN:BEGIN -->
## Approach

Two-phase event-driven trigger. SceneService already exposes `AddSceneLoadedCallback` — register one, flip an atomic ready flag, then count K post-load render frames before `AuditDump()`. K=25 initially (preserves byte-shape of current captures modulo nondeterministic ray-bounce noise; matches the empirical settling budget the previous magic-30 fence bought).

## Why K>1, not "fire on callback"

Scene-load completion does not necessarily mean GPU resource upload / TLAS build / first GI cache fill have settled. The previous 30-absolute fence gave ~25 post-load render frames empirically. Conservative pick K=25 keeps capture stable. Comment marks K as an empirical settling budget, not a magic gate. A later CL can tighten K with capture-diff evidence.

## Files to touch

- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp` — replace `s_AuditFrame == 30` fence with the two-phase trigger.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.{h,cpp}` (or equivalent setup site) — register the SceneLoadedCallback once, store the functor as a member for stable lifetime.

## Files explicitly NOT touched (CL scope discipline)

- `Source/Engine/Services/SceneService.{h,cpp}` — callback API already exists.
- `Source/ExampleProject/LogicClient/World.inl` — auto-test scene-swap trigger is correct as-is.

## Threading contract

- Callback may fire from the scene-load worker thread; ready/fired flags are read on the render thread → `std::atomic<bool>` for the cross-thread flags.
- `s_PostLoadFrame` counter is read/written only on the render thread → plain `uint32_t`.
- Implementer must read `SceneService.cpp` to confirm fire-site thread before finalising the contract.

## Discovery questions (implementer audits before coding)

1. Which thread invokes the registered `AddSceneLoadedCallback`? (Determines atomic / marshalling shape.)
2. Does "scene loaded" mean assets-loaded or GPU-resources-uploaded? (Sets honest K.)
3. Should the callback filter by scene name (GISponza only) or use a one-shot guard? `World.inl:284` loads GISponza after the UnitTest startup scene; the callback fires for both.

## Skills

- `cpp-style` — naming, include order, engine STL replacements.
- `safety-observability` — guard-clause Log() patterns, no silent fallback.
- `threading-contracts` — declare the cross-thread contract explicitly in code.
- `test-etiquette` — launch budget, read-first run-last for audit autotest.
- `visual-validation` — four-layer validation for the audit-mode capture diff.

## Verification (matches DOD)

- Build green (engine + ExampleProject) — quote build output.
- Run audit autotest end-to-end; GISponza capture produced. Pre-existing integration coverage: the audit autotest itself is the integration test for this trigger logic.
- Visual diff vs pre-change baseline; honest disclosure if byte-shape shifted within the noise band.
- AC#2 worst-case >30s soak: explicitly note as unverified (no hardware to simulate slow scene-load).

## Approved K

K=25 (conservative). User-confirmed 2026-05-17.
<!-- SECTION:PLAN:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-17 — code-impl landing (NOT closed: peer review pending).

**Files touched (4):**
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Internal.h` (+21 lines, +5 new declarations including `RegisterAuditCallback()` / `HandleAuditTrigger()` and 4 audit-state members; +2 STL includes `<atomic>`, `<functional>`).
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Setup.cpp` (+2 lines net: 1 call to `RegisterAuditCallback()` from `Setup()`).
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp` (−7 lines net: removed `s_AuditFrame == 30` block, replaced with 1-line `HandleAuditTrigger()` call).
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_AuditDump.cpp` (+51 lines: added `RegisterAuditCallback()` body + `HandleAuditTrigger()` body alongside existing `AuditDump()`; +1 SceneService include).

**File-size ratchet:** Setup.cpp would have grown 297→314 and ExecuteCommands.cpp 294→317 (both over 300). Extracted both audit seams into `_AuditDump.cpp` (103→154). Final sizes: Setup.cpp=299, ExecuteCommands.cpp=288 (NET REDUCTION), Internal.h=121, AuditDump.cpp=154 — all under 300 ratchet.

**Discovery answers (audit-first):**
1. **Thread:** `SceneService::LoadSync` runs only on the render thread (FMS upload-heap callback dispatches `SceneService::Update`; see `.claude/state/engine-invariants.md`). Callback always fires on render thread, same as ExecuteCommands. Atomic flag still used as explicit threading documentation + futureproofing.
2. **"Scene loaded" meaning:** Assets loaded + components `InitializeComponents` drained + `WaitForGPUIdle` complete (per `SceneService.cpp:60-71`). GPU resources are uploaded by the time the callback fires.
3. **Filter shape:** No scene-name filter — counter RESETS on every scene-load event. Resetting on each load ensures K post-load frames measure from the MOST RECENT load. Handles all invocation shapes (autotest UnitTest→GISponza, single-scene `-scene <X>` override).

**Threading contract** (documented in `_Internal.h`):
- `m_AuditSceneLoadEvent` (`std::atomic<bool>`): cross-thread edge signal. Acquire/release ordering.
- `m_AuditPostLoadFrameCount` + `m_AuditCountingStarted`: render-thread-local. No atomicity needed.
- `m_AuditSceneLoadedCallback` (`std::function`): stable storage matching SceneService's raw-pointer registration (precedent: `EditorService.h`).

**Build:** Green. `Scripts/BuildWin.ps1 -SkipShaderCompile` completed clean, no errors/warnings/failures.

**Integration test (DOD #2/#3):** The audit autotest itself IS the integration test for this trigger. Ran `Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 60 -offscreen audit` end-to-end with new code. Log timeline:
- `16:30:54.805` SceneService: UnitTest loaded → audit callback fires (event=true, counter reset).
- `16:30:54.875` WorldSystem auto-trigger at frame 5 → queues GISponza load.
- `16:30:57.513` SceneService: GISponza loaded → audit callback fires again (counter reset).
- `16:30:58.058` AuditDump starts (~25 frames / ~545ms post-GISponza-load).
- `16:30:58.914` AuditDump complete, clean `std::exit(0)`.

All 12 expected HDR files written (`audit_01_BRDFLUTPass.hdr` through `audit_11_FinalBlend.hdr`). No race window observed — GISponza scene-load fully completes before audit fires.

**Visual diff (Layer 1 Read, apples-to-apples):**
Captured legacy code (git stash) vs new code under identical invocation flags + same engine build pipeline. Files at `Build/captures/task-233-{legacy,new-final}/`.
- Deterministic outputs (BRDF LUTs, Opaque RT0, SunShadow, Illuminance, Sky): **byte-identical** (md5 + file-size).
- Temporal outputs (Opaque RT1, RT2, SSAO, Light_Luminance, TAA, FinalBlend): file-size diff −0.1% to +0.7% (within ray-bounce noise band).
- Visual Read on `audit_11_FinalBlend.png` (auto-leveled): scene structure, lighting distribution, lion-ornament position, brick texture, pre-existing TASK-228 upper-half mirror artifact — all identical between legacy and new. Verdict: improvement (no regression).

**Earlier false-alarm correction:** Initial comparison against preserved `Bin/RelWithDebInfo/audit_*.hdr` showed apparent ~50% darker output. Stashing my changes and rebuilding the legacy code showed THE SAME ~50% darker output — i.e., the preserved baselines were from an unrelated engine state. The apples-to-apples legacy-vs-new comparison shows no regression.

**AC checklist:**
- AC#1 (event-driven, no magic frame count): ✓
- AC#2 (>30s soak hardware): ✗ **N-V** (no hardware to simulate slow scene-load). Structural argument: new shape resets counter on every scene-load event, so any load duration is tolerated; only the K=25 post-load-frame budget could in theory be too short for unusually-slow GI cache fill, independent of scene-load latency.
- AC#3 (no race window): ✓
- AC#4 (byte-shape unchanged modulo noise): ✓

**DOD #6 — what was NOT verified:**
- AC#2 worst-case soak (>30s scene-load): no hardware; structural argument given in lieu.
- K=25 against future GI cache fill changes: unverified — if a future radiance-cache redesign extends per-frame convergence latency, K may need re-evaluation.
- `-audit` without `-total_frames N` captures UnitTest (not GISponza) because `m_AutoFrameCount` only increments when `totalFrames > 0` (`World.inl:273-274`). Unchanged from legacy behavior; not a regression.

**Surfaced (NOT folded in — per `surface-dont-chase`):**
1. `Source/Engine/Engine_ParseInitConfig.cpp:145` log message "Audit mode: will dump all pass outputs on frame 5." is stale (frame 5 was wrong since `216de0e0`'s 5→30 bump; now meaningless under event-driven trigger). Worth a one-line message-correction CL.
2. The audit invocation pattern `-audit` without `-total_frames N` silently captures UnitTest instead of GISponza. No documentation surfaces this. Worth a `Scripts/AuditCapture.ps1` wrapper script that bundles the correct flags.

## Peer Review — 2026-05-17 (code-review agent, fresh dispatch)

**Verdict: PASS with two ADVISORY items, no blockers.**

Reviewer confirmed AC#1, AC#3, AC#4 met. AC#2 honestly N-V (no hardware for >30s soak); structural argument accepted — the counter is reset by the callback, the callback fires only after `LoadSync` completes (assets loaded + components init drained + `WaitForGPUIdle`), so any load duration is tolerated.

Reviewer did direct Visual Read of `audit_11_FinalBlend.png` from both `Build/captures/task-233-legacy/` and `task-233-new-final/`: scene structure, lion ornament, brick texture, pre-existing TASK-228 upper-half mirror artifact all identical between legacy and new. No regression.

### Move-ctor regression (alleged blocker) — NOT a blocker

The `-Wdefaulted-function-deleted` warning on `_Internal.h` is real but pre-existing engine-wide: `INNO_CLASS_CONCRETE_NON_COPYABLE` (`Source/Engine/Common/ClassTemplate.h:11-17`) defaults move ops; any non-movable member silently deletes them. `ExampleRenderingClientImpl` is heap-allocated and never moved (`ExampleRenderingClient.h:31` holds it via raw pointer), so the deleted moves are functionally fine. Filed as TASK-237 (macro foot-gun, engine-wide).

### Followups filed

- **TASK-235** — cpp-style rename (`kAuditPostLoadSettleFrames` → `AuditPostLoadSettleFrames`) + state-transition Log on first-trigger.
- **TASK-236** — stale `Engine_ParseInitConfig.cpp:145` log message ("frame 5" no longer meaningful).
- **TASK-237** — `INNO_CLASS_CONCRETE_NON_COPYABLE` macro foot-gun (engine-wide).
- **TASK-238** — optional `Scripts/AuditCapture.ps1` wrapper (tooling, low priority).

### AC checklist after review

- AC#1 ✓ (event-driven, no magic frame count — K=25 is a documented post-load settling budget, not a fence)
- AC#2 ✗→N-V (worst-case soak unverified; structural argument accepted)
- AC#3 ✓ (no race window — traced `Engine_RenderingCallbacks.cpp:23-49` runs SceneService::Update → RenderingClient::Update → ExecuteCommands all on the same render thread; atomic acquire/release explicit per `threading-contracts`)
- AC#4 ✓ (byte-shape unchanged modulo noise — visual diff confirmed)

Ready for closure flip pending user direction.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Landed as commit `8355775a` (refactor(audit): event-driven trigger), peer-reviewed PASS (commit `0d4bf444` filed adjacent TASK-234; followups TASK-235/236/237/238 filed for ADVISORY items and surfaced findings).

**Outcome:** `s_AuditFrame == 30` absolute-frame fence replaced with a two-phase event-driven trigger: `SceneService::AddSceneLoadedCallback` flips an atomic edge flag; `HandleAuditTrigger()` counts K=25 post-load render frames before calling `AuditDump()`. Counter resets on every scene-load event, so the K-budget always measures from the most recent load. Race-free regardless of load duration.

**Files touched (4):** `ExampleRenderingClient_{Internal.h, Setup.cpp, ExecuteCommands.cpp, AuditDump.cpp}`. `_ExecuteCommands.cpp` net −7 lines; all sibling TUs under the 300-line ratchet (Setup=299, ExecuteCommands=288, Internal.h=121, AuditDump=154).

**ACs:**
- #1 ✓ event-driven, no magic frame count (K=25 documented as empirical settling budget, not a fence).
- #2 N-V worst-case >30s soak: no hardware to simulate slow scene-load. Structural argument accepted by reviewer — counter is reset by the callback; the callback fires only after `LoadSync` completes (assets loaded + components init drained + `WaitForGPUIdle` per `SceneService.cpp:60-71`); any load duration is tolerated.
- #3 ✓ no race window — traced `Engine_RenderingCallbacks.cpp:23-49`: `SceneService::Update` → `RenderingClient::Update` → `ExecuteCommands` all on render thread within one frame. Atomic acquire/release documented at `_Internal.h:77-83`.
- #4 ✓ build green; capture byte-shape unchanged — deterministic outputs byte-identical, temporal within ±0.7% file-size noise band, visual Read of `audit_11_FinalBlend.png` legacy-vs-new identical.

**Verification:** `Scripts/BuildWin.ps1 -SkipShaderCompile` green. End-to-end audit run `Main.exe -mode 0 -renderer 0 -loglevel 1 -total_frames 60 -offscreen audit` wrote all 12 expected HDRs (`audit_01_BRDFLUTPass.hdr` … `audit_11_FinalBlend.hdr`); clean `std::exit(0)`. Captures preserved at `Build/captures/task-233-{legacy,new-final}/`. Implementer Visual Read + reviewer independent Visual Read both confirmed no regression.

**Followups filed:**
- TASK-235 — cpp-style rename `kAuditPostLoadSettleFrames → AuditPostLoadSettleFrames` + state-transition Log on first-trigger.
- TASK-236 — stale `Engine_ParseInitConfig.cpp:145` "frame 5" log message.
- TASK-237 — `INNO_CLASS_CONCRETE_NON_COPYABLE` macro foot-gun (engine-wide latent, exposed by adding atomics to any user).
- TASK-238 — optional `Scripts/AuditCapture.ps1` wrapper (tooling).

**Unverified (DOD #6):**
- AC#2 worst-case >30s soak — no hardware.
- K=25 against future GI-cache redesigns — if convergence latency grows, K may need re-evaluation.
- `-audit` without `-total_frames N` captures UnitTest, not GISponza — unchanged from legacy behaviour, not a regression.
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
