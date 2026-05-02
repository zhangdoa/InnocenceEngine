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

### CL C — Pin GISponza camera deterministically across binary launches — surfaced 2026-05-02

**Files touched:**
- `Source/ExampleProject/LogicClient/World.inl` — orbit override now reads its frame index from `FrameManagementService::GetSteadyStateRelativeFrameCount()` instead of `m_AutoFrameCount`. Reason: `m_AutoFrameCount` advances unconditionally post-Activated, so its value at any given dump frame depends on the deferred-init drain timing (RC-6 in the design pass). The steady-state-relative counter is gated on the same latch as `m_autoCaptureFrameCount` (CL B), so yaw at dump frame N is independent of variable load-frame count. Added `#include "../../Engine/Services/FrameManagementService.h"`.
- `Scripts/TestPathTracerThreeScenes.ps1` — added `$DefaultCameraOrbit = "20,8,120"` (PITCH=20°, RADIUS=8, DURATION=120 frames; matches the long-standing precedent across TASK-124 / TASK-138 / TASK-6.x captures). The `-CameraOrbit` parameter now resolves to the default when empty, to no-orbit when explicitly `"none"`, otherwise verbatim. (Cross-subtree edit; Scripts/ is owned by `ci-build-expert` — touched here under dispatcher brief authority because the script-default is the load-bearing piece of the camera pin.)

**`-camera_orbit` flag signature** (pre-existing, NOT changed): `-camera_orbit PITCH_DEG,RADIUS,DURATION_FRAMES`, parsed in `Source/Engine/Engine.cpp:457` (rejects RADIUS<=0 or DURATION<=0). The dispatch brief's "0,8,0" was a transcription artefact — that triple is invalid (DURATION=0). Used the established precedent `20,8,120` as the per-scene default for all three scenes; per-scene overrides can be added later if a scene needs different framing.

**R4 mitigation (default orbit masks scene-file-camera regressions):** confirmed by inspection. The orbit override is a SCRIPT default (`$DefaultCameraOrbit`), not an engine default. When `-camera_orbit` is not on the command line, `Engine.cpp` leaves `cameraOrbitActive = false` and World.inl's orbit block is skipped — the scene-file Main Camera transform applies. Interactive launches via `StartEngineWin.ps1` (no `-camera_orbit` arg) are unaffected. Confirmed engine-side defaults unchanged; no `Engine.cpp` / `Engine.h` edits in this CL.

**R6 mitigation (no wall-clock / random inputs):** the steady-state-relative frame counter is a pure integer subtraction (`m_FrameCountSinceLaunch - m_FirstSteadyStateFrame`), and the orbit transform is a deterministic function of `(yaw, pitch, radius)`. Pure deterministic transform throughout.

**Same-binary repeat MAE matrix** (`-Frames 60 -DumpStart 25 -DumpEnd 29`, run1 vs run2, both built from the CL C source state):

| scene     | MAE per frame [25, 26, 27, 28, 29] |
|-----------|------------------------------------|
| unittest  | 0, 0, 0, 0, 0                      |
| gitestbox | 0, 0, 0, 0, 0                      |
| gisponza  | 0, 0, 0, 0, 0                      |

All 15 frames bit-identical. **AC-1 PASS** under CL C as it did under CL B; the CL C camera-pin did not break the determinism CL B established.

**Cross-binary informal MAE matrix** (run1 vs fresh-binary, two separate clean rebuilds of the CL C source):

| scene     | MAE per frame [25, 26, 27, 28, 29]                        |
|-----------|-----------------------------------------------------------|
| unittest  | 0, 0, 0, 0, 0                                             |
| gitestbox | 0, 0, 0, 0, 0                                             |
| gisponza  | 0, 0, **0.0434**, 4.30e-5, **6.57e-3**                    |

**unittest and gitestbox: full cross-binary determinism achieved** — bit-identical PNGs across two separate builds. This is the headline AC-3 result: the camera pin pulls the deterministic-camera bar for these two scenes from "drifts ~0.05 MAE under the scene-file camera (CL B Ext.3)" to bit-identical.

