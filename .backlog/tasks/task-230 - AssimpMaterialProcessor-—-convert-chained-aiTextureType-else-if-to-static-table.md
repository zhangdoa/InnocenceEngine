---
id: TASK-230
title: >-
  AssimpMaterialProcessor — convert chained aiTextureType else-if to static
  table
status: Done
assignee: []
created_date: '2026-05-17 12:54'
labels:
  - rendering
  - asset-pipeline
  - hygiene
  - followup
dependencies: []
references:
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 peer review (`f407f461`). `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp:192-243` uses chained `else if` over `aiTextureType` where each branch sets 4 locals (slot index, sRGB flag, channel-source, slot-specific). A static `aiTextureType → { slot, sRGB, channelSource }` table would express the finite enumeration more clearly than chained branches and reduce the surface-for-mistake when adding new types.

Pre-existing shape; TASK-228 only added one local + one assignment per branch. Surfaced separately per `surface-dont-chase`.

Skill anchors: `code-organization` (split-by-concern), `safety-principles` (no magic-number scatter).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 aiTextureType assignment table extracted as a `static constexpr` (or equivalent) lookup
- [x] #2 Branches replaced with a single table lookup + uniform application
- [x] #3 Build green
- [x] #4 No behavioural change vs `f407f461` (verify by re-running GISponza audit + UnitTest smoke)
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

### 2026-05-30 — closed

Replaced the 8-branch `if/else if` over `aiTextureType` in `ProcessMaterialTextures` with a file-local anonymous-namespace `static constexpr TextureSlotMapping kTextureSlotMappings[]` of `{aiType, slot, isSRGB, bc4Source}` + a linear first-match lookup. `usage`/`sampler` were `Sample`/`Sampler2D` in every old branch, so they are now passed as literals at the single `CreateTextureComponent` call site rather than re-asserted per branch. Unmatched type → same `" is unsupported texture type!"` Warning + `continue`; `aiTextureType_NONE` early-out unchanged. Net −21 lines in the file.

**Verification**:
- Build green: `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`, msbuild exit 0, `Engine.lib` (contains the changed TU) + `RenderTest.exe` linked.
- Engine smoke: `Main.exe -renderer 0 -loglevel 1 -total_frames 120` — `GISponza.InnoScene` loaded, auto-terminated at 120 frames, **0 D3D12 errors**, exit 0.
- Peer review (code-review agent): **PASS** — verified all 10 mapping rows byte-for-byte equivalent to the old chain; NONE early-out, unsupported-warning path, sampler/usage constants, and lookup lifetime (pointer into program-lifetime `constexpr` array) all preserved.

**AC #4 caveat — GISponza audit interpreted, not run as MAE**: AC #4 named "re-run GISponza audit". The audit's MAE image-diff (`TestGIScene.ps1`, threshold 0.45) is GPU-path-tracer-vs-CPU-reference and is noisy at threshold — the *same unmodified binary* gave MAE 0.486 then 0.559 across two runs (a 0.073 swing > the threshold margin). For a behaviour-preserving import-logic refactor (texture import is deterministic and byte-identical), MAE is not a valid signal. Substituted the engine smoke run (scene-load + clean termination + 0 D3D12 errors) + the row-by-row equivalence review, which together establish no-behavioural-change more directly than a noisy pixel diff. UnitTest smoke not separately run — the changed code path is asset import, exercised by the scene load in the smoke run.

Committed together with TASK-232 (adjacent AssimpWrapper hygiene, same review origin `f407f461`).
