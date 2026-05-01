---
id: TASK-211
title: >-
  Editor Screenshot action — finish the feature: deterministic path, extension,
  timestamp, user-visible location feedback
status: In Progress
assignee:
  - editor-tooling-expert
created_date: '2026-05-01 14:28'
updated_date: '2026-05-01 14:29'
labels:
  - editor
  - bug
  - ipc
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
  - Source/Engine/Services/AssetService.cpp
  - Source/Engine/Services/EditorService.cpp
  - Source/Engine/Services/EditorService.h
  - Source/Engine/ThirdParty/STBWrapper/STBWrapper.cpp
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
  - Source/Editor-Next/src/composables/useIpc.js
  - Source/Editor-Next/src/store/devToggleStore.js
  - Source/Editor-Next/tests/render-toggles.spec.js
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-05-01**: editor's `Screenshot` action button is a half-wired prototype — clicking it does *not* deliver a usable screenshot.

## Diagnosis (already done by main-session)

Current behaviour:

1. Editor IPC routes `data-test="action-btn-Screenshot"` → `devToggleStore.triggerAction("Screenshot")` → `TRIGGER_DEV_ACTION` IPC → engine's `DevToggleRegistry` action `"Screenshot"` registered at `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:147` — sets `m_saveScreenCapture = true`.
2. Next frame, `ExampleRenderingClient.cpp:954-959` reads back `FinalBlendPass::GetResult()` and calls `AssetService::Save("ScreenCapture", textureDesc, data)`.
3. `AssetService::Save` (`Source/Engine/Services/AssetService.cpp:604-607`) → `STBWrapper::Save` (`Source/Engine/ThirdParty/STBWrapper/STBWrapper.cpp:54-104`) → `stbi_write_png(fileName, ...)`.
4. The literal string `"ScreenCapture"` is passed verbatim as the filename. Result: the file lands at `<CWD>/ScreenCapture` (typically `C:\GitRepo\InnocenceEngine\Bin\ScreenCapture`) with **no extension**, **no timestamp**, **no counter**. Every press overwrites the previous output. The editor toast says "Screenshot triggered" but never tells the user where the file went. Existing test: `Source/Editor-Next/tests/render-toggles.spec.js:60-61` only verifies the toast text.

## Goal

Hitting the Screenshot action produces a file the user can find without searching, that does not get overwritten by the next press, that opens correctly in any image viewer, and whose path is surfaced to the user in the editor at capture time.

## Path target — `Bin/Captures/Screenshots/`

**Chosen** over `<UserProfile>/Pictures/InnocenceEngine/` for one reason: the engine already writes its other capture artefacts into the working tree (`-dump_frames` writes `gpu_output_NNNN.png` next to the binary; auto-capture writes `gpu_output.png`). Co-locating user-triggered screenshots with engine-triggered captures keeps them all reachable from a single `Captures/` root, gitignored alongside `Build/`. `<UserProfile>/Pictures` would split capture artefacts across two filesystem roots and complicate `Build/captures/` archival per visual-validation discipline. Directory created on first use.

## Filename shape

`screenshot_YYYY-MM-DD_HH-MM-SS-mmm.<ext>` — sub-second precision (3-digit milliseconds) means two presses within the same second still produce two distinct files without a separate counter. Extension matches `PixelDataType` branch in `STBWrapper::Save`: `.png` for LDR (`UByte`), `.hdr` for `Float16`/`Float32`. Current trigger path saturates LDR at `WriteCaptureToFile` semantics (`UByte` after sqrt-tonemap), so the live screenshot path will produce `.png`. The extension-selection logic still honours the `Float32` branch for any future caller.

## IPC contract change

`TRIGGER_DEV_ACTION` is currently fire-and-forget — the action's `m_Trigger` callback returns `void`, the IPC reply contains only the action name, and the actual save happens *next frame* anyway (so a synchronous return is structurally impossible).

The right shape is the existing **EVENT envelope** (`BuildEvent` + per-client `send` already used by `BroadcastSceneUpdated` at `Source/Engine/Services/EditorService.cpp:221-231`). After the rendering client writes the file, broadcast a `SCREENSHOT_SAVED` event with `{ path: "<absolute>", ok: true }` (or `{ ok: false, error: "<reason>" }`). Editor's `RenderTogglesPanel.vue` subscribes via `on('SCREENSHOT_SAVED', …)` and shows the toast on event arrival, replacing the current immediate optimistic toast.

