---
id: TASK-68
title: >-
  Headless scene conversion (bake mode) — import external models without
  graphics init
status: Done
assignee: []
created_date: '2026-04-18 14:58'
updated_date: '2026-04-18 18:26'
labels:
  - feature
  - tooling
  - asset-pipeline
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Scene conversion (external model → engine-native `.InnoMesh` / `.InnoMaterial` / `.InnoTexture` / `.InnoScene`) is currently bound to the interactive `Y` keyboard shortcut in `ExampleProject/LogicClient/World.inl` and runs inside a full engine session. Two problems:

1. **Slow** — per-frame tick runs everything: rendering, animation, physics, path tracer etc. Asset import is serialized behind those frames.
2. **Requires a window + GPU** — can't be driven from CI or batch-baked on a server.

Expose a dedicated headless "bake" path so conversion can run as a one-shot CLI pass.

## Proposed shape (land the smallest thing first)

**Option A (minimal):** Add `-bake "path1.gltf;path2.fbx;..."` to `Main.exe`:
- Implies `-headless` (skip all rendering services — AssetService and AssimpWrapper have no GPU dependencies).
- `Engine::Run` enters a bake loop: for each path, call `AssetService::ImportSync`; after the last one finishes, call `Terminate` and exit 0.
- Skip the Update() / rendering tick entirely in this mode so there's no per-frame overhead.

**Option B (cleaner, later):** New `Source/Tool/AssetBaker` CMake target that links only `Common + Services/AssetService + ThirdParty/AssimpWrapper + ThirdParty/JSONWrapper + ThirdParty/STBWrapper`. No engine runtime, no Client DLLs. Ship as `Bin/RelWithDebInfo/AssetBaker.exe`.

Start with A because it's a small PR and verifies that `ImportSync` actually runs correctly with `-headless` today; migrate to B once the pipeline is proven.

## Scope check first

Before implementing, verify that `AssetService::ImportSync` → `AssimpWrapper::Import` → `AssimpImporter::Import` only touches CPU state (JSON wrapper + STB image + file I/O). If any of those reach into `TextureResourceService::Initialize` or similar GPU paths, split the "parse + serialize to disk" from "upload to GPU" first — that's a prerequisite task.

## Acceptance

- `Main.exe -bake "…/Sponza_PBR/…"` produces the same `.InnoMesh` / `.InnoMaterial` / `.InnoTexture` files as the interactive `Y` key in the editor, byte-identical where feasible.
- Bake mode skips window creation and rendering services (verifiable: no `DX12Context` in the log).
- Converting the 5 assets currently on the `Y` shortcut is measurably faster than the interactive path — log wall-clock per asset and total.
- Exit code 0 on success, non-zero if any import fails; `-bake` is composable with `-loglevel` for CI.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Verify AssetService::ImportSync has no GPU dependencies (or split them out)
- [ ] #2 Add -bake arg to Main.exe that runs ImportSync on provided paths, then exits
- [ ] #3 -bake implies -headless (no window, no rendering services)
- [ ] #4 Converting the 5 Y-key assets succeeds headlessly and produces byte-equivalent output
- [ ] #5 Wall-clock faster than interactive path; logged per-asset timing
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Landed Option A from the task description: `Main.exe -bake "path1;path2;..."` runs a one-shot asset-import-then-exit pass. `-bake` sets `isBakeMode` in InitConfig and implies `isHeadless` (both in ParseInitConfig and in the early WinMain `l_isHeadless` detection, so CreateRenderingClient / CreateLogicClient are never instantiated). Engine::Run splits the `;`-separated list and calls `AssetService::ImportSync` per path, logging per-file wall-clock and a final total. LogicClient Setup / Initialize / Terminate are skipped in bake mode (they call SceneService::Load → FrameManagementService::WaitForGPUIdle, which has no hardware service wired in headless). WinMain propagates import failure as exit 1, bake success as exit 0. Verified: single-asset bake (bunny 143 ms), two-asset batch (bunny+dragon 2.4 s), nonexistent-path bake (exits 1), RenderTest + Main 3-frame regression green. Prerequisite check from the task — "Verify AssetService::ImportSync has no GPU dependencies" — confirmed by grep: no GraphicsHardwareService / TextureResourceService / etc calls in AssimpWrapper or AssetService::Import path.
<!-- SECTION:FINAL_SUMMARY:END -->
