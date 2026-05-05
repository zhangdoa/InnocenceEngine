---
id: TASK-215
title: >-
  ImGuiWrapper "Load scene" button uses sync Load (engine-invariants.md
  violation)
status: Done
assignee:
  - low-level-expert
created_date: '2026-05-03 08:24'
updated_date: '2026-05-05 07:55'
labels:
  - bug
  - engine-invariant
dependencies: []
references:
  - 'Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:293'
  - .claude/state/engine-invariants.md
  - Source/Engine/Services/SceneService.h
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Violation

`Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:293` (the ImGui "Load scene" button callback) calls:

```cpp
g_Engine->Get<SceneService>()->Load(scene_filePath, false);
```

This is a UI-thread caller passing `AsyncLoad=false` — exactly the failure shape the engine invariant warns against.

## Invariant (verbatim from `.claude/state/engine-invariants.md:7-11`)

> `SceneService::Load()` calls that originate outside the render thread (HIDService button callbacks, any other main-thread context) must pass `AsyncLoad=true`. `LoadSync()` from a non-render-thread caller races the render thread's `IGraphicsService::Update()` and corrupts the DX12 resource state tracker, manifesting as a `tracker=UAV / D3D12-actual=SRV` barrier mismatch on the Final Blend Pass Result texture (e.g. after pressing R to reload a scene).
>
> `LoadAsync()` sets `m_prepareForLoadingScene=true`; `SceneService::Update()` on the render thread picks it up the next frame and calls `LoadSync()` from the safe context. The auto-test path (loading from `World::Update()` inside the render thread callback) may remain synchronous because it already runs on the render thread.

## Precedent

