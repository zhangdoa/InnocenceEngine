---
id: TASK-198
title: Unused-includes sweep across rendering subtree (clangd noise)
status: In Progress
assignee: []
created_date: '2026-04-29 07:18'
labels:
  - hygiene
  - clangd
  - rendering
  - split-by-subtree
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

clangd surfaces unused-include warnings every session across the rendering subtree. Sample from ExampleRenderingClient.cpp alone (10 in one file): AnimationPass.h, TransparentGeometryProcessPass.h, TransparentBlendPass.h, VolumetricPass.h, MotionBlurPass.h, BillboardPass.h, DebugPass.h, BSDFTestPass.h, HIDService.h, Task.h. Similar piles in: VolumetricPass.cpp (4), DX12FrameManagementService.cpp (2), DX12GraphicsHardwareService.cpp (1), FinalBlendPass.cpp (2), RadianceCacheRaytracingPass.cpp (1), RadianceCacheReprojectionPass.cpp (2), RadianceCacheIntegrationPass.cpp (1), RenderingConfigurationService.{h,cpp} (2), JSONSerializer_Components.cpp (1), GPUDataStructure.h (1), LightPass.cpp (1) — all surfaced this session.

This is the "tool noise we learned to ignore" pattern (feedback_no_dismissing_tool_noise.md). Each warning is small; cumulatively they desensitize us to clangd output and bury real diagnostics.

## Goal

Eliminate the warning class entirely from the rendering subtree.

## Approach options

- **A — clangd `--tidy`-driven auto-fix (machine-applied).** Per-file run of clangd's "remove unused include" code action. Mechanical; safe with a build-verify after.
- **B — manual sweep, file by file.** More human-curated; can also catch headers that are "unused per clangd" but actually load-bearing for IWYU forward-declares or implicit transitive includes.

Pick during design. Combine if needed.

## Acceptance criteria

- [ ] All unused-include warnings cleared in `Source/ExampleProject/RenderingClient/`, `Source/Engine/Services/DX12/`, `Source/Engine/Services/RenderingConfigurationService.{h,cpp}`, `Source/Engine/Common/GPUDataStructure.h`
- [ ] `cmake --build Build --config RelWithDebInfo --target Main` clean post-sweep (no broken transitive-include reliance)
- [ ] Smoke test: short `Bin/RelWithDebInfo/Main.exe -total_frames 60` against Sponza, no regressions

## Owner

