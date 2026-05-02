---
id: TASK-213
title: >-
  Auto-test infra: cross-binary deterministic three-scene PT capture (root-cause
  TASK-210 carry-forwards)
status: To Do
assignee:
  - '@test-expert'
created_date: '2026-05-02 17:18'
labels:
  - test-infra
  - rendering
  - harness
  - determinism
dependencies:
  - TASK-210
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

TASK-210 (`bf07e7c9`) shipped per-scene capture; its `## Post-closure follow-up` accumulated three carry-forwards documenting cross-binary nondeterminism in the captures themselves:

- Ext.1 (`473c9985`): TLAS-rebuild-vs-asset-load race — telemetry `instances=94 prevCount=86 dirtyTransforms=8` at frame=30, intermittent black PNGs in GISponza toggle=0.
- Ext.2 (`150dcaab`): GISponza camera-position drift across binary launches.
- Ext.3 (`391af203`): cross-binary toggle=0 captures differ MAE 37 across all three scenes; within-binary repeats MAE 0.003.

Blocks the new visual-inspection regime (`ae1f8e31`) for any CL whose visual contribution depends on cross-binary A/B (e.g. TASK-77.1 D1-reversal CL D).

## Root causes (audit, evidence-backed)

**RC-1 — Deferred mesh activation leaks past `SceneService::LoadSync`'s drain.**
`SceneService::LoadSync` calls `MeshResourceService::InitializeComponents()` once (`SceneService.cpp:83`); shared-handle activation-only tasks re-queue (`MeshResourceServiceImpl.cpp:125`) and drain over N subsequent frames inside `FrameManagementService::Update` (`FrameManagementServiceImpl.cpp:161-165`). As components transition `Activated`, `DX12GPUBufferResourceService::UpdateRaytracingInstances` rebuilds the TLAS on instance-count mismatch (`DX12GPUBufferResourceService.cpp:227`).
Evidence (`Build/captures/TASK-77.1-D1-reversal/D/toggle0/gisponza/engine.log`): TLAS rebuilds at `frame=0/1/2/16/30`; frame=30 bumps `instances 86→94` — exactly the dump frame.

**RC-2 — `m_autoCaptureFrameCount` vs `m_FrameCountSinceLaunch` divergence.**
`m_autoCaptureFrameCount` (`ExampleRenderingClient.cpp:1130`) advances only on rendered frames. `m_FrameCountSinceLaunch` (`FrameManagementServiceImpl.cpp:152,222`) advances every Update including loading frames. PT RNG uses `m_FrameCountSinceLaunch` (`PerFrameDataService.cpp:133`); dump filename uses `m_autoCaptureFrameCount`. Cross-binary, the loading-frame count varies (deferred-init drain timing, asset I/O), so RNG state at the named dump frame differs.

**RC-3 — No "scene fully ready" signal gating capture frame counting.**
Dump trigger is a frame-count threshold over a non-stationary state machine. No signal-driven readiness gate: "deferred queues empty AND TLAS instance-count stable for K frames AND `IsLoading==false`".

**RC-4 (downstream of RC-2, not independent)** — PT seed determinism is fine within a process; cross-binary the value-at-dump differs because RC-2.

**RC-5 (ruled out for `-scene` mode)** — async scene-switch race (`World.inl:282`) is suppressed when `-scene` is set; capture script always uses `-scene`.

**RC-6 (GISponza camera drift)** — Sponza's Main Camera Transform is loaded deterministically. With `m_IsTP=false` (EDITOR_MODE default) Player::Update doesn't stomp camera. Without `-camera_orbit`, no per-frame camera animation. With `-camera_orbit`, the orbit angle = f(`m_AutoFrameCount`); `m_AutoFrameCount` runs only post-`Activated` (`World.inl:265`); so cross-binary the orbit-angle-at-dump-frame depends on RC-1's drain timing → drift.

## CL split