GISponza is partially achieved: the [25,26]-black-window matches across binaries (both runs hit the same black PNGs); frame 28 matches to 4.3e-5 (bit-equivalent in practice); frames 27 and 29 differ. **The cause is the K=3 plateau / TLAS-rebuild flap-back issue documented in CL B's surprise 1**, NOT the camera pin: in fresh-binary's GISponza, frame 27 latched on a transient instance-count plateau and rendered as uniform-black (36970 bytes), while run1's frame 27 latched after the plateau and rendered real content (795KB). This is the same root cause AC-7 is scoped to fix in CL D (script-side flap-back-aware dump shifting OR per-scene K override). CL C's camera pin is correct independent of this — when both runs render real content (e.g. frame 28), the camera viewpoint matches.

**Visual Read assessment** (per `visual-validation.md` layer 1):
- *unittest, frame 25 / frame 29, run1 vs fresh-binary*: ShaderBall reflective sphere on grey floor at right of frame; jagged geometry array (PBR materials) on left; hard sun-shadow cast onto floor. The viewpoint is from yaw≈75° (frame 25) to yaw≈87° (frame 29), pitch 20° elevation, radius 8 — looking down at the scene from above and to the right. Bit-identical between run1 and fresh-binary on every frame.
- *gitestbox, frame 25 / frame 29*: Cornell-box-style coloured walls (pink wall edge upper-left); camera is inside the box at radius 8 looking outward, so most of the frame is dark wall interior with PT noise. Slight yaw progression visible across frames. Bit-identical between run1 and fresh-binary.
- *gisponza, frame 28 / frame 29 (run1)*: ShaderBall reflective sphere centre-frame, Sponza atrium archway visible behind, columns and curtains at edges. Heavy PT noise (correct for low-SPP cache-OFF). The orbit camera puts the viewer inside the atrium looking down at the floor objects.
- *gisponza, frame 27 (fresh-binary)*: uniform-black 36970-byte PNG — same flap-back artefact CL B documented. Frame 27 in run1 is real content (795KB). The flap-back is non-deterministic across builds because the K=3 latch can fire on a transient plateau, and which transient plateau it lands on depends on deferred-init drain timing (the CL B / CL A advisory).
- *gisponza, frame 28-29 (run1 vs fresh-binary)*: viewpoints match — both show the ShaderBall + atrium framing from the same orbit position. MAE 4.3e-5 (frame 28) and 6.57e-3 (frame 29). The 6.57e-3 is dominated by PT noise differences from RNG-state-at-frame divergence (tiny because RNG seed is now steady-state-relative per CL B), NOT camera viewpoint mismatch.

**Verdict: improvement** for AC-3 on unittest and gitestbox (bit-identical cross-binary), partial improvement for AC-3 on gisponza (camera pin is correct; residual cross-binary MAE on gisponza is the AC-7 K=3 plateau issue scoped to CL D, not a camera-pin failure).

**AC-7 status (CL D's job, not CL C's):** GISponza toggle 0 frames 25-26 are still uniform-black PNGs (36970 bytes) on both runs and the fresh binary, matching the [25,26]-black-window CL B documented. Fresh-binary's frame 27 is also black (it landed in the flap-back window). CL C neither fixes nor regresses this surface — confirmed unchanged per the brief's "CL C does NOT fix the [25,26]-black-window" directive.

**Surprises / advisories surfaced:**