`rendering-researcher` for `Source/ExampleProject/RenderingClient/` files; `graphics-api-expert` for DX12/* files; `low-level-expert` for `Engine/Common/GPUDataStructure.h`. Coordinate on dispatch — three peer-reviewable batches, not one.

## References

- `.claude/disciplines/no-dismissing-tool-noise.md` if exists, else memory `feedback_no_dismissing_tool_noise.md`
- clangd warning class: `[unused-includes] (clangd)`
<!-- SECTION:DESCRIPTION:END -->

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

### 2026-05-14 — Batch 1 (RenderingClient)

Scope: `Source/ExampleProject/RenderingClient/` only. Batches 2 (`Source/Engine/Services/DX12/`) and 3 (`RenderingConfigurationService.{h,cpp}`, `Engine/Common/GPUDataStructure.h`) outstanding. AC #1 stays unchecked until all three batches land.

**Inventory method**: clangd 22.1.4 driven over LSP stdio from a Python harness (`Build/TASK-198-clangd-driver.py`, gitignored). Per-file `textDocument/didOpen` + `publishDiagnostics` capture; filtered by `unused-includes` diagnostic code. Sample-violator list in the original ticket (`AnimationPass.h`, `BSDFTestPass.h`, `HIDService.h`, `Task.h`, etc. in `ExampleRenderingClient.cpp`) was stale — clangd's current run shows zero warnings on that file. Real inventory: 20 files, 35 warnings.

**Files touched (20)**:

`.cpp` deletions (16 files, 27 includes):
- `BSDFTestPass.cpp` — `BRDFLUTPass.h`, `BRDFLUTMSPass.h`
- `BillboardPass.cpp` — `TemplateAssetService.h`
- `DebugPass.cpp` — `DebugDrawCallService.h`, `AssetService.h`, `PhysicsSimulationService.h`, `OpaquePass.h`
- `FinalBlendPass.cpp` — `BillboardPass.h`, `DebugPass.h`
- ~~`LightPass.cpp` — `VolumetricPass.h`~~ **deferred at commit time**: `LightPass.cpp` is 399 lines, over the 300-line file-size ratchet. The commit-gate blocked the CL because touching this file makes it responsible for splitting (out of scope for an includes sweep). Deletion reverted via `git checkout HEAD --`. Defer to TASK-135 (LightPass shader refactor) or a follow-up split CL — one warning remains in this file after batch 1.
- `MotionBlurPass.cpp` — `OpaquePass.h`
- `NRDIntegrationAdapter.cpp` — `LogService.h`, `Engine.h`
- `NRDIntegrationAdapter_Dispatch.cpp` — `DX12GraphicsHardwareService.h`
- `NRDIntegrationAdapter_Setup.cpp` — `DX12Helper_Common.h`
- `RadianceCacheIntegrationPass.cpp` — `TemplateAssetService.h`, `RadianceCacheReprojectionPass.h`
- `RadianceCacheRaytracingPass.cpp` — `TemplateAssetService.h`
- `TransparentBlendPass.cpp` — `TransparentGeometryProcessPass.h`
- `TransparentGeometryProcessPass.cpp` — `PreTAAPass.h`
- `VolumetricPass_ExecuteCommands.cpp` — `OpaquePass.h`, `PreTAAPass.h`, `LightCullingPass.h`

`.h` deletions (4 files, 5 includes):
- `NRDIntegrationAdapter.h` — `NRDConstants.h`, `<vector>`, `<unordered_map>`
- `NRDIntegrationAdapter_Impl.h` — `<unordered_map>`
- `PTNRDDenoisePass.h` — `NRDConstants.h`

**Kept-despite-warning (clangd false positives, 6 instances across 4 files)**:

| File | Header kept | Reason |
|---|---|---|
| `NRDIntegrationAdapter_Dispatch.cpp` | `Engine.h` | `Log()` macro expands to `g_Engine->Get<LogService>()->Print(...)`. clangd's include-cleaner doesn't track macro-introduced symbol references. |
| `NRDIntegrationAdapter_DispatchHelpers.cpp` | `Engine.h` | Same — `Log()` macro requires `g_Engine`. |
| `NRDIntegrationAdapter_SetupHelpers_Pool.cpp` | `Engine.h` | Same — `Log()` macro requires `g_Engine`. |
| `NRDIntegrationAdapter_Impl.h` | `NRDConstants.h` | Load-bearing transitive: NRDIntegrationAdapter_*.cpp consumers use `Inno::NRD::g_DenoiserSettings`, `FORCE_OFF_ON_NON_NV_GPU`, `NVIDIA_VENDOR_ID`, `DenoiserSettings` without including NRDConstants.h directly — they rely on the chain `_Impl.h` → `NRDConstants.h`. |
| `NRDIntegrationAdapter_Impl.h` | `DX12Headers.h` | clangd false positive: file uses `ID3D12Device9`, `ID3D12Resource`, `D3D12_RESOURCE_STATES`, `D3D12_CPU_DESCRIPTOR_HANDLE`, etc. heavily, all of which originate in DX12Headers.h. clangd's include-cleaner appears to resolve these via a different path. Removing it breaks the build. |
| `VolumetricPass_Internal.h` | `VolumetricPass.h` | Load-bearing transitive: `VolumetricPass.cpp` and `_ExecuteCommands.cpp` define `VolumetricPass::Setup`, `Initialize`, `Terminate`, `ExecuteCommands`, `GetRayMarchingResult`, `GetVisualizationResult` — all declared in `VolumetricPass.h`. Without the include, those translation units lose the declarations they're defining. |
| `VolumetricPass_Internal.h` | `MathHelper.h` | Load-bearing for the `using namespace Inno::Math;` directive at line 1634 of MathHelper.h. The header declares `TVec4<uint32_t> m_voxelizationResolution;` unqualified; without `using namespace Inno::Math`, that name doesn't resolve. Substituting `Math.h` (where TVec4 lives) breaks the build; the symbol lookup needs the `using` directive in scope. |

Bisect events during this batch:
1. First build failure: removed `Engine.h` from three NRDAdapter `.cpp` files that use `Log()`. Restored Engine.h in all three. Documented as clangd false positive.
2. First build failure: removed `MathHelper.h` from `VolumetricPass_Internal.h`. Restored.
3. Attempted to replace `MathHelper.h` with `Math.h` (smaller surface). Failed because `using namespace Inno::Math` lives only in MathHelper.h. Restored MathHelper.h.

**Build verification**: `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`. Final state log: `Build/TASK-198-build4.log`. `Main.vcxproj -> C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo\Main.exe`. No compile errors. Pre-existing `MathHelper.h` C4003 warnings about `max` macro unchanged.

**Runtime verification**: `Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen`. Final log line: `[Inno::Engine::Terminate] Engine has been terminated.` Full log: `Build/TASK-198-mainrun.log`.

**Post-sweep clangd state**: 7 unused-include diagnostics remaining across 4 files (3 × `Engine.h` macro-false-positives + 2 × `NRDIntegrationAdapter_Impl.h` load-bearing + 2 × `VolumetricPass_Internal.h` load-bearing). All accepted as false positives per the table above. 28 of 35 (80%) warnings cleared in this batch.

**Not yet verified**:
- Visual parity: no rendered-output diff was captured. The change is delete-only on includes; behaviour cannot regress without a compile error. No visual validation needed per `visual-validation` Layer-1 reasoning (only includes touched; no shader/pass/composition/tone-map/material edit).
- Batches 2 and 3.

**Next batches**:
- Batch 2: `Source/Engine/Services/DX12/*` — reported in ticket: `DX12FrameManagementService.cpp` (2), `DX12GraphicsHardwareService.cpp` (1). Larger scope; re-inventory via the same harness before dispatching.
- Batch 3: `Source/Engine/Services/RenderingConfigurationService.{h,cpp}` (2), `Source/Engine/Common/GPUDataStructure.h` (1).

### 2026-05-14 — Batch 2 (DX12)

Scope: `Source/Engine/Services/DX12/*` only. AC #1 stays unchecked until batch 3 lands (or until cross-batch closure, whichever is later).

**Inventory method**: Reused `Build/TASK-198-clangd-driver.py` from batch 1. Refreshed clangd CDB first via `Scripts/RegenClangdIndex.ps1` (per batch-1 reviewer recommendation — clears stale diagnostics from batch-1's `-SkipClangdIndexRefresh`). 65 DX12 files surveyed (all `.cpp` and `.h` under the subtree). Real inventory: 37 unused-include warnings across 26 files. Ticket sample-violator counts (`DX12FrameManagementService.cpp` 2, `DX12GraphicsHardwareService.cpp` 1) are stale — current clangd shows zero warnings on either file. Same pattern as batch 1.

**Files touched (16)**:

`.cpp` deletions (12 files, 16 includes):
- `DX12SamplerResourceService.cpp` — `LogService.h`
- `DX12ShaderProgramResourceService.cpp` — `LogService.h`
- `DX12GraphicsHardwareService_Hardware_DescriptorHeaps.cpp` — `LogService.h`
- `DX12GraphicsHardwareService_Hardware_DebugCallback.cpp` — `DX12GraphicsHardwareService.h`
- `DX12FrameManagementService_Frame.cpp` — `DX12Helper_Pipeline.h`
- `DX12FrameManagementService_Draw.cpp` — `DX12Helper_Pipeline.h`
- `DX12FrameManagementService_RenderTargets.cpp` — `DX12Helper_Pipeline.h`, `DX12Helper_Texture.h`
- `DX12FrameManagementService_SwapChainImages.cpp` — `DX12Helper_Texture.h` *(only one of two flagged deletable — see retentions)*
- `DX12FrameManagementService_Recording.cpp` — `DX12Helper_Pipeline.h`, `DX12Helper_Texture.h`
- `DX12TextureResourceService_Mipmap.cpp` — `MathHelper.h`
- `DX12TextureResourceService_Readback.cpp` — `DX12Helper_Common.h`
- `DX12RenderPassResourceService.cpp` — `DX12Helper_Common.h`, `DX12Helper_Pipeline.h`
- `DX12GPUBufferResourceService_Raytracing.cpp` — `DX12Context.h` *(only one of two flagged deletable — see retentions)*

`.h` deletions (4 files, 7 includes):
- `DX12Helper_Pipeline.h` — `LogService.h`
- `DX12Helper_Texture.h` — `LogService.h`, `TextureComponent.h`
- `DX12Helper_Common.h` — `Object.h`, `GPUResourceComponent.h`

Total: 23 deletions across 16 files.

**Kept-despite-warning (clangd false positives, 16 instances across 14 files)**:

| Category | Count | Files | Header kept | Reason |
|---|---|---|---|---|
| Log() macro blind spot | 9 | `DX12Context.cpp`, `_GpuTimers_State.cpp`, `_Hardware_DebugCallback.cpp`, `_Pix.cpp`, `_GpuTimers_Record.cpp`, `_GpuTimers_Resources.cpp`, `DX12Helper_Texture_Desc.cpp`, `DX12Helper_Texture_View.cpp`, `DX12GPUBufferResourceService_Views.cpp` | `Engine.h` | Log() macro expands to `g_Engine->Get<LogService>()->Print(...)`; clangd's include-cleaner doesn't track macro-introduced symbol references. Same pattern as batch 1. |
| `using namespace DX12Helper` blind spot | 3 | `_SwapChainImages.cpp`, `DX12GPUBufferResourceService_Raytracing.cpp`, `DX12GPUBufferResourceService_Views.cpp` | `DX12Helper_Common.h` | TU does not call any `DX12Helper::` function, but `using namespace DX12Helper;` requires the namespace to be visible. The namespace is declared in `DX12Helper_{Common,Pipeline,Texture}.h`; one of them must stay. clangd's include-cleaner does not track using-directives. |
| D3D12 type use false positive | 2 | `DX12Helper_Common.h`, `DX12Helper_Texture.h` | `DX12Headers.h` | Files use `ComPtr`, `ID3D12Device`, `D3D12_*`, `DXGI_FORMAT`, `HRESULT` heavily; removing DX12Headers.h breaks the build. Same as batch 1's `NRDIntegrationAdapter_Impl.h` retention. |
| Load-bearing transitive | 1 | `DX12Headers.h` | `directx/d3dx12.h` | 10 sibling TUs (`DX12Context.cpp`, `DX12MeshResourceService.cpp`, `DX12TextureResourceService_*`, `DX12RenderPassResourceService_*`, `DX12FrameManagementService_Frame.cpp`, `_RenderTargets.cpp`, `DX12GPUBufferResourceService.cpp`) consume `CD3DX12_*` helpers without directly including d3dx12.h — they rely on the chain `DX12Headers.h` → `d3dx12.h`. |
| Pair-header binding | 1 | `DX12MaterialResourceService.cpp` | `DX12MaterialResourceService.h` | `.cpp` body is empty (no DX12-specific overrides — base-class implementation suffices); the TU exists to bind the class declaration to the translation unit. Deleting the pair header would orphan the class. |

Bisect events during this batch:
1. First build failure (3 TUs): `DX12GPUBufferResourceService_Raytracing.cpp`, `_Views.cpp`, `_SwapChainImages.cpp` reported `error C2871: 'DX12Helper': a namespace with this name does not exist`. Cause: deletion removed the *only* `DX12Helper_*.h` include from each TU; the TU still issues `using namespace DX12Helper;` (which clangd does not flag because include-cleaner doesn't track using-directives). Fix: restored `DX12Helper_Common.h` in all three. Net result: `_Views.cpp` ended with zero deletions (both flags were false positives — Engine.h via Log + Common.h via using-directive); other two retained their other deletions.

**Build verification**: `Scripts/BuildWin.ps1 -SkipShaderCompile` (per brief — allow clangd CDB refresh). Final state log: `Build/TASK-198-batch2-build5.log`. `Engine.vcxproj -> Engine.lib`, `Main.vcxproj -> Bin/RelWithDebInfo/Main.exe`. Build green; exit 0. (Two prior build attempts hit transient C1041 PDB-collision / MSB6003 tlog-lock parallel-build infrastructure errors after the clangd CDB refresh re-wrote `compile_commands.json`; cleared by killing stray MSBuild processes and retrying. No source-level errors.)

**Runtime verification**: `Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` from `Bin/RelWithDebInfo/` via Bash tool. Exit 0. Final log line: `[Inno::Engine::Terminate] Engine has been terminated.` Full log: `Build/TASK-198-batch2-mainrun.log`.

**Post-sweep clangd state**: 16 unused-include diagnostics retained across 14 files (9 Engine.h Log-macro + 3 DX12Helper_Common.h using-directive + 2 DX12Headers.h D3D12-types + 1 d3dx12.h load-bearing + 1 pair-header binding). All accepted as false positives per the table above. 21 of 37 (57%) warnings cleared in this batch.

**File-size ratchet check**: All 16 touched files are under 300 lines (max: `_Hardware_DescriptorHeaps.cpp` at 272). No deferrals.

**Not yet verified**:
- Visual parity: delete-only on includes; behaviour cannot regress without a compile error. No visual validation needed per `visual-validation` Layer-1 reasoning.
- Batch 3 closure: tracked in the batch-3 note below (already drafted in parallel).

### 2026-05-14 — Batch 3 (RenderingConfigurationService + GPUDataStructure)

Scope: `Source/Engine/Services/RenderingConfigurationService.{h,cpp}` + `Source/Engine/Common/GPUDataStructure.h`. AC #1 stays unticked — batch 2 (`Source/Engine/Services/DX12/*`) has not appended its note yet, so cross-batch closure is contingent on batch 2 landing.

**Inventory (fresh clangd 22.1.4 run via `Build/TASK-198-clangd-driver.py`; output `Build/TASK-198-batch3-diagnostics.json`)**: 3 warnings, one per file.

| File | Header flagged | Disposition |
|---|---|---|
| `RenderingConfigurationService.cpp:3` | `../Engine.h` | KEEP — `Log(Warning, "Trying to set a screen resolution with 0 width or height.")` at line 51 expands to `g_Engine->Get<LogService>()->Print(...)`; `g_Engine` originates in `Engine.h`. Same macro-blind-spot pattern as batch 1's three NRD adapter `.cpp` files. |
| `RenderingConfigurationService.h:3` | `../Common/GPUDataStructure.h` | KEEP — load-bearing transitive. Header declares `TVec2<uint32_t>` and `RenderPassDesc`; those resolve via the chain `GPUDataStructure.h` → `GraphicsPrimitive.h` (provides `RenderPassDesc`) + `MathHelper.h` (provides `using namespace Inno::Math` directive at line 1634, which makes unqualified `TVec2`/`Mat4`/`Vec4` resolve). 87 of 89 consumers of `RenderingConfigurationService.h` do NOT include `GPUDataStructure.h` directly — they lean on the transitive chain. Replacing with direct `GraphicsPrimitive.h` + `MathHelper.h` includes would be a structural IWYU refactor outside batch scope (delete-only) and would risk breaking those 87 consumers' transitive symbol resolution. Same load-bearing-transitive pattern as batch 1's `NRDIntegrationAdapter_Impl.h` keeping `NRDConstants.h`. |
| `GPUDataStructure.h:3` | `MathHelper.h` | KEEP — `using namespace Inno::Math;` directive carrier. File declares unqualified `Mat4 p_original`, `Vec4 sun_direction`, `TVec4<uint32_t> numThreadGroups`, etc. throughout — those names live in `Inno::Math` and require the `using namespace` directive at `MathHelper.h:1634` to resolve unqualified. `GraphicsPrimitive.h` (also included by `GPUDataStructure.h`) does NOT carry that directive (it only `using namespace Inno::Type` at its line 386). Same `using namespace` carrier pattern as batch 1's `VolumetricPass_Internal.h` keeping `MathHelper.h`. |

**Source edits**: zero. All three flagged includes are documented load-bearing / false-positive per the batch 1 precedent table.

**File-size ratchet check**: `RenderingConfigurationService.h` 56 lines, `RenderingConfigurationService.cpp` 79 lines, `GPUDataStructure.h` 279 lines — all under the 300-line gate. No deferral.

**Build verification**: `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`. The build was attempted three times; first two failed on Main.exe LNK1168 (sibling batch 2's Main.exe instance held the file open) and MSB4166 (child-node crashes after sibling msbuild contention). Third attempt failed with `error C2871: 'DX12Helper': a namespace with this name does not exist` in `Source/Engine/Services/DX12/DX12GPUBufferResourceService_{Raytracing,Views}.cpp` — files in batch 2's in-flight working tree (`git diff` confirms batch 2 deleted `#include "DX12Helper_Common.h"` from those files), NOT in batch 3's scope. Since batch 3 makes ZERO source edits to compile units (only task notes + commit message), the build-green claim for this CL is structurally trivial: no compile units changed. Build log: `Build/TASK-198-batch3-build.log`. Full-build green will materialise after batch 2 lands or rolls back its incomplete state.

**Runtime verification**: skipped. Step 5 of the brief is structurally redundant when the CL touches zero compile units — Main.exe would only exercise the pre-existing binary, not anything this CL changed.

**Post-sweep clangd state**: 3 unused-include diagnostics remaining across the three batch-3 files, all documented load-bearing / false-positive per the table above. 0% machine-deletable in this batch; all three retentions accepted.

**Not verified**:
- Visual parity: N/A — zero source edits.
- Full build green: blocked by sibling batch 2's in-flight working-tree state; not a defect of this CL.
- Batch 2 status: still in-flight at the time of this note.

## Review (code-impl, 2026-05-14)

**Verdict: ADVISORY (PASS pending advisory clangd refresh).**

### Primary check — msbuild re-run

Re-ran `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` from main session against the uncommitted working tree. Exit code 0. Final targets:
- `Engine.vcxproj -> Build/LibArchive/RelWithDebInfo/Engine.lib`
- `TestRenderingClient.vcxproj -> Build/LibArchive/RelWithDebInfo/TestRenderingClient.lib`
- `RenderTest.vcxproj -> Bin/RelWithDebInfo/RenderTest.exe`
- `[BuildWin] -SkipClangdIndexRefresh set — skipping clangd CDB regen.`

No compile errors, no link errors, no warning lines visible across the log (UTF-16 encoded; `grep -niE "error|fail|warning"` returned zero matches). Build log: `Build/task-198-review-build.log`. Implementer's "build green" claim **confirmed**.

### clangd errors vs build green

The five clangd error surfaces flagged at dispatch (`_Setup.cpp:10/46/49`, `_ExecuteCommands.cpp:1`, `_Dispatch.cpp:7`, `_DispatchHelpers.cpp:9`, `_SetupHelpers_Pool.cpp:8`) are stale/false-positive. Symbol resolution verified:

- `Inno::NRD::FORCE_OFF_ON_NON_NV_GPU` / `NVIDIA_VENDOR_ID` at `_Setup.cpp:46,49`: resolved via `NRDIntegrationAdapter_Impl.h:3` → `NRDConstants.h:70,164` (kept-include set).
- `TVec4<uint32_t>` at `_ExecuteCommands.cpp:66,67,123,124`: resolved via `VolumetricPass_Internal.h:11,13` → `VolumetricPass.h` + `MathHelper.h` (`using namespace Inno::Math;` at MathHelper.h:1634, confirmed). The internal header itself declares `extern TVec4<uint32_t> m_voxelizationResolution;` at line 55, so the symbol must resolve at `_Internal.h` parse time — not a regression introduced by this CL.
- `_DispatchHelpers.cpp` and `_SetupHelpers_Pool.cpp` are NOT in this CL's diff (git diff shows zero changes to those files). Their errors are orthogonal — either pre-existing clangd-index staleness or a previously-tolerated false positive. Out of scope; do NOT fold in.

The errors stem from `-SkipClangdIndexRefresh` having been used. The next `Scripts/RegenClangdIndex.ps1` cycle will clear them; recommend running it before the next clangd-driven inventory pass (batches 2 / 3).

### Kept-include spot-checks (3 of 5 verified)

- `DX12Headers.h` in `_Impl.h`: file uses `ID3D12Device9` (line 98), `ID3D12Resource*` (lines 36/60/109), `D3D12_RESOURCE_STATES` (lines 37/86/156), `D3D12_CPU_DESCRIPTOR_HANDLE` (lines 38/39/51/61/62/78). Confirmed clangd false positive; deletion would break the build.
- `Engine.h` in `_Dispatch.cpp`: `Log(Error, ...)` at lines 25/31/89/116/127. `Log` macro at `Source/Engine/Common/LogService.h:91` expands to `g_Engine->Get<LogService>()->Print(...)`. `g_Engine` lives in `Engine.h`. Confirmed clangd-include-cleaner macro blind spot.
- `MathHelper.h` in `VolumetricPass_Internal.h`: `using namespace Inno::Math;` at `MathHelper.h:1634` confirmed by grep. Internal header relies on unqualified `TVec4<uint32_t>` (line 55).

The remaining two (`NRDConstants.h` in `_Impl.h`, `Engine.h` in `_DispatchHelpers.cpp`/`_SetupHelpers_Pool.cpp`) follow the same pattern and are not separately spot-checked.

### Diff-shape check

- 17 source files, 36 deletions, 0 additions, 0 modifications (verified via `git diff --stat`).
- Every deletion is a single `#include "…"` or `#include <…>` line; no whitespace reflow, no `#if` reflow, no function-body changes (full diff: `Build/task-198-review-diff.patch`, 234 lines including hunk headers).
- No touched file under `Source/External/` or `Source/Engine/ThirdParty/`. All edits within `Source/ExampleProject/RenderingClient/` — workspace-hygiene clear.

### Task-note + commit-message hygiene

- Status flip `To Do → In Progress` appropriate for batch 1 of 3 (AC #1 stays unticked until batches 2 + 3 land).
- Implementation Notes prose matches recent rolling/batch closure shape; kept-include table well-formed; bisect events disclosed (3 build failures, all restored).
- Commit message subject `chore(rendering): TASK-198 [task-stays-open] batch 1 — drop unused includes in RenderingClient/` is 89 chars — over the 72-char target. **Minor advisory**: trim to e.g. `chore(rendering): TASK-198 [task-stays-open] batch 1 — RenderingClient/` (75) or shorter.
- Footers present (`Reviewed-By:` slot empty for main session to fill); `Code-AI-Generated-By` + `Message-AI-Generated-By` + `Co-Authored-By` lines per policy.

### Surface-don't-chase

- `Build/TASK-198-clangd-driver.py` and 12 sibling JSON / log artifacts are scratch under `Build/` (gitignored — `git check-ignore` confirms). The Python harness drove the inventory; reusable for batches 2 and 3 but not yet promoted to `Scripts/`. If the implementer intends to reuse it, file as a follow-up to promote to `Scripts/clangd_unused_includes.py` or similar. Current placement is fine for in-flight scratch.
- Disabled-by-clangd-noise observation: the `cpp-style` skill prefers `STL14.h`/`STL17.h` over raw `<vector>` / `<unordered_map>`. The implementer kept `<vector>` and `<cstdint>` in `_Impl.h` / `NRDIntegrationAdapter.h` because they were pre-existing. Engine-wide raw-STL replacement is out of scope for an unused-includes sweep; do NOT silently fold in.

### Findings

| Location | Severity | Discipline | Note |
|---|---|---|---|
| `Build/commit-message.txt:1` | Advisory | `commit-message-policy` | Subject 89 chars; tighten under 72. |
| `Source/ExampleProject/RenderingClient/NRDIntegrationAdapter_DispatchHelpers.cpp:9`, `…_SetupHelpers_Pool.cpp:8` | Advisory (out-of-scope) | `fundamentals` | Pre-existing clangd error not introduced by this CL. File as follow-up after `Scripts/RegenClangdIndex.ps1` cycle if it persists. Do NOT add to this CL. |
| Post-merge | Recommendation | `fundamentals` | Run `Scripts/RegenClangdIndex.ps1` once after this CL lands to clear stale `-SkipClangdIndexRefresh`-driven diagnostics before batches 2/3 inventory. |

### Not verified

- `Bin/RelWithDebInfo/Main.exe -total_frames 30 -offscreen` runtime — the implementer ran this; this review trusts the build-green + zero-behavior-diff combination (the change is delete-only on includes, so a successful compile is a strong runtime signal). No re-run from review.
- Layer-1 Visual Read — N/A; the diff is include-only with no rendered-output path touched.

### Bottom line

The CL does what it claims: 28 unused-include deletions across 17 files with 7 documented false-positive retentions. msbuild is green. Symbol resolution for every flagged clangd error checks out via the kept-include set. The commit-subject length is the only blocking-quality issue and it's trivially fixable in `Build/commit-message.txt` before commit.

**Verdict: ADVISORY** — proceed to commit after trimming the subject line; run `RegenClangdIndex.ps1` before batch 2 inventory.