| CL | Scope | Discriminator (within-binary) | Closure AC (cross-binary) |
|---|---|---|---|
| **A — Readiness predicate** | Add `FrameManagementService::IsSteadyState()`: deferred queues all empty AND `m_PrevInstanceCount` stable for K=3 frames AND `IsLoading==false`. Log Verbose `Auto-test: steady state reached at frame=N` once. Predicate-only. | Same-binary repeat captures produce same "steady state at frame=N" log per scene. | n/a (predicate-only) |
| **B — Gate dump counting on steady state** | `m_autoCaptureFrameCount` freezes at 0 until `IsSteadyState()` first true; PT-RNG seed = steady-state-relative frame when `-total_frames>0`. | Same-binary, two `TestPathTracerThreeScenes.ps1 -Frames 60 -DumpStart 25 -DumpEnd 29` runs: six pixel-equivalent PNGs per scene (MAE<0.003). | Two FRESH binaries: same MAE<0.003. |
| **C — Pin Sponza camera deterministically** | Three-scene driver defaults to `-camera_orbit "0,8,0"` per scene (yaw=0 fixed). | Same-binary identical camera-position log per scene per dump frame. | Two FRESH binaries: Visual Read confirms same viewpoint. |
| **D — Closure: wire steady-state grep into script** | Script greps `Auto-test: steady state reached`; FAILs if absent. | Script PASS on three scenes, two consecutive same-binary runs. | Cross-binary diff MAE<0.003 per scene per dump frame. |

Each CL surfaces with three-scene captures and follows the visual-inspection regime (`Reviewed-Visually:` footer per `ae1f8e31`).

## Acceptance criteria

- AC-1: Same-binary two-run repeat: six pixel-equivalent PNGs per scene (MAE<0.003).
- AC-2: Cross-binary (two fresh-rebuilds): six pixel-equivalent PNGs per scene (MAE<0.003).
- AC-3: GISponza Visual Read across fresh binaries: same viewpoint per dump frame.
- AC-4: Zero `TLAS rebuild:` log lines in [DumpStart, DumpEnd] window for any scene.
- AC-5: `Auto-test: steady state reached at frame=N` logged once per scene; script fails if absent.
- AC-6: Existing `TestGPUPathTracer.ps1` (default async-switch path) continues to PASS — steady-state gate handles late-binding load too.
- AC-7 (CL D): GISponza toggle 0 frames in the [25,26]-black-window must render real content (not deterministic-black). Root cause: K=3 plateau latches on transient instance-count walk `85→86→87→94`. Fix path: either (a) script-side flap-back-aware dump shifting (consume `Auto-test: steady state lost at frame=N` log markers and shift dump start past max-flap-back), or (b) per-scene K override. CL D author chooses; document in CL D's Implementation Note. Evidence from CL B: `Build/captures/TASK-213-CL-B/{run1,run2}/toggle0/gisponza/gpu_output_002{5,6}.png` are 36970-byte uniform-black PNGs across both runs.

## Out of scope

- TASK-109 reload determinism (UnitTest reload after GISponza).
- TASK-211 manual-screenshot path.
- Refactoring the deferred-init queue to drain in one call.

## Files in scope

- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp`(+`.h`) — predicate, log.
- `Source/Engine/Services/{Mesh,Texture,Material,GPUBuffer}ResourceService.h` — `IsDeferredQueueEmpty()` accessors.
- `Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp` — TLAS-stability accessor.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — gate `m_autoCaptureFrameCount`; reroute PT seed in capture mode.
- `Scripts/TestPathTracerThreeScenes.ps1` — default per-scene `-camera_orbit`; success-grep `steady state reached`; per-scene MAE check across consecutive runs (CL D).

## Risk register

- **R1 — Predicate too eager.** Mitigation: K=3 frames stability + explicit log-on-first-true.
- **R2 — Predicate too lazy.** Mitigation: 120-frame timeout fallback with `Log(Warning,...)`; `-total_frames` remains hard cap.
- **R3 — PT seed change ripples.** Mitigation: visual-validation §3 self-reference compares candidate-vs-candidate, not vs historical baseline.
- **R4 — Default orbit masks regressions visible at scene-file camera.** Mitigation: keep no-orbit option for non-capture sessions; CL C default is script-only.
- **R5 — Unknown within-binary nondeterminism.** Mitigation: surface back if AC-1 fails.
- **R6 — No wall-clock / sleep-based inputs.** Audited: capture paths have none; predicate is signal-driven.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

### CL A — `IsSteadyState()` predicate (read-only signal) — surfaced 2026-05-02

**Files touched:**
- `Source/Engine/Services/FrameManagementService.h` — declared `bool IsSteadyState()`; added rolling state members (`m_LastObservedInstanceCount`, `m_TLASStableFrameCount`, `m_SteadyStateMarkerLogged`, `m_SteadyStateTimeoutLogged`).
- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — implemented the predicate; wired one `(void)IsSteadyState()` call at the end of `Update()` so the rolling-K counter advances and the first-true marker fires.
- `Source/Engine/Services/MeshResourceService.h` — added inline `bool IsDeferredQueueEmpty() const`.
- `Source/Engine/Services/GPUBufferResourceService.h` — added `virtual size_t GetRaytracingInstanceCount() const { return 0; }`.
- `Source/Engine/Services/DX12/DX12GPUBufferResourceService.h` — overrode `GetRaytracingInstanceCount()` to return `m_PrevInstanceCount`.

No script or capture-client changes — the predicate is unconsumed this CL (read-only signal).

**Predicate components and how each is queried:**
1. **Deferred mesh-activation queue empty** — `MeshResourceService::IsDeferredQueueEmpty()` reads `m_DeferredQueue.empty()` directly. New accessor; nothing else exposed it before.
2. **TLAS instance-count stable for K=3 frames** — `GPUBufferResourceService::GetRaytracingInstanceCount()` returns the DX12 backend's `m_PrevInstanceCount` (last value written by `UpdateRaytracingInstances`). FMS owns the K-frame stability window: each `IsSteadyState()` call compares current vs `m_LastObservedInstanceCount`; reset to 0 on change, increment on match. Threshold is `TLASStabilityWindowFrames = 3`. (Default 0 in the base class — backends without raytracing report a stable 0, never blocking the predicate.)
3. **`IsLoading() == false`** — `SceneService::IsLoading()` (already public). Read directly.

**Per-scene first-true frame log evidence** (from `Build/captures/TASK-213-CL-A-steady-state-marker/<scene>/engine.log`):

| Scene     | First-true frame | Instance count | TLAS rebuilds observed                      |
|-----------|------------------|----------------|---------------------------------------------|
| unittest  | frame=4          | 56             | frame=0 (only)                              |
| gitestbox | frame=4          | 22             | frame=0 (only)                              |
| gisponza  | frame=7          | 94             | frame=0/1/2/3 (last rebuild at frame=3)     |

All three log lines have `deferredQueueEmpty=1 tlasStableFrames=3 isLoading=0`. Marker fires exactly once per scene (verified by `grep -c`). No 120-frame timeout warnings fired in any scene.

**K=3 stability window evidence:** GISponza's last TLAS rebuild was at frame=3 (count→94). Marker fires when `m_FrameCountSinceLaunch=7`, with `tlasStableFrames=3`. Trace: frames 4/5/6 are observed as stable (counter=1/2/3), and the marker logs after the FCSL increment at the end of frame 6's Update — i.e. as soon as the K-frame window has actually elapsed. unittest/gitestbox follow the same pattern from frame=0's single rebuild.

**Surprises / advisories surfaced:**

1. **K=3 may be insufficient against the worst observed pattern** — the design pass cited GISponza rebuilds at `frame=0/1/2/16/30` (gap of 14 frames between rebuild bursts). This run observed only `frame=0/1/2/3` — much tighter. K=3 is correct for *this run*; if a future run reproduces the 14-frame-gap pattern, the predicate could go true at frame=5 and then a frame-16 rebuild would invalidate it. CL B/D should either (a) widen K, (b) latch the predicate so once-true-stays-true is not assumed by consumers, or (c) re-evaluate the predicate continuously and respect the *most recent* steady-state-reached marker. **Flap-back log added per peer-review fold-in:** when the marker has already latched and the predicate then loses TLAS-stable state (instance-count change), `IsSteadyState()` emits `Auto-test: steady state lost at frame=N (instanceCount changed M->N)` at Verbose. The first-true marker remains one-shot per session per scene (existing behavior unchanged); the lost-state log is additive so CL B's consumer can see the full flap timeline and pick first-true-latch vs. per-frame-evaluate semantics. **Recommend CL B's design re-examine flap behavior.**
2. **`MeshResourceService::IsDeferredQueueEmpty()` accessor was new** — `m_DeferredQueue` was private with no accessor. Added inline (no engine API calls, safe in header).
3. **`GPUBufferResourceService::GetRaytracingInstanceCount()` is a new virtual** — the count was tracked in DX12-only state (`m_PrevInstanceCount`). Exposed via base-class virtual so the backend-agnostic predicate doesn't reach into DX12 internals. Default 0 keeps the predicate well-defined for hypothetical non-raytracing backends.
4. **Predicate evaluation cadence** — wired into `Update()` after the FCSL increment so each frame produces exactly one observation. This is signal-driven (no `std::chrono`, no `Sleep`) per R6 in the risk register.

**R1 mitigation (predicate-too-eager):** K=3 enforced; documented in code comment with the historical evidence anchor. See advisory 1 above for the residual concern.
**R2 mitigation (predicate-too-lazy):** 120-frame timeout fires `Log(Warning, ...)` if the predicate never goes true. `-total_frames` script-level cap remains the hard-stop. Did not fire in any of the three scenes.
**R6 mitigation (no wall-clock/sleep inputs):** Confirmed by inspection — predicate reads three boolean/integer signals only.

**Build:** RelWithDebInfo clean (Main.exe + RenderTest.exe), no new warnings. Pre-existing `MathHelper.h` `C4003 max` warnings unchanged.

**Three-scene script run:** `Scripts/TestPathTracerThreeScenes.ps1 -Frames 60 -DumpStart 30 -DumpEnd 30 -RunTag TASK-213-CL-A-steady-state-marker` → `OVERALL: PASS`. Per-scene `PASS [unittest]`, `PASS [gitestbox]`, `PASS [gisponza]`. AC-6 (`TestGPUPathTracer.ps1`-style three-scene capture continues to PASS) preliminarily evidenced, though task scope says full closure on AC-1..AC-6 is CL B/C/D's job.

**Reviewer:** test-expert peer (per `peer-review-required.md`'s reviewer-selection rule). Surface-back only; no commit this CL.

### CL B — Gate dump counting on steady state + reroute PT-RNG seed in capture mode — surfaced 2026-05-02

**Files touched:**
- `Source/Engine/Services/FrameManagementService.h` — added `HasReachedSteadyState()` + `GetSteadyStateRelativeFrameCount()` accessors and the `m_FirstSteadyStateFrame` member.
- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — snapshot `m_FirstSteadyStateFrame = m_FrameCountSinceLaunch.load()` at the latch instant; implement `GetSteadyStateRelativeFrameCount()` (returns 0 until latch, then `FCSL - m_FirstSteadyStateFrame`).
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:1130` — gated `m_autoCaptureFrameCount++` on `HasReachedSteadyState()`. No flap-back reset (per CL A reviewer carry-forward).
- `Source/Engine/Services/PerFrameDataService.cpp:133` — in capture mode (`getInitConfig().totalFrames > 0`) the `frameIndex` field of the per-frame CB is sourced from `GetSteadyStateRelativeFrameCount()`; interactive mode keeps `GetFrameCountSinceLaunch()`. The downstream TAA jitter and `radianceCacheHaltonJitter` both pull from the same `frameIndex`, so they automatically inherit the steady-state-relative seed in capture mode.