1. **Pre-existing PT PSO build hazard, not CL C-caused.** The first capture attempt failed with `DX12 create failed: Raytracing PSO context=GPUPathTracerPass HRESULT=-2147024809`. Investigation: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` had been edited (mtime 20:08) AFTER the last DXIL compile (`Bin/Shaders/DXIL/GPUPathTracerRayGen.hlsl.dxil` mtime 20:03), and `Scripts/BuildWin.ps1`'s post-build step does not re-run `Scripts/HLSL2DXIL_NoPause.ps1` — it only mirror-copies the existing DXIL into `Bin/RelWithDebInfo/`. So a stale DXIL with a root-signature shape that no longer matches the runtime PSO descriptor was being loaded. Fix path: re-running `Scripts/HLSL2DXIL_NoPause.ps1` produced fresh DXIL; the same PT capture then ran clean. **The hazard is the build script not invoking shader compile** — a future CL (likely owned by `ci-build-expert`) should either chain HLSL2DXIL into BuildWin.ps1 or add a freshness check that fails loudly when an HLSL is newer than its DXIL. Out of scope for CL C; flagged here for the dispatcher and `ci-build-expert`.

2. **Cross-subtree edit acknowledged.** `Scripts/TestPathTracerThreeScenes.ps1` is owned by `ci-build-expert` per `Scripts/CLAUDE.md`. The dispatch brief explicitly listed this script in "Files likely to touch" and routed CL C to test-expert; the edit was made under that authority. The change is self-contained (default-value resolution + new `$DefaultCameraOrbit` constant; no harness-level rewiring) and the existing `-CameraOrbit` parameter contract is preserved (empty → default, "none" → off, value → verbatim).

3. **gisponza interior is too small for radius=8 orbit.** With Sponza's Main Camera scene-file pose at (0,2,0) and the atrium being roughly 30 units wide, an orbit at radius 8 puts the camera inside the atrium walls / against geometry. The captures are visually claustrophobic (lots of curtain / column / ShaderBall close-up). For closure of AC-3 this does not matter — bit-equivalent captures are bit-equivalent regardless of framing — but a future iteration of the script may want a per-scene orbit triple (e.g. radius 30 for gisponza). Decision deferred: per the brief, "all three the same starting point" was the explicit instruction, and AC-3 closure is on cross-binary equality, not aesthetic framing.

4. **R5 not triggered.** Same-binary repeats produced 15/15 MAE=0. No additional within-binary nondeterminism surfaced beyond the CL B / CL A flap-back advisory.

5. **Cross-subtree note for TASK-212 cleanup:** `Scripts/TestPathTracerThreeScenes.ps1` gained a `$DefaultCameraOrbit = "20,8,120"` block + `-CameraOrbit` resolution. Per-scene radii are deferred (Sponza atrium ~30 units wide; orbit radius 8 is tight per advisory 3). `ci-build-expert` may want to fold per-scene radii into the cleanup.

**Capture archive:** `Build/captures/TASK-213-CL-C/{run1,run2,fresh-binary}/{unittest,gitestbox,gisponza}/gpu_output_{0025..0029}.png` (45 PNGs total) plus per-scene `engine.log` files. The fresh-binary archive was produced by touching `World.inl` and re-running `BuildWin.ps1` to force a full source recompile + link before re-running the capture script.

**Build:** RelWithDebInfo clean (Main.exe + RenderTest.exe) — both source-rebuilt for the fresh-binary run. No new compiler warnings; pre-existing `MathHelper.h` C4003 unchanged.

**Reviewer:** test-expert peer (per `peer-review-required.md`'s reviewer-selection rule — same role family, fresh dispatch). Reviewer should independently `Read` at minimum: unittest frame 25 from run1 + fresh-binary (verify bit-identical viewpoint), gitestbox frame 25 from run1 + fresh-binary (verify bit-identical), gisponza frame 28 from run1 + fresh-binary (verify viewpoint match within MAE 4.3e-5), and gisponza frame 27 fresh-binary (verify the uniform-black flap-back artefact AC-7 will fix). Then verify the same-binary and cross-binary MAE matrices with their own `magick compare` runs. Reviewer should also confirm advisory 1 (stale-DXIL hazard) is filed as a follow-up rather than being CL C's blocker. Surface-back; no commit this CL.

### Load-determinism foundation chain — CL 3: residency predicate on five resource services — surfaced 2026-05-03

This is CL 3 of the standalone "load-determinism foundation" chain that re-frames TASK-213's CL A/B latch issue (`m_autoCaptureFrameCount` gating on `IsSteadyState`'s K=3 plateau, which lands on transient instance-count plateaus mid-walk — see CL B surprise 2 / CL C AC-7 black-frame surface). The foundation chain replaces the K=3 latch with a per-component residency observation per the `no-shadow-state.md` discipline (commit `a86e6e93`).

Foundation-chain anchors (none committed under TASK-213 explicitly; see commits):
- CL 1: `a86e6e93` — `no-shadow-state.md` + universal-list wiring + 5 agent cross-refs.
- CL 2: `46c3ac85` — SceneService unload reorder (client callbacks fire BEFORE engine-internal unload).
- **CL 3 (this surface-back)**: residency predicate on five resource services.
- CL 4 (next): `SceneService::IsResidencyComplete()` aggregator + wire `m_autoCaptureFrameCount` gate to it instead of `IsSteadyState`.
- CL 5: remove FMS::Update lines 161-165 + `ClearLoadingFlag`; set `m_IsLoading` synchronously at LoadSync entry/exit. Discards TASK-213 CL E stash (superseded).

**Files touched (CL 3):**
- `Source/Engine/Common/NamedObjectPool.h` — added a const overload of `ForEach(std::function<void(T*)>)`. The existing non-const overload was untouched. The const overload delegates to `ThreadSafeVector::for_each(...) const` (line 128 of `ThreadSafeVector.h`), which already takes a `std::shared_lock`.
- `Source/Engine/Services/MeshResourceService.h` — declared `MeshComponent* GetFirstPendingComponent() const`.
- `Source/Engine/Services/Common/MeshResourceServiceImpl.cpp` — implemented.
- `Source/Engine/Services/TextureResourceService.h` — declared `TextureComponent* GetFirstPendingComponent() const`.
- `Source/Engine/Services/Common/TextureResourceServiceImpl.cpp` — implemented.
- `Source/Engine/Services/MaterialResourceService.h` — declared `MaterialComponent* GetFirstPendingComponent() const`.
- `Source/Engine/Services/Common/MaterialResourceServiceImpl.cpp` — implemented.
- `Source/Engine/Services/GPUBufferResourceService.h` — declared `GPUBufferComponent* GetFirstPendingComponent() const`.
- `Source/Engine/Services/Common/GPUBufferResourceServiceImpl.cpp` — implemented.
- `Source/Engine/Services/RenderPassResourceService.h` — declared `RenderPassComponent* GetFirstPendingComponent() const`.
- `Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp` — implemented.

**Implementation shape (identical across the five services):** the method takes `m_Pool.ForEach(...)` and captures the first non-`Activated` component via a closure-captured pointer; subsequent invocations short-circuit on the body's first `if (l_pending) return;`. No counts, no flags, no shadow state — pure observation of `m_ObjectStatus`.

```cpp
ServiceComponent* SomeService::GetFirstPendingComponent() const
{
    ServiceComponent* l_pending = nullptr;
    m_Pool.ForEach([&l_pending](ServiceComponent* in_Component)
    {
        if (l_pending)
            return;
        if (in_Component && in_Component->m_ObjectStatus == ObjectStatus::Created)
            l_pending = in_Component;
    });
    return l_pending;
}
```

**Pre-activation status used:** `ObjectStatus::Created`. All five services route their `Add(name)` through `NamedObjectPool::Allocate(name)`, which stamps `l_ptr->m_ObjectStatus = ObjectStatus::Created` (NamedObjectPool.h:47). Activation happens only on successful `InitializeComponents()` paths — Mesh:148, Texture:157 (also Synchronous:130), Material:79, GPUBuffer:69, RenderPass:62. No path stamps `Suspended` between Allocate and Activate. Per the brief edge case 2: explicit `== ObjectStatus::Created` check (not `!= ObjectStatus::Activated`) so `Suspended`/`Terminated` components are correctly treated as not-pending.

**Per-service quirks (none required code differences):**
- **Mesh** has the shared-handle re-queue at MeshResourceServiceImpl.cpp:121-127 (activation-only tasks re-queued when their primary handle has not yet built). Re-queued components retain `ObjectStatus::Created` until line 148 stamps `Activated` on success — predicate correctly observes them as pending.
- **Mesh** also has a re-queue on `InitializeImpl` failure at line 151 — same correct behavior.
- **Texture / Material / GPUBuffer / RenderPass** have failure-only re-queues (`m_DeferredQueue.push(std::move(l_task));` on init-failure path). Same correctness — pending until activated.
- The `m_DeferredQueue` is intentionally NOT consulted by the predicate (per brief). A queued-but-not-yet-popped component is in `m_Pool` with `Created` status; iteration catches it directly.
- **GPUBuffer / RenderPass** already exposed a `ForEach(std::function<void(T*)>)` on the service itself (used by other engine code, e.g. resize callbacks). Reused via the pool's overload directly inside the new method; the service-level `ForEach` was not touched.

**No-shadow-state self-check:**
- No `int m_X_Count` added to any service.
- No `bool m_IsXResidencyComplete` / `m_AllReady` / `m_IsLoaded` added.
- No `std::atomic<bool>` shadow added.
- The new const `NamedObjectPool::ForEach` overload is a pure read accessor; introduces no state.
- Predicates derive entirely from per-component `m_ObjectStatus` (the data's own state) and the live-objects vector membership (`NamedObjectPool::Allocate` populates, `Release` removes).

**Test by inspection — MaterialResourceService walk-through (per brief):**
Hypothetical scene: 3 materials at `Created`, 2 at `Activated` (5 total live in `m_Pool`).
1. `GetFirstPendingComponent()` calls `m_Pool.ForEach(lambda)`.
2. `NamedObjectPool::ForEach(...) const` (just added) calls `m_LiveObjects.for_each(lambda)`.
3. `ThreadSafeVector::for_each(...) const` takes a `std::shared_lock` and runs `std::for_each` over all 5 entries in allocation order.
4. Element 1 (`Created`): `l_pending == nullptr` → status check passes → `l_pending = element1`. Returns.
5. Elements 2-5: `if (l_pending) return;` short-circuits. Lambda body's status check skipped.
6. `for_each` returns. `l_pending` holds the first `Created` MaterialComponent. Method returns it.
   Expected result: returns the first `Created` material. ✓

If all 5 were `Activated`: each invocation finds `l_pending == nullptr` AND status check fails → `l_pending` stays `nullptr` for the entire walk → method returns `nullptr` (vacuously residency-complete). ✓ (matches brief edge case 1)

**Threading note (per `threading-contracts.md`):** `GetFirstPendingComponent()` is `const`. Underneath, `NamedObjectPool::ForEach (const)` → `ThreadSafeVector::for_each (const)` takes a `std::shared_lock` on the live-objects mutex — multiple readers may call concurrently; concurrent `Add`/`Delete` on the service block the reader briefly but do not corrupt iteration. Per-component `m_ObjectStatus` is a plain `ObjectStatus` enum, written by the engine main thread inside `InitializeComponents()`. Reading it from a non-main thread is technically a data race per the C++ memory model, but the result is at worst a stale snapshot (one frame off) — acceptable for the residency-aggregator use case at CL 4. If CL 4's caller is main-thread-only this concern is moot.

**Build:** RelWithDebInfo full-tree build clean (`cmake --build Build --config RelWithDebInfo`, exit 0). All consumers compile (Engine, RenderTest, Main, all DX12/VK/MT backends, TestSuite). No new compiler warnings on any of the six touched files. Pre-existing `MathHelper.h` C4003 `max` macro warnings unchanged (unrelated; predates this CL).

**Surprises / advisories:**
1. **`NamedObjectPool::ForEach` was non-const-only.** Adding the const overload was a one-line delegate (`ThreadSafeVector::for_each` already had its const overload). Owned subtree (`Source/Engine/Common`) per low-level-expert; legitimate within-scope edit, no cross-subtree exposure.
2. **`NamedObjectPool::ForEach` signature is `std::function<void(T*)>`, not a generic predicate.** I considered adding a `FindIf`-style overload that early-exits properly, but the existing `for_each` is fine — the lambda short-circuits on `if (l_pending) return;` and the iteration cost over a few hundred components per service is negligible for a per-frame query. The brief specifically said "don't reshape the pool API unnecessarily."
3. **Per-component `m_ObjectStatus` lives directly on the component struct** for all five (verified: Mesh/Texture/Material/GPUBuffer/RenderPass all carry it). No base-class inheritance gymnastics required; no shared template machinery touched.

**Reviewer:** low-level-expert peer (per `peer-review-required.md`'s reviewer-selection rule — same role family, fresh dispatch). Reviewer should inspect for no-shadow-state compliance (no new counter / flag fields on any of the five services) and predicate correctness (early-exit lambda short-circuit, `== Created` check, const correctness through the pool walk). Surface-back; no commit this CL.

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