## Acceptance Criteria
<!-- AC:BEGIN -->
| AC | Bar |
|---|---|
| AC-1 | File written to `Bin/Captures/Screenshots/`. Directory created if missing. |
| AC-2 | Filename `screenshot_YYYY-MM-DD_HH-MM-SS-mmm.<ext>`; two presses within the same second produce two distinct files. |
| AC-3 | Extension matches the chosen pixel format (`.png` / `.hdr`). |
| AC-4 | Editor toast on success names the **full absolute path** of the saved file. On failure (write error / readback failure), the toast says so explicitly with the reason. Both paths covered in the spec. |
| AC-5 | `Source/Editor-Next/tests/render-toggles.spec.js` existing test still passes. Spec extended (or sibling spec added) to assert (a) success toast contains a path string, (b) the file actually exists on disk after the action. |
| AC-6 | Engine log line on save is informative — exact path written, success/failure. |

## Cross-agent dependency map

- **Engine-side (rendering client trigger site)** — `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:147` (action registration) and `:954-959` (per-frame save). Filename construction, directory ensure, save-then-broadcast logic. Owner: `rendering-researcher` (the file is owned by the rendering client team per CLAUDE.md framing — render-pass `*Pass.cpp` files are owned by `rendering-researcher`; this is the consuming non-pass file but main-session brief routes the change there for proximity).
- **Engine-side (event broadcast)** — `Source/Engine/Services/EditorService.cpp` — expose a `BroadcastDevActionResult(name, payload)` (or domain-specific `BroadcastScreenshotSaved`) helper, mirroring `BroadcastSceneUpdated`. Owner: `software-architect` (service architecture / IPC contract).
- **Editor-side** — `Source/Editor-Next/src/components/RenderTogglesPanel.vue` (toast wiring on `SCREENSHOT_SAVED` event, drop the optimistic toast), `Source/Editor-Next/tests/render-toggles.spec.js` (extend or sibling). Owner: `editor-tooling-expert` (this dispatch).

## Non-goals

- Not redesigning the `m_saveScreenCapture` flag mechanism — the next-frame trigger pattern stays.
- Not pushing timestamp/path logic into `STBWrapper` — STB stays a thin format wrapper.
- Not extending the `TRIGGER_DEV_ACTION` reply shape to carry async results — events are the right venue for "save completed N frames later" deliveries.

## References

- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:147` — action registration
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:954-959` — per-frame save
- `Source/Engine/Services/AssetService.cpp:604-607` — Save dispatch
- `Source/Engine/ThirdParty/STBWrapper/STBWrapper.cpp:54-104` — STB write
- `Source/Engine/Services/EditorService.cpp:93-100` — `BuildEvent`
- `Source/Engine/Services/EditorService.cpp:221-231` — `BroadcastSceneUpdated` (precedent)
- `Source/Engine/Services/EditorService.cpp:330-371` — `LIST_DEV_TOGGLES` / `SET_DEV_TOGGLE` / `TRIGGER_DEV_ACTION` handlers
- `Source/Editor-Next/src/components/RenderTogglesPanel.vue:24-27` — current optimistic toast
- `Source/Editor-Next/src/composables/useIpc.js:83-105` — event envelope dispatch (`emit('engine-message')` + `emit(msg.type)`)
- `Source/Editor-Next/tests/render-toggles.spec.js:60-63` — existing test
<!-- SECTION:DESCRIPTION:END -->

- [ ] #1 File written to Bin/Captures/Screenshots/. Directory created if missing.
- [ ] #2 Filename screenshot_YYYY-MM-DD_HH-MM-SS-mmm.<ext>; two presses within the same second produce two distinct files.
- [ ] #3 Extension matches the chosen pixel format (.png for LDR / .hdr for float).
- [ ] #4 Editor toast on success names the full absolute path of the saved file. On failure, the toast says so explicitly with the reason. Both paths covered in the spec.
- [ ] #5 Existing render-toggles.spec.js test still passes. Spec extended (or sibling added) to assert (a) success toast contains a path string, (b) the file actually exists on disk after the action.
- [ ] #6 Engine log line on save is informative — exact path written, success/failure.
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Implementation plan (editor-tooling-expert, 2026-05-01)

### Sequencing