**Build:** RelWithDebInfo clean (Main.exe + RenderTest.exe). Pre-existing `MathHelper.h` C4003 noise unchanged; no new warnings.

**Same-binary repeat MAE matrix** (`-Frames 60 -DumpStart 25 -DumpEnd 29`, 5 dump frames per scene per toggle per run, 30 frame pairs total):

| scene     | toggle 0 (cache OFF) MAE per frame [25,26,27,28,29] | toggle 1 (cache ON) MAE per frame [25,26,27,28,29] |
|-----------|-----------------------------------------------------|----------------------------------------------------|
| unittest  | 0, 0, 0, 0, 0                                        | 0, 0, 0, 0, 0                                       |
| gitestbox | 0, 0, 0, 0, 0                                        | 0, 0, 0, 0, 0                                       |
| gisponza  | 0, 0, 3.28e-6, 5.68e-6, **7.62e-4**                  | 0, 0, 0, 0, 0                                       |

All 30 same-binary repeat-MAE values are below the 0.003 acceptance threshold. **AC-1 PASS** for the 12-pair / 30-frame-pair interpretation. Toggle 1 is bit-identical across the entire dump window; toggle 0 has small (<8e-4) residual divergence on GISponza frames 27-29 attributable to TLAS-rebuild flap-back during the dump window (see surprise 1 below).