Commit `f4e0252a` fixed the same shape in `Source/Engine/Common/World.inl` (the active failure point — latent race surfaced by TASK-213 CL 5's wider LoadSync window). The test-expert peer review of that fix audited remaining `Load()` call sites and flagged ImGuiWrapper.cpp:293 as the same shape, but only triggers on user UI-button reload — not on the auto-test path, so latent until exercised.

## Owner

`low-level-expert` per fallback rule in dispatch brief: `editor-tooling-expert` scope is `Source/Editor-Next` (Vue / Electron / TS), not the C++ runtime ImGui wrapper.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 ImGuiWrapper.cpp:293 changed to pass AsyncLoad=true (matching the f4e0252a precedent in World.inl).
- [x] #2 Audit all other Load() call sites in Source/Engine/ThirdParty/ImGuiWrapper/ — any UI-thread caller passing AsyncLoad=false is corrected; any intentional sync caller (render-thread context) is documented in-place.
- [x] #3 Optional: extend .claude/state/engine-invariants.md to enumerate all known SceneService::Load() call sites and their thread context, so the next audit is a one-liner grep against the enumerated list.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Review (code-impl, 2026-05-05)

**Verdict: PASS** with 2 ADVISORY findings. Terminal goal met — ImGui Load button no longer calls sync `Load()`; mirrors `f4e0252a` precedent shape exactly.

**Verified independently:**

- AC #1 — literal flip landed. `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:296` reads `Load(scene_filePath, true)`; signature matches `f4e0252a` (`World.inl:253` → `Load(l_initialScene, true)`). Pre-fix line 293 is now occupied by the rationale comment block.
- AC #2 — ImGui subtree audited. `grep "->Load("` under `Source/Engine/ThirdParty/ImGuiWrapper/` returns exactly one hit, the fixed line. No other `Load(` callers in that subtree.
- AC #3 — call-site enumeration cross-checked against `grep "SceneService.*->Load\(" Source/`. Five hits in tree; table enumerates all five.

**Findings (both ADVISORY, both addressed in same CL):**

1. **Line-number drift on day one.** Table cited `ImGuiWrapper.cpp:293`; with the inserted comment block, the actual `Load()` call is at line 296. → Fixed in same CL: table updated to `:296`.
2. **Invariant prose vs new table row contradicted each other.** `engine-invariants.md:7` originally read "calls that originate **outside the render thread** … must pass `AsyncLoad=true`" and `:9` exempted the render-thread callback path. The new table row correctly identified the ImGui button as render-thread (`GUIService::Update` inside FMS, after pass commands recorded) yet still requiring async — which made the prose internally inconsistent with the table. → Fixed in same CL via reviewer's option (a): widened prose to cover both (a) non-render-thread callers and (b) render-thread callers fired mid-frame after pass commands have been recorded; updated the auto-test exemption note to specify "before any pass commands are recorded for that frame."

**Cross-references used:** `Source/Engine/Services/GUIService.cpp:31-39`, `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp:176`, `Source/ExampleProject/LogicClient/World.inl:253` (precedent), `git show f4e0252a` (precedent CL).

**Reviewed-By: code-impl**

## Surfaced (out of scope, dispatcher to triage)

- **Pre-existing flake at `Source/Engine/Services/HIDService.cpp:66` (`HIDService::Update`)** — AV at +0x110, fault address `0xFFFFFFFFFFFFFFFF`. Reproduces on clean `master` with the same auto-test repro (`Scripts/TestGPUPathTracer.ps1 -Frames 30`). NOT caused by TASK-215. Implementer's hypothesis: use-after-free or torn map iteration on `m_ButtonEvents` — possibly racing `HIDService::Setup` callback registration against the first `Update()` tick. Worth filing as a separate bug.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Final Summary — TASK-215

**Status:** Done. Reviewed: PASS with 2 ADVISORY findings, both addressed in same CL.

### What landed (2 files)

- `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:296` — flipped `AsyncLoad=false` to `true` on the "Load scene" ImGui button callback. 3-line comment cites the invariant (lines 293-295).
- `.claude/state/engine-invariants.md` — appended a 5-row "Known SceneService::Load() call sites" subsection enumerating every call site in the engine; widened the prose at lines 7-9 (per peer-review ADVISORY #2) to cover both (a) non-render-thread callers AND (b) render-thread callers fired mid-frame after pass commands have been recorded.

### AC coverage

| AC | Status | Evidence |
|---|---|---|
| #1 ImGuiWrapper.cpp:293 → AsyncLoad=true | ✓ | Line is now :296 (post-comment-block); reads `Load(scene_filePath, true)`, mirrors f4e0252a |
| #2 Audit Load() sites in ImGuiWrapper subtree | ✓ | grep confirmed only one site in subtree, the fixed line |
| #3 (optional) enumerate all SceneService::Load() sites | ✓ | 5-row table in engine-invariants.md, grep-validated |

### DoD coverage

| DoD | Status | Notes |
|---|---|---|
| #1 Code compiles | ✓ | Pre-existing Bin/RelWithDebInfo/Main.exe; subagent build cleanly produced .exe with this CL |
| #2 Pre-existing integration tests re-run | ✓ | Main.exe -total_frames 30 (main session): GISponza loaded at frame 5, 30 frames rendered, engine terminated cleanly. No D3D12 errors, no UAV/SRV barrier mismatch on Final Blend Pass Result texture. One spurious HIDService flake on first attempt (filed separately, see Implementation Notes §Surfaced) |
| #3 New integration test | N/A | TestGPUPathTracer.ps1 / Main.exe -total_frames cover the surrounding scene-load path |
| #4 Mocks not sole validation | ✓ | Real Main.exe runs only |
| #5 User-observable verification | partial | Code-path equivalence to f4e0252a (precedent CL) + 30-frame engine run validate the fix surface; ImGui button click itself is not exercised by any auto-test (inherent gap) |
| #6 What was NOT verified | ✓ | listed below |

### What was NOT verified (DoD #6)

- **The ImGui button callback was not exercised at runtime.** No automated harness clicks the "Load scene" ImGui button. Code-path equivalence to `f4e0252a` and `EditorService.cpp:498` (mirror caller, already `, true`) carries the bulk of confidence. Fix is a 1-literal flip of a function parameter whose semantics are documented in `SceneService.h:20`.
- **Pre-existing HIDService::Update flake at HIDService.cpp:66 (+0x110)** reproduces on clean master — surfaced for separate filing, not addressed here.

**Reviewed-By: code-impl**
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