The editor-side change is **dependent** on the engine emitting a `SCREENSHOT_SAVED` event on the wire. Without the event source, replacing the optimistic toast would leave the user with **no feedback at all** between click and the actual save (~1 frame typical, but indeterminate on slow scenes). So the editor change cannot meaningfully land before the engine change — implementing it first would commit dead code (an event subscriber with no producer).

Right order:

1. **Engine-side first** (cross-agent — dispatcher must route):
   - `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:147` — register a richer action callback (still `void` return — async result delivered via the event channel below) that captures `EditorService*` so the per-frame save can broadcast.
   - `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:954-959` — replace the literal `"ScreenCapture"` filename with: (a) ensure `Bin/Captures/Screenshots/` exists via `std::filesystem::create_directories`; (b) timestamp via `std::chrono::system_clock::now()` formatted to `screenshot_YYYY-MM-DD_HH-MM-SS-mmm`; (c) extension from `textureDesc.PixelDataType` (`UByte` → `.png`, `Float16`/`Float32` → `.hdr`); (d) on `Save` return value, broadcast `SCREENSHOT_SAVED` with `{ ok: bool, path?: <absolute>, error?: <reason> }`; (e) `Log(Success, ...)` / `Log(Warning, ...)` carrying the same path string for AC-6.
   - `Source/Engine/Services/EditorService.h` + `.cpp` — expose a `BroadcastEvent(const char* type, json payload)` (or `BroadcastScreenshotSaved(...)`) public method, mirroring `BroadcastSceneUpdated`. The rendering client calls it from `Update()` after `AssetService::Save` returns.

2. **Editor-side second** (this dispatch):
   - `Source/Editor-Next/src/components/RenderTogglesPanel.vue` — drop the optimistic `message.success(\`${action.name} triggered\`)` from `onActionTrigger`; subscribe via `useIpc().on('SCREENSHOT_SAVED', ({ ok, path, error }) => ...)`. On `ok: true` → `message.success(\`Screenshot saved: ${path}\`)`; on `ok: false` → `message.error(\`Screenshot failed: ${error}\`)`. Other dev actions remain on the optimistic-toast path until they grow their own result events.
   - `Source/Editor-Next/tests/render-toggles.spec.js` — extend the existing test (or add a sibling spec): click `action-btn-Screenshot`, wait for the success toast, assert (a) toast text matches `/Screenshot saved: .+\.(png|hdr)$/`, (b) the path component of the toast resolves to a real file on disk via Node `fs.existsSync` from the spec process. Use `await window.waitForSelector('.n-message:has-text("Screenshot saved:")', { timeout: 10000 })` then read the textContent to extract the path.

### Path target rationale (recap)

`Bin/Captures/Screenshots/` over `<UserProfile>/Pictures/InnocenceEngine/` — co-locates with engine-side capture artefacts already written near the binary (`gpu_output.png`, `gpu_output_NNNN.png`). Single root for `Build/captures/` archival per visual-validation discipline. No new gitignore needed (`Build/` style — `Bin/` is itself a build-output area).

### IPC contract widening

`SCREENSHOT_SAVED` is the first per-action result event. Naming per-event rather than a generic `DEV_ACTION_RESULT` because the payload schema differs per action (a future "Reload Shaders" action would carry `{ recompiled: N, errors: [...] }`, not `{ path }`). One-event-per-action keeps payloads strongly typed at the consumer; the precedent is `SCENE_UPDATED` rather than a generic `EVENT`.

### Surfaced back to dispatcher (2026-05-01)

The editor-tooling-expert has paused implementation pending the engine-side CL. The C++ change list is:

1. **Rendering client** (`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`, lines 147 and 954-959) — owner: `rendering-researcher`. The brief states main-session will dispatch this; the file is currently quiescent (rendering-researcher is suspended on TASK-77.1 awaiting user direction, no live edits in the trigger region per the dispatch brief).
2. **EditorService** (`Source/Engine/Services/EditorService.h` + `.cpp`) — owner: `software-architect`. Expose a public `BroadcastEvent` (or `BroadcastScreenshotSaved`) helper following the `BroadcastSceneUpdated` precedent. The rendering client links against EditorService through `g_Engine->Get<EditorService>()`.

Once both land, this task picks up with the editor-side toast wiring + spec extension as the closure CL.
<!-- SECTION:NOTES:END -->