**Toggle 0 vs toggle 1 same-camera A/B MAE** (run1, frame 25 / frame 29):
- unittest: 3.16e-3 / 3.15e-3
- gitestbox: 6.85e-2 / 6.27e-2
- gisponza:  3.61e-1 / 3.28e-1

Toggle 1 is materially different from toggle 0 (cache contribution clearly visible), as expected.

**GISponza toggle 0 black-frame status: PERSISTENT, partially.** Toggle 0 GISponza dump frames 25 and 26 are uniform-black PNGs (36970 bytes each, `Read`-confirmed visually). Frames 27-29 render correctly. **The steady-state-gating reduces but does not fully eliminate the GISponza black-frame issue on the toggle-off side** — see surprise 1.

Toggle 1 GISponza dump frames 25-29 all render correctly (atrium curtains, columns, floor visible across the entire window).

**Visual Read assessment** (per `visual-validation.md` layer 1):
- *unittest, toggle 0/1 frame 0029*: row of PBR material spheres on a flat ground plane, blue-grey sky gradient. Clean, no rings, no NaN, no obvious regression vs. CL A baseline. Toggle 0 and toggle 1 both correct and visually similar (small albedo / shading deltas consistent with cache contribution).
- *gitestbox, toggle 0/1 frame 0029*: Cornell-box-style red/green/grey walls with coloured spots, light-bleed visible. Clean, no artifacts. Toggle 1 has slightly different colour mixing (cache contribution).
- *gisponza, toggle 1 frames 0025/0029*: full Sponza atrium with red/orange curtains, fluted columns, marble floor, heavy PT noise (correct for low-SPP). Clean, no rings, no NaN clipping. Improvement vs. toggle 0 frames 25-26 (which are black).
- *gisponza, toggle 0 frames 0025-0026*: BLACK. Toggle 0 frame 0029: dark Sponza atrium, curtains visible, much noisier than toggle 1 (expected — no cache convergence).

**Verdict: improvement** (CL B fixes the determinism it was scoped to fix; the residual GISponza toggle-0 black-frame surface is a separate root cause flagged below).

**Surprises / advisories surfaced:**

1. **TLAS rebuild flap-back during the dump window — toggle 0 only.** The CL A flap-back log (`Auto-test: steady state lost at frame=N`) fires multiple times in toggle-0 GISponza runs:
   - run1: lost at FCSL=10, FCSL=29, FCSL=32 (instance-count walk 85→86→87→94)
   - run2: lost at FCSL=23, FCSL=32 (different timing, same end-state)

   The K=3 latch is ONCE-TRUE-STAYS-TRUE so the dump-frame counter is stable, BUT the engine itself is still under TLAS reconstruction during dump frames 25-26 (FCSL 31-32). This produces uniform-black readbacks. Same-binary repeats produce the SAME black PNGs (MAE=0 on frames 25-26), so AC-1 is met — the determinism fix is correct; the captures are just visually unusable in the [25,26] window for GISponza-toggle-0.

   **Toggle 1 latches with `instanceCount=94` immediately at FCSL=5 with NO flap-back logs**, suggesting the cache-on path forces a different deferred-init schedule (possibly more eager mesh activation triggered by the cache-bind fence). Toggle 1 captures are clean across the entire window.

   This is the design pass's residual concern about K=3 vs the "frame=0/1/2/16/30" historical rebuild pattern. K=3 is correct for the latch trigger; the issue is that subsequent (post-latch) rebuilds fire INSIDE the dump window. **Recommend CL D widen K, or have CL D's script consume the flap-back log and shift the dump start to "first-true-frame + max(observed-flap-back-frame) + N" rather than literal frame=25.**

2. **GISponza toggle 0 instanceCount at latch differs from CL A.** CL A reported gisponza first-true at FCSL=7 with instanceCount=94. CL B's toggle 0 binary observes first-true at FCSL=6 with instanceCount=85 (then walks 85→86→87→94). Toggle 1 observes first-true at FCSL=5 with instanceCount=94. Three different runs, three different latch values. The K=3 stability check is firing on ANY 3-frame plateau — including transient plateaus mid-walk. **Recommend CL D add a "instance-count not below historical max" check on the latch condition, or consume the flap-back log and re-arm the latch.**

3. **PT-RNG seed change rippled correctly.** Toggle 1 same-binary repeats produce bit-identical output across the entire dump window (15 frames, all MAE=0), confirming the seed-source change to `GetSteadyStateRelativeFrameCount()` is working as designed and producing deterministic noise patterns. R3 (PT-seed-change ripples) is mitigated — candidate-vs-candidate comparison is internal to this CL and matches.

4. **Toggle file reversion noted.** To produce the toggle-1 captures, this CL temporarily flipped `PT_HASH_GRID_CACHE_ENABLED` 0→1 in both `HashGridCacheConstants.h:30` and `GPUPathTracerRayGen.hlsl:34`, then reverted them before surfacing back. The four CL B files modified for this surface-back are exactly:
   - `Source/Engine/Services/FrameManagementService.h`
   - `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp`
   - `Source/Engine/Services/PerFrameDataService.cpp`
   - `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`

   `git diff Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` and `git diff Source/ExampleProject/RenderingClient/HashGridCacheConstants.h` are clean.

**R5 mitigation (unknown within-binary nondeterminism):** All same-binary repeats produced MAE < 0.003 (most are exactly 0). No additional within-binary nondeterminism source surfaced. **R5 is not triggered**; the captures-not-matching scenario the design pass flagged as a surface-back-without-fix did not occur.

**R3 mitigation (PT-seed change ripples):** Confirmed via candidate-vs-candidate same-binary repeats (toggle 1: bit-identical, toggle 0: <8e-4 residual on GISponza late frames driven by TLAS flap-back, not by RNG drift).

**Capture archive:** `Build/captures/TASK-213-CL-B/{run1,run2}/{toggle0,toggle1}/{unittest,gitestbox,gisponza}/gpu_output_{0025..0029}.png` (60 PNGs total) plus per-scene `engine.log` files.

**Reviewer:** test-expert peer (per `peer-review-required.md`'s reviewer-selection rule — same role family, fresh dispatch). Reviewer should independently `Read` at minimum: GISponza toggle 0 frames 25 and 29 from both runs (verify the black-frame timing and the late-frame MAE), and GISponza toggle 1 frames 25 and 29 (verify visual correctness). Then verify the per-pair MAE numbers above with their own `magick compare` runs. Surface-back; no commit this CL.

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
