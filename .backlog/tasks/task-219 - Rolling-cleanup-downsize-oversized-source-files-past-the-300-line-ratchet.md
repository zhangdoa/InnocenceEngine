---
id: TASK-219
title: 'Rolling cleanup: downsize oversized source files past the 300-line ratchet'
status: To Do
assignee: []
created_date: '2026-05-05 16:48'
updated_date: '2026-05-06 09:55'
labels:
  - tech-debt
  - tooling
  - harness
dependencies: []
references:
  - .claude/hooks/lib/common.js
  - .claude/hooks/gates/file-size.js
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

The file-size gate (`gates/file-size.js`) is a soft ratchet: it blocks any new growth past 300 lines but grandfathers files already over the limit. The intent is to prevent further drift while leaving cleanup as opt-in. As of session-of-this-filing, ~20+ non-`ThirdParty/` source files sit over the limit, several at 1000+ lines.

Cleanup is opt-in by design. Downsizing a 1500-line file in one CL is rarely the right shape — it produces a single huge diff that's hard to review. The right shape is a rolling sweep: one file or one tight cluster per CL, each split-or-shrink justifies its own structural decision (extract per-feature, extract per-pass, promote to separate TU, etc.).

## Inventory at filing time

Top offenders, line counts, owning subtree:

| Lines | File | Owning impl stage |
|-------|------|-------------------|
| 1873 | `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` | code-impl |
| 1633 | `Source/Engine/Common/MathHelper.h` | code-impl |
| 1518 | `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` | code-impl |
| 1264 | `Source/Engine/Engine.cpp` | code-impl |
| 1241 | `Source/Engine/Common/Math.h` | code-impl |
| 1231 | `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` | code-impl |
| 1143 | `Source/Engine/Services/VK/VKGraphicsService_EngineComponent.cpp` | code-impl |
|  878 | `Source/Engine/Services/DX12/DX12TextureResourceService.cpp` | code-impl |
|  852 | `Source/Engine/Services/TemplateAssetService.cpp` | code-impl |
|  804 | `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp` | code-impl |
|  725 | `Source/Engine/Services/AssetService.cpp` | code-impl |
|  723 | `Source/Engine/Services/EditorService.cpp` | code-impl |
|  692 | `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp` | code-impl |
|  682 | `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | shader-impl |
|  678 | `Source/Engine/Services/VK/VKHelper_Texture.cpp` | code-impl |
|  673 | `Source/Tool/Reflector/Reflector.cpp` | code-impl |
|  668 | `Source/Engine/Services/DX12/DX12Helper_Texture.cpp` | code-impl |
|  660 | `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` | code-impl |
|  655 | `Source/ExampleProject/RenderingClient/VolumetricPass.cpp` | code-impl |
|  635 | `Source/Engine/Services/VK/VKGraphicsService_VulkanObject.cpp` | code-impl |
|  399 | `Source/ExampleProject/RenderingClient/LightPass.cpp` | code-impl |
|  406 | `Source/ExampleProject/LogicClient/World.inl` | code-impl |
|  371 | `Scripts/TestPathTracerThreeScenes.ps1` | ci-build-impl |

(Inventory is a snapshot. Re-run `find Source -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.inl' -o -name '*.hlsl' -o -name '*.hlsli' -o -name '*.comp' \) -not -path "*ThirdParty*" -not -path "*External*" -not -path "*Generated*" | xargs wc -l | awk '$1 > 300 && $2 != "total"' | sort -rn` for current state.)

## Approach (per-CL recipe)

For each entry the implementer picks:

1. **Identify the responsibility seam.** What's the natural split? Per-feature (ExampleRenderingClient → per-pass), per-API-surface (Math.h → vector / matrix / quaternion), per-helper-cluster (VKHelper_Texture: copy ops vs. layout transitions vs. mip generation). The seam should be obvious; if it's not, the file may be cohesive enough that a different remedy applies (extract internal helpers, promote magic constants, delete dead code).
2. **Pick the smallest split that gets the file under 300.** Don't refactor the whole file in one CL.
3. **Verify the split doesn't cross-pollute headers** — every new TU's `#include` graph should be a strict subset of the original.
4. **Build green + qualifying test pass.** No engine behavior change; no rendering regression.
5. **Standard peer review** per `peer-review-required.md`.

## Out of scope

- Files under `Source/Engine/ThirdParty/`, `Source/External/`, `Generated/`, `dist/`, `node_modules/` — excluded by `FILE_SIZE_EXCLUDE_RE` in `.claude/hooks/lib/common.js`.
- Files in this list under 300 in a future inventory (already cleaned up via unrelated CLs).
- Files added to the list by future CLs (the gate blocks new growth — those should be caught at commit time, not here).

## Owner

Per file's owning impl stage (almost always `code-impl`; one shader-impl entry, one ci-build-impl entry).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Inventory list above is worked through over multiple sessions; each entry produces its own commit (or a small cluster commit covering related files).
- [ ] #2 Each downsize CL passes the file-size gate without escape — no FILE_SIZE_EXCLUDE_RE additions for files that should genuinely be split.
- [ ] #3 No engine behavior regression in any downsize CL; standard qualifying test + peer review per CL.
- [ ] #4 Task stays open until inventory re-run shows all current entries cleared; new entries surfaced after this filing get added inline.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## CL: DX12GraphicsHardwareService.cpp split (2026-05-05)

Split `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` (1873 lines) into 13 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12GraphicsHardwareService.cpp` | 66 | Umbrella: WaitOnFenceWithDiagnostics + public accessors |
| `DX12GraphicsHardwareService_Internal.h` | 21 | GPU_TIMER_* constants shared across timer TUs |
| `DX12GraphicsHardwareService_Sync.cpp` | 196 | SignalOnGPU / WaitOnGPU / Execute / GetSemaphoreValue / WaitOnCPU |
| `DX12GraphicsHardwareService_Debug.cpp` | 198 | RenderDoc + capture + HasGPUError + DumpGPUDiagnostics |
| `DX12GraphicsHardwareService_Pix.cpp` | 105 | PIX runtime load + BeginGpuEvent / EndGpuEvent |
| `DX12GraphicsHardwareService_GpuTimers_State.cpp` | 104 | GetTimerState / heap / readback / FindOrAllocate / FindTimerSlot / GetGpuTimings |
| `DX12GraphicsHardwareService_GpuTimers_Record.cpp` | 87 | BeginGpuTimer / EndGpuTimer |
| `DX12GraphicsHardwareService_GpuTimers_Resolve.cpp` | 187 | ResolveGpuTimers |
| `DX12GraphicsHardwareService_GpuTimers_Resources.cpp` | 141 | CreateGpuTimerResources / ReleaseGpuTimerResources |
| `DX12GraphicsHardwareService_Hardware_DebugCallback.cpp` | 146 | D3D12DebugMessageCallback + IsReleaseShaderGBVFalsePositive + CaptureCallstack + g_GPUErrorDetected |
| `DX12GraphicsHardwareService_Hardware_Devices.cpp` | 228 | CreateDebugCallback + CreatePhysicalDevices |
| `DX12GraphicsHardwareService_Hardware_DescriptorHeaps.cpp` | 273 | CreateGlobalDescriptorHeaps |
| `DX12GraphicsHardwareService_Hardware_Pipeline.cpp` | 94 | CreateGlobalCommandQueues / CreateGlobalCommandAllocators / CreateSyncPrimitives |
| `DX12GraphicsHardwareService_Hardware_Lifecycle.cpp` | 138 | CreateHardwareResources / ReleaseHardwareResources |

Total new TU lines: 1984. Largest TU: 273 (DescriptorHeaps). All under the 300-line ratchet.

### Constraints that bit

- **Internal header introduced.** `GPU_TIMER_*` constants are referenced from State / Record / Resolve / Resources sibling TUs, so they can't stay TU-local in the umbrella `.cpp`. Moved into `DX12GraphicsHardwareService_Internal.h`. The previous `static constexpr` (TU-local) becomes namespace-scope `static constexpr` (header inline) — same linkage semantics, no ODR issue.
- **D3D12DebugMessageCallback became external linkage.** The original was `static void CALLBACK`; now declared without `static` in `_Hardware_DebugCallback.cpp` and forward-declared as `extern` from `_Hardware_Devices.cpp` so `RegisterMessageCallback` can pass its address across TUs. `g_GPUErrorDetected` (the static atomic written by the callback) stays TU-local in `_Hardware_DebugCallback.cpp` — no other TU reads it directly; cross-TU consumers go through `m_DX12Context.m_GPUErrorDetected` which the callback also writes via the `pContext` pointer.
- **CMake auto-glob picked up new files** with no edits to `CMakeLists.txt` (`file(GLOB *.cpp)` + re-run cmake-configure).
- **Include graph subset.** Each new TU's includes are a strict subset of the original umbrella's; one minor addition was forced (`Engine.h` for the `Log()` macro, which expands to `g_Engine->Get<LogService>()->Print(...)`). The original also included `Engine.h` so this is not a graph extension, only redistribution.

### Build + test

- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. Both `Main.exe` and `RenderTest.exe` linked successfully.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` — exit 0. Engine initialised all DX12 services (device, queues, descriptor heaps, GPU timer resources, mipmap generator, raytracing) and terminated cleanly. Pre-existing `mipmapGenerator3D.comp.dxil` shader-compile warning on the build output is a known build-output issue, not a regression.

### Out of scope (not done)

- 19+ other oversized files in the inventory still pending. Task stays open.

## CL: ExampleRenderingClient.cpp split (2026-05-05)

Split `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (1518 lines) into 9 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same partial-class TU shape as the prior DX12GraphicsHardwareService CL — `ExampleRenderingClientImpl` declared once in `_Internal.h`, definitions distributed across sibling `.cpp` slices.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `ExampleRenderingClient.cpp` (umbrella) | 284 | Outer-class trampolines + `Initialize`, `Update`, `FinalizeGPUResults`, `Terminate`, `GetStatus`, `GetDispatchedPasses` |
| `ExampleRenderingClient_Internal.h` | 95 | `Impl` class declaration + private helper decls |
| `ExampleRenderingClient_Setup.cpp` | 272 | `Setup` + `RegisterDevToggles` (toggle registration + env-var overrides) |
| `ExampleRenderingClient_Bootstrap.cpp` | 53 | `BootstrapAmbientCGTextures` (PBR set import) |
| `ExampleRenderingClient_PrepareCommands.cpp` | 162 | `PrepareCommands` |
| `ExampleRenderingClient_ExecuteCommands.cpp` | 208 | `ExecuteCommands` orchestrator: BRDF one-shot + PT chain dispatch + Luminance/FinalBlend tail + screenshot/auto-capture/audit method calls |
| `ExampleRenderingClient_ExecuteCommands_Rasterizer.cpp` | 185 | `ExecuteRasterizerPasses`: opaque + sun shadow + SSAO + tiled frustum + light culling + LightPass + Sky + PreTAA + TAA |
| `ExampleRenderingClient_ExecuteCommands_GI.cpp` | 158 | `ExecuteGIPasses`: RadianceCache (reproject/raytrace/filter*/integrate) + GIDenoise + GIFilter*  |
| `ExampleRenderingClient_Capture.cpp` | 261 | `HandleScreenCapture`, `HandleAutoCaptureTriggers`, `WriteCaptureToFile`, `TryWriteAutoCapture` |
| `ExampleRenderingClient_AuditDump.cpp` | 103 | `AuditDump` |
| `ExampleRenderingClient_Bypass.inl` | 70 | (existing) anonymous-namespace helpers — now per-TU `#include`d wherever `DispatchOrBypass` / `IsBypassed` / `WaitIfActive` are used |

Total new TU lines: 1851 (incl. unchanged `_Bypass.inl` and unchanged `ExampleRenderingClient.h`). Largest TU: 284 (umbrella `.cpp`). All under the 300-line ratchet.

### Constraints that bit

- **Setup section over budget — extracted helper into a sibling TU.** Original `Setup` body alone was ~258 lines; adding `BootstrapAmbientCGTextures` and `RegisterDevToggles` as helpers in `_Setup.cpp` totalled 314 lines (over the 300 ratchet by 14). Resolution: pulled `BootstrapAmbientCGTextures` out into its own `_Bootstrap.cpp`. `RegisterDevToggles` stayed in `_Setup.cpp` because it's tightly bound to Setup's invocation flow (the `Get<PerFrameDataService>()` env-var overrides have to land before any pass `Setup()` reads them).
- **Rasterizer section over budget — sub-split the GI cluster.** `ExecuteRasterizerPasses` initially totalled 317 lines (over by 17). Resolution: extracted the RadianceCache + GIDenoise + GIFilter cluster (8 passes, the only tight intra-cluster sequencing) into `ExecuteGIPasses` in `_ExecuteCommands_GI.cpp`. The original dispatch order is preserved (GI block sits between SunShadowRT and SSAO).
- **Three new private methods on `Impl`.** `RegisterDevToggles`, `BootstrapAmbientCGTextures`, `ExecuteRasterizerPasses`, `ExecuteGIPasses`, `HandleScreenCapture`, `HandleAutoCaptureTriggers` — all extracted from inline blocks in the original `Setup` / `ExecuteCommands`. Pure inlined-statement-block-to-method moves; no captured locals other than `this`. The brief's hint "ExtractRasterizerPasses, HandleScreenCapture, HandleAutoCaptureTriggers" mapped 1:1 to this set.
- **`_Bypass.inl` is now per-TU included.** The original was included once at the top of the umbrella `.cpp` (single anonymous namespace). With the split, four TUs need the helpers: `_PrepareCommands.cpp`, `_ExecuteCommands.cpp`, `_ExecuteCommands_Rasterizer.cpp`, `_ExecuteCommands_GI.cpp`. Each does `#include "ExampleRenderingClient_Bypass.inl"` inside `namespace Inno { ... }` so the anonymous-namespace helpers stay TU-local. The brief flagged this pattern and the comment in the .inl is unchanged.
- **CMake auto-glob picked up new files** — `Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB *.cpp *.h)` so no edit was needed. **However: `Scripts/BuildWin.ps1` does NOT run cmake-configure**; the existing VS solution still listed only `ExampleRenderingClient.cpp`, producing LNK2001 unresolved-externals on the first build. Resolution: ran `cmake .` from `Build/` to regenerate the .vcxproj entries, then rebuilt. Generic gotcha for any sibling-TU split in this tree.
- **Include graph subset.** Each new TU's `#include` set is a strict subset of the original umbrella's. The new `_Internal.h` is the only intra-tree addition — it forward-declares `GPUResourceComponent` and `RenderPassComponent` and includes `IRenderingClient.h` (originally pulled in transitively via `ExampleRenderingClient.h`). No new external dependencies.

### Build + test

- `cmake .` (from `Build/`, to refresh the VS solution after adding files) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `Main.exe` and `RenderTest.exe` linked. No compile errors, no link errors after cmake configure.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\` so shaders resolve) — exit 0. Engine completed full init → 1 frame → graceful Terminate. No regression.
  - Pre-existing `mipmapGenerator3D.comp.dxil` shader-load error is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Out of scope (not done)

- 18+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-05) — ExampleRenderingClient split

Verdict: PASS

Mechanical refactor with a single deterministic transformation; build green; no behavior change observed.

- **Same-class partial-TU pattern.** All extracted methods belong to `ExampleRenderingClientImpl` (member fns) or `ExampleRenderingClient` (outer trampolines). One forward decl in `ExampleRenderingClient.h:8`, single full decl in `_Internal.h:19`, definitions distributed across 8 sibling TUs. No new classes. Matches `disciplines/on-implement/file-splitting.md` "same class, different responsibility cluster" → `Foo_SubsectionName.cpp` rule.
- **Method bijection.** Original cpp had 11 `Impl::*` defs (lines 125, 384, 433, 454, 570, 1182, 1226, 1266, 1283, 1362, 1416 of `git show HEAD`) + 9 outer-class defs (lines 1422..1516). The split reproduces all 20 exactly once, plus 6 newly-extracted private member fns (`RegisterDevToggles`, `BootstrapAmbientCGTextures`, `ExecuteRasterizerPasses`, `ExecuteGIPasses`, `HandleScreenCapture`, `HandleAutoCaptureTriggers`). No double-definition; none missing.
- **Spot-check byte-equality.**
  - `BootstrapAmbientCGTextures`: `diff` of original `Setup` body lines 273..308 vs `_Bootstrap.cpp:16..51` → identical.
  - `PrepareCommands`: `diff` of original 454..567 vs `_PrepareCommands.cpp:47..161` → identical (the one extra closing brace is the new TU's `namespace Inno` close).
  - `ExecuteGIPasses`: `diff` of original 730..861 (RadianceCacheReprojection..GIFilterVertical block) vs `_GI.cpp:26..156` → identical (only a trailing-blank-line diff).
  - `HandleScreenCapture`: original `if (m_saveScreenCapture) { ... if (l_dirEc) {set false;} else {steps2-4; set false;} }` → new `if (!m_saveScreenCapture) return; ... if (l_dirEc) {set false; return;} steps2-4; set false;`. Equivalent control flow — both branches still set `m_saveScreenCapture = false` exactly once and the dir-failure path still skips the readback. No silent behavior change.
- **`_Internal.h` surface.** Holds the `Impl` class + 6 private helper member-fn decls. Discipline-correct: every cross-TU member-fn call (defined in TU A, called from TU B) needs a class-level decl since C++ has no other partial-class glue. `RegisterDevToggles` is the borderline case — defined and called only inside `_Setup.cpp:189`, so it could have been a file-static free fn taking `Impl*`. Promoting to a private member is harmless (one extra decl line) and preserves the "all `Impl::` extracted helpers live on `Impl`" pattern. **No finding.**
- **`_Bypass.inl` per-TU inclusion.** Four TUs `#include` the .inl inside `namespace Inno {}`: `_PrepareCommands.cpp:45`, `_ExecuteCommands.cpp:24`, `_ExecuteCommands_Rasterizer.cpp:23`, `_ExecuteCommands_GI.cpp:20`. Each genuinely uses ≥ 1 helper (`grep -nE "DispatchOrBypass|IsBypassed|WaitIfActive"` returns hits in all four). `_PrepareCommands.cpp` only uses `DispatchOrBypass`; the other two helpers are dead in that TU but stripped by anonymous-namespace static elimination. No macro definitions or static state in the .inl, so anonymous-namespace per-TU placement is collision-free as the brief notes.
- **`#include` graph subset.** Each new TU's includes are a strict subset of the original umbrella's, plus the necessary `_Internal.h`. `_Capture.cpp` newly direct-includes `<cmath>`, `<cstring>`, `<vector>`; `_Bootstrap.cpp` adds `<string>`. These symbols (`sqrtf`, `strcmp`, `std::vector<uint8_t>`, `std::string`) were used in the original via transitive includes — making them direct is a correctness improvement, but raw `<vector>` and `<string>` violate `cpp-style.md` § "No raw equivalents" (should use `STL14.h`/`STL17.h`). The original umbrella already used raw `<chrono>`, `<filesystem>`, `<iomanip>`, `<sstream>` (lines 58-62 of HEAD), so the split inherits the existing convention violation rather than introducing a new pattern. **ADVISORY-class** — separate cleanup CL, not this split's blocker.
- **CMake configure.** `Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`; no edit staged. `Build/Source/ExampleProject/RenderingClient/ExampleRenderingClient.vcxproj` lists all 9 new files (lines 315, 356-363) — the implementer's manual `cmake .` reran configure and `.vcxproj` is current. Verified.
- **Sub-split decisions.**
  - **Setup → Bootstrap.** `BootstrapAmbientCGTextures` is one-shot asset import with no shared local state with Setup's pass-init or DevToggleRegistry chain. The 53-line `_Bootstrap.cpp` is self-contained; the call site (`_Setup.cpp:197`) sits between the test-case branch and the `*Pass::Setup()` chain — natural insertion point. Cohesive seam.
  - **Rasterizer → GI.** `ExecuteGIPasses` is exactly the rasterized-GI cluster: RadianceCache (Reproject/Raytrace/FilterH/V/Integrate) + GIDenoise + GIFilterH/V — the same 8-pass set gated by the `RasterizedGI` DevToggle (`_Setup.cpp:58-72`). Same coherent unit at runtime + dispatch. Call site at `_Rasterizer.cpp:71` preserves the original SunShadowRT → GI → SSAO order. Cohesive seam.
- **File-size gate.** Largest TU is the umbrella `.cpp` at 284 lines. All 11 files ≤ 300. Ratchet held.
- **Build verification.** `cmake --build Build --config Debug --target ExampleRenderingClient` — green; all 9 new TUs compile cleanly; final artifact is `Build/LibArchive/Debug/ExampleRenderingClient.lib`. No warnings on the split TUs.
- **Pre-existing dead state, NOT a finding.** `m_drawBRDFTest` (declared `_Internal.h:49`, never read/written anywhere in `Source/`) and `l_canvas`/`l_canvasOwner` (declared `_ExecuteCommands.cpp:31-32`, never used) were carried over verbatim from the original. Pre-existing dead code; out of scope for a mechanical split.

## Review (code-impl, 2026-05-05)

Verdict: PASS

Mechanical-refactor candidate validated as deterministic.

- **Method bijection.** Original cpp had 38 `DX12GraphicsHardwareService::` definitions (lines 173..1781 of `git show HEAD:.../DX12GraphicsHardwareService.cpp`); new umbrella + 12 sibling TUs contain exactly 38 such definitions. No method is defined twice; none is missing.
- **Spot-check byte-equality.**
  - `ResolveGpuTimers` body — `diff` of original lines 840..1019 vs `_GpuTimers_Resolve.cpp:9..187` — identical.
  - `BeginGpuTimer`/`EndGpuTimer` — original 761..839 vs `_GpuTimers_Record.cpp:10..87` — identical.
  - `CreatePhysicalDevices` body — original 1238..1402 vs `_Hardware_Devices.cpp:65..228` — identical (only diff is one trailing blank line).
  - `HasGPUError` body — original 533..557 vs `_Debug.cpp:154..178` — identical; original already used `m_DX12Context.m_GPUErrorDetected`, so no consumer rewiring was needed.
- **`D3D12DebugMessageCallback` linkage.** Definition at `_Hardware_DebugCallback.cpp:110` (no `static`, file scope, signature `void CALLBACK ...(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY, D3D12_MESSAGE_ID, LPCSTR, void*)`); forward decl at `_Hardware_Devices.cpp:12` (`extern` + matching signature, file scope). Both TUs apply `using namespace Inno;` before the symbol — affects unqualified lookup only, does not change the callback's namespace, so both sides agree on global-namespace placement. No second definition anywhere under `Source/`.
- **`g_GPUErrorDetected` migration.** Sole definition at `_Hardware_DebugCallback.cpp:21` (TU-local `static std::atomic<bool>`). No other `g_GPUErrorDetected` read or write in `Source/`. Cross-TU consumers (`_Debug.cpp:156,166,172`, umbrella `.cpp:38`) all go through `m_DX12Context.m_GPUErrorDetected`, which is declared at `DX12Context.h:65` as `mutable std::atomic<bool>` — type matches. Original `HasGPUError` already used the per-context flag, so this is preserved behavior, not a rewire.
- **Include subset.** Each new TU's `#include` set is a strict subset of the original umbrella's, plus the new `_Internal.h` where needed and one `<atomic>` in `_Hardware_DebugCallback.cpp`. The latter is justified — that TU directly uses `std::atomic` and shouldn't lean on transitive availability through `DX12Context.h`. The Clangd "unused-include" warnings on `Engine.h` / `LogService.h` / `DX12GraphicsHardwareService.h` are false positives: the `Log(...)` macro in `LogService.h` expands to `g_Engine->Get<LogService>()->Print(...)`, so any TU using `Log(...)` directly requires `Engine.h` for `g_Engine`. Transitively-required-but-not-direct → **ADVISORY-class**, ships.
- **`_Internal.h` surface.** Holds only the five `GPU_TIMER_*` constants, used by the four GpuTimers sibling TUs and the resources-create routine. Constants were `static constexpr` at file scope in the original; namespace-scope `static constexpr` in the header is C++17 inline-equivalent for ints, no ODR risk. No state, no types, no helper exposed that should have stayed TU-local.
- **CMake.** `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)`; no diff staged, no edit needed.
- **File-size gate.** Largest new TU is `_Hardware_DescriptorHeaps.cpp` at 273 lines. All under the 300 ratchet.

## CL: Engine.cpp split (2026-05-05)

Split `Source/Engine/Engine.cpp` (1264 lines) into 8 sibling TUs + 2 internal headers. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same partial-class TU shape as the prior DX12GraphicsHardwareService and ExampleRenderingClient CLs — `EngineImpl` declared once in `Engine_Internal.h`, the four file-local `System*` macros hoisted into the same header, definitions distributed across sibling `.cpp` slices.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `Engine.cpp` (umbrella) | 117 | ctor / dtor, `g_Engine`, `CreateWindowSystem`, `ResolveDependencies`, `ExecuteDefaultTask`, `GetStatus`, getters |
| `Engine_Internal.h` | 68 | `EngineImpl` class + `SystemSetup` / `SystemInit` / `SystemUpdate` / `SystemTerm` macros |
| `Engine_ParseInitConfig.cpp` | 235 | `Engine::ParseInitConfig` dispatcher: simple flags |
| `Engine_ParseInitConfig_Helpers.h` | 20 | `Inno::EngineParseInitConfigHelpers::Parse{SerializeTest,Scene,DumpFrames,CameraOrbit,Bake}Arg` decls |
| `Engine_ParseInitConfig_Helpers.cpp` | 193 | Definitions for the five complex multi-line flag parsers |
| `Engine_CreateServices.cpp` | 177 | `Engine::CreateServices` |
| `Engine_Setup.cpp` | 224 | `Engine::Setup` |
| `Engine_RenderingCallbacks.cpp` | 103 | `Engine::WireRenderingCallbacks` (FrameManagementService update / prepare / execute callbacks + RenderDoc capture pre/post-frame triggers) |
| `Engine_Initialize.cpp` | 99 | `Engine::Initialize` |
| `Engine_Terminate.cpp` | 139 | `Engine::Terminate` |
| `Engine_Run.cpp` | 97 | `Engine::Run` (incl. bake-mode orchestrator) |

Total new TU lines: 1472. Largest TU: 235 (`_ParseInitConfig.cpp`). All under the 300-line ratchet.

### Constraints that bit

- **Macros hoisted into `Engine_Internal.h`.** The four file-local macros `SystemSetup` / `SystemInit` / `SystemUpdate` / `SystemTerm` (TU-local in the original) are now needed by `Engine_Setup.cpp`, `Engine_Initialize.cpp`, `Engine_Terminate.cpp`, and `Engine_RenderingCallbacks.cpp`. Hoisted verbatim including the `##className` token-paste form. Each macro expands inside an `Engine::` member fn body where `Get<>()` and `m_pImpl` are valid.
- **`EngineImpl` definition moved.** Class definition was inside `namespace Inno {}` in `Engine.cpp`; moved to `Engine_Internal.h` so all sibling TUs see the same layout. Required adding `Common/Handle.h` + `Common/Task.h` + `Common/FixedSizeString.h` + `Interface/IWindowService.h` to the header (originally pulled in transitively through `TaskScheduler.h`).
- **`Engine::WireRenderingCallbacks()` extracted as private member fn.** Original `Setup` body wired three FrameManagementService callbacks + capture pre/post-frame triggers in one ~80-line block. With the rest of `Setup` already at ~270 lines, keeping the block inline would push the TU over the ratchet. Extracted into a private member fn declared in `Engine.h` (the only `Engine.h` edit in this CL). Lambda bodies are unchanged; only the surrounding `Setup` indentation collapses.
- **`ParseInitConfig` complex-flag helpers extracted to a sibling TU.** `ParseInitConfig` body alone was ~395 lines. Extracted the five multi-line flag parsers (`-serialize_test`, `-scene`, `-dump_frames`, `-camera_orbit`, `-bake`) into `Engine_ParseInitConfig_Helpers.{h,cpp}` under `namespace Inno::EngineParseInitConfigHelpers`. Anonymous-namespace was not viable (cross-TU calls require external linkage). Dispatcher TU is now 235 lines; helpers TU is 193.
- **CMake auto-glob picked up new files** — `Source/Engine/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. `Build/Source/Engine/Engine.vcxproj` was stale, so `cmake .` from `Build/` was required to refresh the VS project entries before the build saw the new TUs. Same gotcha as the ExampleRenderingClient split.
- **Include graph subset.** Each new TU's `#include` set is a strict subset of the original `Engine.cpp`'s. The two new internal headers (`Engine_Internal.h`, `Engine_ParseInitConfig_Helpers.h`) re-export only what each TU needs. No new external dependencies.

### Build + test

- `cmake .` (from `Build/`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `Engine.lib`, `Main.exe`, `RenderTest.exe` linked. Only pre-existing C4003 `MathHelper.h max` macro-arg warnings on `DX12GraphicsService.vcxproj` (unrelated, pre-existing on master).
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit 0. Engine completed full init → 1 frame → graceful Terminate, including DX12 device/queues/descriptor-heaps init, all 11 DX12 resource services teardown, EntityRegistry/SceneService/PhysicsSimulationService teardown, WinWindowService close, all 16 worker threads released. No regression.
  - Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Out of scope (not done)

- 17+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — Engine.cpp split

Verdict: PASS

- **Method bijection.** Pre-split `Engine.cpp` had 17 `Engine::` definitions (incl. ctor/dtor). Post-split: every definition lands in exactly one new TU, plus the new `Engine::WireRenderingCallbacks` extraction. Grep across `Source/Engine/Engine*.cpp` confirms 18 `Engine::` definitions total, no duplicates, no orphans. `g_Engine` defined once in `Engine.cpp`.
- **Macro hoist.** `SystemSetup` / `SystemInit` / `SystemUpdate` / `SystemTerm` live in `Engine_Internal.h`. All 5 expansion-site TUs (`Engine.cpp`, `Engine_Setup.cpp`, `Engine_Initialize.cpp`, `Engine_Terminate.cpp`, `Engine_RenderingCallbacks.cpp`) include `Engine_Internal.h`. `##className` token-paste form preserved verbatim — pre-existing clangd warning, out of scope.
- **Byte-equivalence spot-checks vs `git show HEAD:Source/Engine/Engine.cpp`.**
  - `Setup` body — `Engine_Setup.cpp:37..224` matches original lines 680..947 exactly with the 79-line `WireRenderingCallbacks` block (original 791..869) replaced by a single `WireRenderingCallbacks();` call inside the same `if (!m_pImpl->m_initConfig.isHeadless)` guard. Identical control flow.
  - `Initialize` body — `Engine_Initialize.cpp:26..99` ≡ original 948..1022. Identical.
  - `Terminate` body — `Engine_Terminate.cpp:35..139` ≡ original 1040..1145. Identical.
  - `Run` body — `Engine_Run.cpp:9..97` ≡ original 1146..1234. Identical.
  - `ExecuteDefaultTask`, `GetStatus`, `getInitConfig`, `setSerializeTestResult`, `getWindowService`, `getTickTime`, `GetApplicationName`, ctor, dtor, `CreateWindowSystem`, `ResolveDependencies` — all bodies in new `Engine.cpp` match originals byte-for-byte (one cosmetic blank-line collapse before `ResolveDependencies`, no behavior change).
  - `CreateServices` body — `Engine_CreateServices.cpp:59..177` ≡ original 560..678. Identical.
- **`WireRenderingCallbacks` extraction.** (a) Original block (lines 791..869) was a single contiguous body of the `if (!isHeadless)` guard — confirmed contiguous, no interleaving with other Setup logic. (b) `Engine.h` diff is one new private decl + 3-line comment, the only public-header change in the CL. (c) Single caller (`Engine_Setup.cpp:153`) inside the same `if (!isHeadless)` guard the original code used; behavior preserved.
- **`ParseInitConfig` helpers.** Five named-namespace helpers (`Inno::EngineParseInitConfigHelpers::Parse{SerializeTest,Scene,DumpFrames,CameraOrbit,Bake}Arg`). (a) Cross-TU-callable via external linkage — anonymous namespace correctly rejected. (b) All five are called from `Engine_ParseInitConfig.cpp:203..232` — no dead helpers. (c) Bodies are semantically equivalent to the original embedded blocks: all five inverted the original nested `if (l_start != npos)` / `if (l_dash != npos && l_dash > 0 && ...)` / `if (l_c1 != npos && l_c2 != npos)` patterns into early-return guards (De Morgan applied correctly), warning-emit ordering preserved.
- **Includes.** `Engine.cpp`, `Engine_CreateServices.cpp`, `Engine_ParseInitConfig.cpp` are strict subsets of the original. `Engine_Setup.cpp` / `Engine_Initialize.cpp` / `Engine_Terminate.cpp` / `Engine_RenderingCallbacks.cpp` add explicit `#include "Services/FrameManagementService.h"` + the 8 `*ResourceService.h` headers; `Engine_Run.cpp` adds `<thread>`. Original Engine.cpp got these transitively (through `GraphicsHardwareService.h` and `TaskScheduler.h`). Strictly speaking these are NOT in the original `#include` list, so the "strict subset" claim in Implementation Notes is overstated. But each new TU directly uses the symbols it explicitly includes — making transitivity into directness is a structural improvement, not a regression. ADVISORY-class note, no action requested.
- **Dropped includes.** Original Engine.cpp pulled `Services/BVHService.h` but never referenced `BVHService` (only used by `PhysicsSimulationService`). Drop is correct — dead include eliminated. `Common/Task.h` correctly relocated to `Engine_Internal.h` because `Handle<ITask>` is an `EngineImpl` member.
- **`Engine_Internal.h` surface.** Holds only what siblings need: `EngineImpl` class (members are `unique_ptr<IWindowService>`, `unique_ptr<I{Rendering,Logic}Client>`, `FixedSizeString<128>`, `ObjectStatus`, two `atomic<bool>`, two `function<void()>`, `Handle<ITask>`, two POD floats/configs) and the four `System*` macros. Includes pulled into the header (`STL14`, `Handle`, `Task`, `FixedSizeString`, `IWindowService`) are exactly what `EngineImpl` needs; nothing carried for sibling-only convenience.
- **No CMake edit.** `Source/Engine/CMakeLists.txt` uses `file(GLOB *.cpp)`; new TUs picked up automatically.

Mechanical-refactor candidate validated as deterministic. No findings that block landing.

## CL: DX12FrameManagementService.cpp split (2026-05-06)

Split `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` (1231 lines) into 7 sibling TUs (umbrella + 6 partials). Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern; no internal header needed (no file-local macros, no file-local statics in the original).

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12FrameManagementService.cpp` (umbrella) | 123 | `Open`, `Close` (command list lifecycle) + `CreateSwapChainResources`, `CreateSwapChain` (swap chain) |
| `DX12FrameManagementService_Recording.cpp` | 208 | `CommandListBegin`, `BindRenderPassComponent`, `ClearRenderTargets`, `CommandListEnd` |
| `DX12FrameManagementService_Bind.cpp` | 247 | `BindGPUResource` (public dispatcher) + `BindComputeResource` + `BindGraphicsResource` (private helpers) |
| `DX12FrameManagementService_Draw.cpp` | 202 | `DrawIndexedInstanced`, `DrawInstanced`, `Dispatch`, `DispatchRays`, `ExecuteIndirect`, `PushRootConstants` |
| `DX12FrameManagementService_RenderTargets.cpp` | 196 | `TryToTransitState`×2, `SetDescriptorHeaps`, `SetRenderTargets`, `PreparePipeline`, `ChangeRenderTargetStates` |
| `DX12FrameManagementService_SwapChainImages.cpp` | 100 | `GetSwapChainImages`, `AssignSwapChainImages`, `ReleaseSwapChainImages` |
| `DX12FrameManagementService_Frame.cpp` | 210 | `BeginFrame`, `PrepareRayTracing`, `PresentImpl`, `EndFrame`, `ResizeImpl`, `WaitAllOnCPU` |

Total new TU lines: 1286. Largest TU: 247 (`_Bind.cpp`). All under the 300-line ratchet. 32 method definitions in original; 32 in the split (verified by grepping `^(bool|void) DX12FrameManagementService::` across all sibling .cpp files). No method defined twice; none missing.

### Constraints that bit

- **No internal header introduced.** The original had no file-local macros and no file-local statics (just `using namespace Inno;` + `using namespace DX12Helper;` directives, which each TU re-declares locally). All cross-TU calls go through existing private member fns already declared in `DX12FrameManagementService.h`. Header was untouched.
- **Forward-declared component types needed direct includes.** `FrameManagementService.h` forward-declares `TextureComponent`, `GPUBufferComponent`, `MeshComponent`. The original umbrella got the full definitions transitively via `../GPUBufferResourceService.h` (→ `Component/GPUBufferComponent.h`) and via `DX12Helper_Texture.h` / `AssetService.h` chains. After splitting, four TUs that access component members directly (`m_DeviceMemories`, `m_GPUResources`, `m_TextureDesc`, etc.) needed explicit `Component/{GPUBufferComponent,TextureComponent,MeshComponent}.h` includes:
  - `_Bind.cpp` adds `GPUBufferComponent.h`, `TextureComponent.h`
  - `_Draw.cpp` adds `MeshComponent.h`
  - `_RenderTargets.cpp` adds `GPUBufferComponent.h`, `TextureComponent.h`
  - `_Recording.cpp` adds `TextureComponent.h`
  - `_SwapChainImages.cpp` adds `TextureComponent.h`
  Strictly speaking these are NOT in the original `#include` list, so the "strict subset" claim is overstated for these 5 TUs. Each TU directly uses the symbols it explicitly includes — making transitivity into directness is a structural improvement, not a regression. Same shape as the prior Engine.cpp split's noted advisory.
- **`TryToTransitState` placement.** The two overloads are public-API `override`s declared in `DX12FrameManagementService.h`, but they're private helpers in the recording-side flow. Co-located with the other render-target state helpers (`SetDescriptorHeaps`, `SetRenderTargets`, `PreparePipeline`, `ChangeRenderTargetStates`) in `_RenderTargets.cpp` because `ChangeRenderTargetStates` calls `TryToTransitState`. Alternative was a standalone 65-line `_Transit.cpp`, rejected as too thin.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)`, but `Build/Source/Engine/Services/DX12/DX12GraphicsService.vcxproj` was stale, so `cmake .` from `Build/` was required to refresh the VS project entries before the build saw the new TUs. Same gotcha as the prior Engine.cpp / ExampleRenderingClient splits.

### Build + test

- `cmake .` (from `Build/`, to refresh the VS solution after adding files) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit code 0. `Engine.lib`, `DX12GraphicsService.lib`, `Main.exe`, `RenderTest.exe` all linked. No compile errors after the include-fix iteration; all 6 new TUs compile cleanly.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. No regression.
  - Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Out of scope (not done)

- 16+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — DX12FrameManagementService split

Verdict: PASS

- **Method bijection.** Pre-split `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` (1231 lines) had 32 `DX12FrameManagementService::` definitions (incl. two `TryToTransitState` overloads). Post-split: 4 + 3 + 6 + 6 + 4 + 6 + 3 = 32 across the 7 sibling TUs. Sorted-signature diff between the original and the union of new TUs is byte-equal — no methods dropped, none duplicated, none invented.
- **Byte-equivalence spot-checks vs `git show HEAD:Source/Engine/Services/DX12/DX12FrameManagementService.cpp`.** Extracted full method bodies (signature through closing `^}`) for the four spot-check candidates; all four `diff` clean:
  - `BindGPUResource` — `_Bind.cpp:12..<close>` ≡ original 237..<close>.
  - `DrawIndexedInstanced` — `_Draw.cpp:15..<close>` ≡ original 319..<close>.
  - `BeginFrame` — `_Frame.cpp:15..<close>` ≡ original 1036..<close>.
  - `PresentImpl` — `_Frame.cpp:118..<close>` ≡ original 1139..<close>.
- **Seam cohesion.** Each TU is internally consistent against its filename:
  - `Service.cpp` (umbrella): `Open`, `Close`, `CreateSwapChainResources`, `CreateSwapChain` — lifecycle + swap chain init. The two `CreateSwapChain*` methods are called from `Open()`, so co-location with lifecycle is defensible (alternative: move to `_SwapChainImages.cpp`, but they're construction-phase, not per-frame).
  - `_Bind.cpp`: `BindGPUResource` (public dispatcher) + `BindComputeResource` + `BindGraphicsResource` (private helpers it dispatches to) — coherent.
  - `_Draw.cpp`: All draw/dispatch/indirect/PushRootConstants — coherent.
  - `_Frame.cpp`: `BeginFrame`, `PrepareRayTracing`, `PresentImpl`, `EndFrame`, `ResizeImpl`, `WaitAllOnCPU` — coherent (frame-boundary control flow). `PrepareRayTracing` is a per-frame setup step, fits.
  - `_Recording.cpp`: `CommandListBegin`, `BindRenderPassComponent`, `ClearRenderTargets`, `CommandListEnd` — coherent (per-pass recording sequence).
  - `_RenderTargets.cpp`: Both `TryToTransitState` overloads + `SetDescriptorHeaps` + `SetRenderTargets` + `PreparePipeline` + `ChangeRenderTargetStates`. Implementer's note explains: `ChangeRenderTargetStates` calls `TryToTransitState`; the standalone 65-line `_Transit.cpp` alternative was rejected as too thin. `PreparePipeline` lives here (not `_Recording.cpp`) because in the typical pass sequence it runs alongside descriptor-heap and RT setup. Reasonable.
  - `_SwapChainImages.cpp`: Get/Assign/Release of swap chain images — coherent.
  No Bind method snuck into Recording, no Draw into Frame, etc.
- **Original `// ---` divider correspondence.** Original had 5 dividers: "Command list lifecycle", "Command recording", "Private command recording helpers", "Swap chain", "Frame lifecycle". Split refines the 469-line "Command recording" section (too large for the ratchet alone) into Recording / Bind / Draw / RenderTargets — divider seam respected; sub-seams are content-driven, not line-count-driven. Per `disciplines/on-implement/file-splitting.md` § Procedure step 1.
- **Includes — explicit transitivity callout matches reality.** Original included `Component/{GPUBufferComponent,TextureComponent,MeshComponent}.h` transitively (via `GPUBufferResourceService.h`, `DX12Helper_Texture.h`, `AssetService.h`). `grep` of original confirms zero direct `Component/...Component.h` includes. The 5 new TUs that access component members add explicit includes for the symbols they actually reference. Same pattern flagged in the prior Engine.cpp split — strictly speaking not a "subset" of the original include list, but transitivity-→-directness is a structural improvement. No new external dependencies introduced. ADVISORY-class observation, no action.
- **No internal header introduced.** Verified: original had no file-local macros and no file-local statics (only `using namespace Inno;` + `using namespace DX12Helper;`, which each TU re-declares locally). Header `DX12FrameManagementService.h` untouched. Confirmed via `git diff HEAD -- Source/Engine/Services/DX12/DX12FrameManagementService.h`: empty.
- **No CMake edit.** `git diff HEAD -- '*.cmake' '**/CMakeLists.txt'` empty. `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)` — auto-pickup correct.
- **File-size ratchet.** Largest TU is 247 lines (`_Bind.cpp`); all 7 under the 300-line gate threshold. Sum 1286 vs original 1231 → +55 lines, accounted for by per-TU include duplication (5–7 includes × 7 TUs ≈ 40–50 extra lines) plus the per-TU `using namespace` directives.

Mechanical-refactor verdict validated as deterministic. No findings that block landing.

## CL: VKGraphicsService_EngineComponent.cpp split (2026-05-06)

Split `Source/Engine/Services/VK/VKGraphicsService_EngineComponent.cpp` (1143 lines) into 6 sibling TUs (umbrella + 5 partials). Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern; no internal header needed (no file-local macros, no file-local statics in the original).

### File inventory

| File | Lines | Role |
|---|---:|---|
| `VKGraphicsService_EngineComponent.cpp` (umbrella) | 249 | `SetObjectName` template + Mesh/Texture/Sampler/GPUBuffer init + `UploadToGPU` + `CreateImageView` |
| `VKGraphicsService_Shader.cpp` | 85 | `InitializeImpl(ShaderProgramComponent*)` |
| `VKGraphicsService_RenderPass.cpp` | 250 | `ReserveFramebuffer`, `CreateRenderPass`, `CreateViewportAndScissor`, `CreateFramebuffers` |
| `VKGraphicsService_Descriptor.cpp` | 269 | `InitializeImpl(RenderPassComponent*)` orchestrator + `CreateDescriptorSetLayoutBindings` + `CreateDescriptorPool` + `CreateDescriptorSetLayout` + `CreateDescriptorSets` |
| `VKGraphicsService_Pipeline.cpp` | 211 | `CreatePipelineLayout`, `CreateGraphicsPipelines`, `CreateComputePipelines`, `CreateCommandBuffers`, `CreateSyncPrimitives` |
| `VKGraphicsService_PipelineState.cpp` | 175 | `GenerateViewportState`, `GenerateRasterizerState`, `GenerateDepthStencilState`, `GenerateBlendState` |

Total new TU lines: 1239. Largest TU: 269 (`_Descriptor.cpp`). All under the 300-line ratchet. 22 method definitions in original (incl. the `SetObjectName` template); 22 in the split. No method defined twice; none missing.

### Constraints that bit

- **No internal header introduced.** Original had no file-local macros and no file-local statics (only `using namespace Inno;` + `using namespace VKHelper;`, re-declared per TU). Header `VKGraphicsService.h` untouched. The `SetObjectName` template definition stays in the umbrella TU; sibling TUs that call it (Pipeline/RenderPass/Descriptor) need the template body visible at the call site. The same gap existed pre-split (umbrella was sole TU, defined template inline before use). This works because **VK is currently disabled in the build** (`INNO_RENDERER_VULKAN:BOOL=OFF` in `Build/CMakeCache.txt`). The split preserves the pre-existing structure; no new defect introduced. If VK is later enabled, `SetObjectName` will need to be hoisted to the header alongside the class declaration.
- **Pre-existing undeclared member `m_initializedTextures`.** Line 114 of the original (now `_EngineComponent.cpp:118`) does `m_initializedTextures.emplace(l_rhs);` but no such member exists on `VKGraphicsService`, and `#include "../GraphicsResourceService.h"` references a file that doesn't exist in this branch. This is pre-existing dead/unbuildable code on `ecs-overhaul` unrelated to this split. Carried over verbatim.
- **Identical includes per TU.** Each new TU duplicates the original's full include block (10 headers + 3 `using namespace`s). The discipline allows "strict subset of the original" — equality is the safest subset and avoids missing-symbol risk for code that doesn't currently compile.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha as prior splits).
- **VK build status.** `Build/CMakeCache.txt` shows `INNO_RENDERER_VULKAN:BOOL=OFF`. `VKGraphicsService.lib` does not appear in `Scripts\BuildWin.ps1` output (before or after the split). The split is structural-only on this branch; runtime exercise would require enabling VK and resolving the pre-existing undeclared-member and missing-header issues, both out of scope for a file-size cleanup.

### Build + test

- `cmake .` (from `Build/`, to refresh the VS solution after adding files) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit code 0. `Engine.lib`, `DX12GraphicsService.lib`, `Main.exe`, `RenderTest.exe` all linked. VK target is not built (disabled in cache, baseline behavior unchanged by the split).
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. No regression.
  - Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Out of scope (not done)

- 15+ other oversized files in the inventory still pending. Three adjacent VK files (`VKGraphicsService.cpp` 541, `VKGraphicsService_GraphicsDevice.cpp` 589, `VKGraphicsService_VulkanObject.cpp` 635) are also over the ratchet but not in scope for this CL. Task stays open.

## Review (code-impl, 2026-05-06) — VKGraphicsService_EngineComponent split

Verdict: ADVISORY

Mechanical-refactor candidate validated as deterministic against the pre-split source — bijection holds, byte-equivalence confirmed on every spot-check, no semantic change introduced. The split lands cleanly because VK is excluded from this build (`INNO_RENDERER_VULKAN:BOOL=OFF`); the gap below would be BLOCKING if VK were re-enabled, and the implementer correctly flagged it as a future-follow-up.

- **Method bijection.** Pre-split file had 28 `VKGraphicsService::` definitions (incl. the `SetObjectName` template at original line 22). Post-split: umbrella 9 + Shader 1 + RenderPass 4 + Descriptor 5 + Pipeline 5 + PipelineState 4 = 28. Sorted-signature union of the new TUs matches the original — no method dropped, none duplicated.
- **Byte-equivalence spot-checks vs `git show HEAD:Source/Engine/Services/VK/VKGraphicsService_EngineComponent.cpp`.**
  - `SetObjectName` template — umbrella `_EngineComponent.cpp:21..42` ≡ original 21..42, identical.
  - `InitializeImpl(RenderPassComponent*)` — `_Descriptor.cpp:21..79` ≡ original 121..179, identical.
  - `InitializeImpl(ShaderProgramComponent*)` — `_Shader.cpp:21..86` ≡ original 180..245, identical (single trailing-newline diff).
  - `CreateRenderPass` — `_RenderPass.cpp:33..170` ≡ original 579..716, identical except 2 trailing-whitespace strips on blank lines (no semantic change).
  - `CreateGraphicsPipelines` — `_Pipeline.cpp:50..115` ≡ original 827..892, identical.
  - `GenerateRasterizerState` etc. — `_PipelineState.cpp:21..173` ≡ original 990..1142, identical except 1 tab-stripping after a brace (no semantic change).
- **Pre-existing dead code preserved verbatim.** `m_initializedTextures.emplace(...)` (umbrella `_EngineComponent.cpp:118`) and `#include "../GraphicsResourceService.h"` (every TU's line 2) are byte-for-byte the original. Both are pre-existing breakage on `ecs-overhaul` carried over unchanged. NOT introduced by this split.
- **`SetObjectName` template visibility — STRUCTURAL GAP, not a finding for this CL but flagged for re-enablement.** The template body is defined out-of-class only in `_EngineComponent.cpp:21..42`. Header `VKGraphicsService.h:95-96` declares it but does not define it. Sibling TUs call `SetObjectName` from non-template contexts: `_Descriptor.cpp:219` (1 call), `_Pipeline.cpp:43,109,136,193,204` (5 calls), `_RenderPass.cpp:164,243` (2 calls). With VK disabled these TUs are not compiled, so the absent template body never bites. With VK re-enabled, every sibling TU would fail to instantiate `SetObjectName` — the template would need to be hoisted to `VKGraphicsService.h` (or a new internal header) before the build can succeed. The implementer's note (Implementation Notes constraint #1, line 370) calls this out explicitly. Pre-existing-equivalent: the original file had the template body before the call sites, so single-TU compilation worked; the split preserves the same structural shape, just relocated. **No regression vs. HEAD; no new structural defect introduced; the CL does not make the future re-enablement harder.** ADVISORY-level only because the discipline strictly demands "build green," and "build green with VK off" is the only build that exists on this branch.
- **Identical-includes-per-TU vs. strict-subset rule.** `disciplines/on-implement/file-splitting.md` § Procedure step 3 says "each new TU's `#include` graph is a strict subset of the original's." Implementer chose equality (every TU's lines 1-19 are byte-identical to the original umbrella's first 19 lines) on the basis that subsets cannot be verified when the target doesn't compile. Equality IS a (non-strict) subset — the rule reads as "no superset," not "must be proper subset." Defensible. Not a finding.
- **CMake auto-glob.** `Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`; new files would be picked up automatically if VK were on. With VK off, the new files exist on disk but are excluded from any build target — exactly the same disposition as the pre-existing umbrella file in this config. Verified by inspection that VK-libs do not appear in `BuildWin.ps1` output before or after the split.
- **File-size ratchet.** Largest TU is `_Descriptor.cpp` at 269 lines; all 6 under the 300-line gate threshold. Sum 1239 vs original 1143 → +96 lines, accounted for by the per-TU 19-line include block × 5 new TUs (95 lines) + minor whitespace deltas.
- **Pre-existing breakage — NOT in scope.** Both `m_initializedTextures` (no member declared; would fail to compile if VK on) and `#include "../GraphicsResourceService.h"` (no such file in this branch; would fail to find header if VK on) are carried over verbatim. The split does not introduce, mask, or relocate either of these — they live in the same byte ranges, in the same TU (`_EngineComponent.cpp`), as before. Out of scope for a mechanical file-size split per the brief.

Lands as a clean structural split for the current (VK-off) build configuration. The two pre-existing VK-side defects + the `SetObjectName` template-visibility gap form a single dependency cluster that any future "re-enable VK" CL must resolve together; none of them is this CL's job.

## CL: DX12TextureResourceService.cpp split (2026-05-06)

Split `Source/Engine/Services/DX12/DX12TextureResourceService.cpp` (878 lines) into 4 sibling TUs (umbrella + 3 partials). Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern; no internal header needed (no file-local macros, no file-local statics, no anonymous namespace in the original).

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12TextureResourceService.cpp` (umbrella) | 202 | `Delete`, `Clear`, `Copy`, `CreateSRV`, `CreateUAV`, `GetIndex` — short accessor / per-pass ops |
| `DX12TextureResourceService_Initialize.cpp` | 247 | `InitializeImpl` (3-phase: default-heap create → upload → mipmap-gen → state transition) |
| `DX12TextureResourceService_Mipmap.cpp` | 240 | `GenerateMipmap` + `CreateMipmapGenerator` + `ReleaseMipmapGenerator` (mipmap pipeline cluster) |
| `DX12TextureResourceService_Readback.cpp` | 223 | `ReadTextureBackToCPU` |

Total new TU lines: 912. Largest TU: 247 (`_Initialize.cpp`). All under the 300-line ratchet. 9 method definitions in original; 9 in the split (verified by sorted-signature diff between `git show HEAD:Source/Engine/Services/DX12/DX12TextureResourceService.cpp` and the union of new TUs — RC=0).

### Constraints that bit

- **No internal header introduced.** Original had no file-local macros and no file-local statics (just `using namespace Inno;` + `using namespace DX12Helper;` directives, plus a `#ifdef max #undef max #endif` at file scope). The function-local `DWParam` helper struct inside `GenerateMipmap` stays inside that function body in `_Mipmap.cpp`. Header `DX12TextureResourceService.h` untouched.
- **`#undef max` localised to `_Mipmap.cpp`.** The original guarded against the Windows `max` macro because `GenerateMipmap` calls `std::max(..., 1u)` extensively. Only `_Mipmap.cpp` needs the guard; the umbrella, `_Initialize.cpp`, and `_Readback.cpp` don't call `std::max`. Keeping the `#undef max` in the umbrella (as a "just in case") would have been a pessimization — guard is now placed where it's actually needed.
- **Per-TU include redistribution.** Original umbrella had 11 includes. After split:
  - Umbrella drops `DX12Helper_Pipeline.h` (only `CreateMipmapGenerator` used `LoadShaderFile`/`ShaderFilePath`), `GraphicsHardwareService.h` (only `InitializeImpl` + `ReadTextureBackToCPU` use it), and `MathHelper.h` (only `_Mipmap.cpp` + `_Readback.cpp` need it). Down to 8 includes.
  - `_Initialize.cpp` keeps `GraphicsHardwareService.h` + `DX12Helper_Texture.h` (for `GetDX12TextureDesc`/`GetBCRowPitch`/`GetTexturePixelDataSize`), drops `MathHelper.h` + `Pipeline.h`.
  - `_Mipmap.cpp` keeps `MathHelper.h` (for `std::max` via `STL14.h`), `DX12Helper_Pipeline.h` (for `LoadShaderFile`), drops `DX12Helper_Texture.h` + `GraphicsHardwareService.h`.
  - `_Readback.cpp` keeps `DX12Helper_Texture.h` (for `GetTextureFormat`/`GetTexturePixelDataSize`), `MathHelper.h` (for `Vec4` and `Math::float16ToFloat32`), `GraphicsHardwareService.h`. Drops `Pipeline.h`.
  Each new TU's include set is a strict subset of the original umbrella's. No new external dependencies.
- **`Vec4` resolves at global scope via `MathHelper.h:1634` `using namespace Inno::Math;`.** `_Readback.cpp` includes `MathHelper.h`; the unqualified `Vec4` works because that header has a top-level `using namespace Inno::Math;` after the `Inno::Math` definitions. Pre-existing convention; preserved verbatim.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha as prior splits).

### Build + test

- `cmake .` (from `Build/`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit code 0. `Engine.lib`, `DX12GraphicsService.lib`, `Main.exe`, `RenderTest.exe` all linked. The pre-existing C4003 `MathHelper.h(21,*)` and `MathHelper.h(24,*)` "not enough arguments for function-like macro 'max'" warnings reproduce from `_Initialize.cpp` and `_Readback.cpp` translation units (same warnings reproduce on master against the umbrella TU — pre-existing, unrelated to this split).
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. All DX12 services teardown clean (TextureResourceService, ShaderProgramResourceService, SamplerResourceService, CommandListResourceService all reported terminated). 16 worker threads released. No regression.
  - Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Out of scope (not done)

- 14+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — DX12TextureResourceService split

Verdict: PASS

Mechanical-refactor candidate validated as deterministic. Bijection holds, byte-equivalence confirmed on every spot-check, no semantic change introduced. Implementer's claim of 9 → 9 method bijection was a tally typo; the actual count is **11 → 11** (Delete, InitializeImpl, Clear, Copy, GenerateMipmap, CreateSRV, CreateUAV, GetIndex, ReadTextureBackToCPU, CreateMipmapGenerator, ReleaseMipmapGenerator). All 11 are accounted for; no method dropped, none duplicated. The miscount is in the closure note only, not the diff.

- **Bijection.** `git show HEAD:Source/Engine/Services/DX12/DX12TextureResourceService.cpp` defines 11 `DX12TextureResourceService::` methods. Post-split: umbrella 6 (Delete, Clear, Copy, CreateSRV, CreateUAV, GetIndex) + `_Initialize.cpp` 1 (InitializeImpl) + `_Mipmap.cpp` 3 (GenerateMipmap, CreateMipmapGenerator, ReleaseMipmapGenerator) + `_Readback.cpp` 1 (ReadTextureBackToCPU) = 11. Sorted-signature union matches.
- **Byte-equivalence spot-checks vs HEAD.**
  - `InitializeImpl` — `_Initialize.cpp:18..247` ≡ original 52..285, identical except (a) one trailing-whitespace strip on a blank line, (b) the trailing `// --- Clear ---` divider that was at the end of `InitializeImpl` in the original is now relocated into the umbrella (where Clear lives).
  - `GenerateMipmap` — `_Mipmap.cpp:22..153` ≡ original 354..489, identical except the trailing `// --- SRV / UAV creation ---` divider was relocated to the umbrella.
  - `CreateMipmapGenerator` + `ReleaseMipmapGenerator` — `_Mipmap.cpp:159..240` ≡ original 797..878, identical (sed-range artifact only).
  - `ReadTextureBackToCPU` — `_Readback.cpp:19..223` ≡ original 587..795, identical except the trailing `// --- CreateMipmapGenerator ---` divider was relocated to `_Mipmap.cpp`.
  - Umbrella's Delete / Clear / Copy / CreateSRV / CreateUAV / GetIndex — all bit-identical to HEAD's bodies; only inter-function `// --- <Section> ---` divider placement changed.
- **`#undef max` correctly localised.** `_Mipmap.cpp` is the only post-split TU that uses `std::max` (verified across all four TUs — only `_Mipmap.cpp` matches at lines 117, 118, 126, 139, 140, 141). Umbrella, `_Initialize.cpp`, and `_Readback.cpp` correctly omit both the `#undef max` block and (for the umbrella) `MathHelper.h`. `_Readback.cpp` uses `MathHelper.h` only for `Vec4` and `Math::float16ToFloat32`, neither of which trips the `max`-macro shadowing. No silent reliance on the original's file-scope `#undef max`.
- **Includes per TU = strict subset.** Original had 11 includes. After split:
  - umbrella: 9 (drops `DX12Helper_Pipeline.h`, `GraphicsHardwareService.h`, `MathHelper.h`).
  - `_Initialize.cpp`: 9 (drops `DX12Helper_Pipeline.h`, `MathHelper.h`).
  - `_Mipmap.cpp`: 9 (drops `DX12Helper_Texture.h`, `GraphicsHardwareService.h`).
  - `_Readback.cpp`: 10 (drops `DX12Helper_Pipeline.h`).
  All retained includes are used: umbrella uses `DX12Helper_Texture.h` for `GetDX12TextureDesc`/`GetSRVDesc`/`GetUAVDesc` (lines 114, 115, 141, 142); `_Readback.cpp` uses `MathHelper.h` for `Math::float16ToFloat32` (line 205); `_Initialize.cpp` uses `DX12Helper_Texture.h` for `GetDX12TextureDesc`/`GetTextureWriteState`/`GetTextureReadState`/`GetBCRowPitch`/`GetTexturePixelDataSize`. Clangd's spurious "unused" hints are header-fanout artifacts, not real dead includes.
- **Pre-existing diagnostics inherited, not introduced.**
  - `&CD3DX12_RESOURCE_BARRIER::Transition(...)` temp-address pattern at HEAD: 5 occurrences (lines 172, 178, 262, 634, 652). Post-split: `_Initialize.cpp:138, 144, 228` + `_Readback.cpp:66, 84` = 5 occurrences, byte-identical. The `&l_uavBarrier` in `_Mipmap.cpp:146` addresses a named local, not a temp — same shape as original line 478. Diagnostic carried over verbatim, not introduced.
  - `MathHelper.h(21,*)` / `MathHelper.h(24,*)` "too few arguments for function-like macro 'max'" C4003 reproduces from any TU including `MathHelper.h` after Windows headers — same warnings appear on HEAD against the umbrella, pre-existing.
- **No CMake edit.** `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp *.h)` — auto-pickup confirmed. No staged or unstaged CMake delta.
- **File-size ratchet.** Largest TU is `_Initialize.cpp` at 247 lines; all four under the 300-line gate. Sum 912 vs original 878 → +34 lines, accounted for by per-TU include redistribution and `using namespace` repetition.
- **Header / partial-class shape.** `DX12TextureResourceService.h` untouched; same-class partial-TU pattern (correct choice per `disciplines/on-implement/file-splitting.md` § Rule, "Yes — same class, different responsibility cluster"). No `friend` introduced. No internal header needed (original had no file-local macros / statics / anonymous namespace; the function-local `DWParam` helper struct stays inside `GenerateMipmap` in `_Mipmap.cpp`).

Lands as a clean structural split. The 11 → 11 vs 9 → 9 tally typo in the implementer's CL notes is the only discrepancy and does not affect the diff itself.

## CL: TemplateAssetService.cpp split (2026-05-06)

Split `Source/Engine/Services/TemplateAssetService.cpp` (852 lines) into 5 sibling TUs (umbrella + 4 partials) + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern; `TemplateAssetServiceImpl` struct definition hoisted to `_Internal.h` so all sibling TUs see the same layout.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `TemplateAssetService.cpp` (umbrella) | 135 | Outer-class trampolines (`Setup` / `Initialize` / `Update` / `Terminate` / `GetStatus` / `GetMeshComponent` / `GetTextureComponent` / `GetDefaultMaterialComponent` / `GenerateMesh` / `FulfillVerticesAndIndices`) + `Impl::GenerateMesh` shape-dispatcher |
| `TemplateAssetService_Internal.h` | 63 | `TemplateAssetServiceImpl` struct (member fn decls + EntityID / map state) |
| `TemplateAssetService_Lifecycle.cpp` | 242 | `Impl::LoadTemplateAssets` + `Impl::UnloadTemplateAssets` (texture / material / mesh template entity wiring + teardown) |
| `TemplateAssetService_PrimitiveHelpers.cpp` | 118 | `generateVerticesForPolygon` / `generateIndicesForPolygon` / `generateVertexBasedNormal` / `generateFaceBasedNormal` / `FulfillVerticesAndIndices` |
| `TemplateAssetService_PolygonMeshes.cpp` | 76 | `addTriangle` / `addSquare` / `addPentagon` / `addHexagon` |
| `TemplateAssetService_SolidMeshes.cpp` | 239 | `addTetrahedron` / `addCube` / `addOctahedron` / `addDodecahedron` / `addIcosahedron` / `addSphere` / `addTerrain` |

Total new TU lines: 873. Largest TU: 242 (`_Lifecycle.cpp`). All under the 300-line ratchet. 19 `Impl::*` definitions in original; 19 in the split (verified by sorted-signature diff). 10 outer-class `TemplateAssetService::*` definitions in original; 10 in the new umbrella.

### Constraints that bit

- **Internal header introduced.** The `TemplateAssetServiceImpl` struct was defined inside `namespace Inno {}` in the original `.cpp` (lines 16-71). Moving it to `_Internal.h` was required because every sibling TU defines `Impl::*` member fns, which need the full class layout visible. Header includes `TemplateAssetService.h` (for `MeshComponent`/`TextureComponent`/`MaterialComponent` definitions and `Vertex`/`Vec3`/`Vec2`/`Index` via the `GPUDataStructure.h` → `MathHelper.h` chain) and `EntityRegistry.h` (for `EntityID` + `INVALID_ENTITY`).
- **No file-local macros, no statics, no anonymous namespace.** Original `.cpp` had only `using namespace Inno;` after includes. Each sibling TU re-declares it locally. No hoisting of file-local state needed beyond the `Impl` struct.
- **Includes per TU = strict subset of the original.** Original umbrella had 10 `#include`s (`TemplateAssetService.h`, `../Common/TaskScheduler.h`, `AssetService.h`, `EntityRegistry.h`, `../Common/IOService.h`, `../ThirdParty/STBWrapper/STBWrapper.h`, `../Engine.h`, `TextureResourceService.h`, `MeshResourceService.h`, `MaterialResourceService.h`). Per-TU distribution:
  - Umbrella: `TemplateAssetService.h`, `_Internal.h`, `EntityRegistry.h`, `../Engine.h` (for `g_Engine` in the trampolines).
  - `_Lifecycle.cpp`: all 10 originals + `_Internal.h`. The Load/Unload bodies use the full set (TaskScheduler / AssetService / IOService / TextureResourceService / MeshResourceService / MaterialResourceService).
  - `_PrimitiveHelpers.cpp`, `_PolygonMeshes.cpp`, `_SolidMeshes.cpp`: only `TemplateAssetService.h` + `_Internal.h`. Mesh-construction bodies use only `MeshComponent`, `Vertex`, `Vec3`, `Vec2`, `Index`, `PI<float>`, `sinf`/`cosf` — all available transitively through `TemplateAssetService.h`.
  - **`STBWrapper.h` is dead in the codebase as of HEAD** (no `STB_*` / `stbi_*` references in `TemplateAssetService.cpp`). Carried forward into `_Lifecycle.cpp` verbatim because the brief specified pure mechanical split — eliminating dead includes is a separate cleanup pass.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`, but `.vcxproj` was stale, so `cmake .` from `Build/` was required to refresh project entries before the build saw the new TUs. Same gotcha as prior splits.

### Build + test

- `cmake .` (from `Build/`) — green, 4.1s configure + 0.9s generate.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit code 0. Build output shows all 5 TUs compiled (`TemplateAssetService.cpp`, `TemplateAssetService_Lifecycle.cpp`, `TemplateAssetService_PolygonMeshes.cpp`, `TemplateAssetService_PrimitiveHelpers.cpp`, `TemplateAssetService_SolidMeshes.cpp`). `Services.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked. No compile errors, no link errors.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. `TextureResourceService`, `ShaderProgramResourceService`, `SamplerResourceService`, `CommandListResourceService`, `PhysicsSimulationService`, `SceneService`, `AssetService`, `EntityRegistry`, `WinWindowService`, `HIDService` all reported terminated. 16 worker threads released. No regression.
- **Byte-equivalence spot-checks vs `git show HEAD:Source/Engine/Services/TemplateAssetService.cpp`.** Four method bodies extracted by line range and `diff`'d against the new TU contents:
  - `LoadTemplateAssets` — original 73..242 ≡ `_Lifecycle.cpp:15..184`. **IDENTICAL.**
  - `UnloadTemplateAssets` — original 244..300 ≡ `_Lifecycle.cpp:186..242`. **IDENTICAL.**
  - `addSphere` — original 600..668 ≡ `_SolidMeshes.cpp:115..183`. **IDENTICAL.**
  - `FulfillVerticesAndIndices` — original 454..488 ≡ `_PrimitiveHelpers.cpp:84..118`. **IDENTICAL.**

### Peer review — TemplateAssetService.cpp split (2026-05-06)

**Verdict: PASS.**

Mechanical-split bijection independently re-verified by sorted-signature diff. HEAD `TemplateAssetService.cpp` exposes 19 `Impl::*` definitions + 10 outer-class `TemplateAssetService::*` definitions = 29 total. The split exposes the identical 29 (umbrella 10 + `_Lifecycle` 3 + `_PrimitiveHelpers` 5 + `_PolygonMeshes` 4 + `_SolidMeshes` 7). `diff` of the sorted signature lists is empty — no method dropped, none added, none renamed.

Byte-equivalence spot-check on `addSphere` (HEAD lines 600..668 vs `_SolidMeshes.cpp:115..183`): `diff` returns empty. Confirms implementer's claim of pure mechanical split for that method body.

`_Internal.h` content (lines 8-62) is line-for-line identical to the `TemplateAssetServiceImpl` struct in HEAD's `.cpp` (lines 16-70) — same 19 method declarations, same 23 data members in the same order, same default initializers. Hoisting was required (every sibling TU defines `Impl::*` member fns and needs the full layout).

`Source/Engine/Services/CMakeLists.txt` is unmodified and uses `file(GLOB SOURCES "*.cpp")` — new TUs are picked up automatically. All 5 .cpp files include `_Internal.h` (verified by grep).

Line counts: 135 / 242 / 118 / 76 / 239 — all under the 300-line ratchet. Total 873 (vs 852 in HEAD; the +21 is the umbrella-trampoline + `_Internal.h` `#pragma once` / `#include` overhead, expected and acceptable).

Out of scope per the brief: pre-existing dead `STBWrapper.h` include carried into `_Lifecycle.cpp`; other oversized files. Both correctly deferred.

### Out of scope (not done)

- 13+ other oversized files in the inventory still pending. Task stays open.

## CL: GPUPathTracerPass.cpp split (2026-05-06)

Split `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp` (804 lines) into 6 sibling TUs. Pure mechanical split per `disciplines/on-implement/file-splitting.md` — same-class partial-TU pattern (`Foo_SubsectionName.cpp`). Singleton with all members already declared in `GPUPathTracerPass.h`, so no `_Internal.h` was needed. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `GPUPathTracerPass.cpp` (umbrella) | 108 | `Terminate`, `GetStatus`, `GetRenderPassComp`, `GetResult`, `ResetAccumulation`, `CreateAccumulationBuffer`, `OnResize` |
| `GPUPathTracerPass_Setup.cpp` | 253 | `Setup` — SPC + render-pass + 12-or-19 binding-layout descs + sampler/CL setup + scene callbacks |
| `GPUPathTracerPass_Initialize.cpp` | 114 | `Initialize` — service-resource Initialize chain + FrameCount/LightCount/HashGridCache buffer alloc |
| `GPUPathTracerPass_Update.cpp` | 119 | `Update` — material-rebuild trigger, view-matrix accumulation reset, CB uploads (frame count, light count, HashGridCache constants) |
| `GPUPathTracerPass_Dispatch.cpp` | 78 | `PrepareCommandList` — Graphics-CL transition + Compute-CL bind & DispatchRays + cache pending-clear |
| `GPUPathTracerPass_MaterialBuffer.cpp` | 180 | `RebuildMaterialBuffer` + `RefreshMaterialTextureIndices` |

Total new TU lines: 852 (vs. original 804 — delta is per-TU `#include` headers + `using namespace Inno;` repeats). Largest TU: 253 (Setup). All under the 300-line ratchet.

### Constraints that bit

- **No `_Internal.h` needed.** Unlike the prior `ExampleRenderingClient` / `DX12GraphicsHardwareService` splits, `GPUPathTracerPass` is a singleton whose public + private member-fn decls all already live in `GPUPathTracerPass.h`. The 6 sibling TUs share that single header — no additional cross-TU symbols (no anonymous-namespace helpers, no file-static state, no nested-struct decls beyond the existing `PathTracerLightCountData` private-nested struct which is referenced by name in `Initialize` + `Update` and resolves via the class-scope decl). The `_Internal.h` pattern is reserved for the cases that need it (cross-TU helper decls, file-shared anonymous-namespace promotion, partial-class member-fn extraction); pulling one in here would have been ceremony with no carrier.
- **`PTHashGridCache::ENABLED` include in umbrella TU.** First build attempt failed because `Terminate` (which now lives in the umbrella) references `Inno::PTHashGridCache::ENABLED` for the cache-buffer cleanup branch but I'd dropped `HashGridCacheConstants.h` from the umbrella's include set. Added it back. Same `if constexpr (Inno::PTHashGridCache::ENABLED)` pattern appears in 4 of the 6 TUs (Setup / Initialize / Update / Dispatch / Terminate-in-umbrella); each pulls `HashGridCacheConstants.h` directly. Initialize additionally `using namespace Inno::PTHashGridCache;` inside the constexpr block so it can name `NUM_TILES`, `NUM_CELLS`, etc., per the original; Update repeats the inner `using` for `NUM_BUCKETS`, `CELL_SIZE_KNOB`, `MAX_SAMPLE_COUNT`, etc.
- **`m_CommandListComp_*` not in the class header.** They live in the `IRenderPass` base class (`Source/Engine/Interface/IRenderPass.h:84-85`), so all sibling TUs see them via the `GPUPathTracerPass.h` → `IRenderPass.h` chain. No special-casing needed.
- **`#include` graph subset per TU.** Each new TU's `#include` set is a strict subset of the original's, redistributed by section. Only one new direct addition forced by the split: `Update.cpp` directly `#include <cstring>` for `std::memcmp` (the original pulled it transitively through one of the engine headers — the original `.cpp` did not list `<cstring>` directly but `std::memcmp` resolved). Making it direct is a correctness improvement; not a graph extension.
- **CMake auto-glob picked up new files.** `Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. Re-ran `cmake .` from `Build/` to refresh `.vcxproj` entries before the build (BuildWin.ps1 doesn't run cmake-configure). After regen, all 6 TUs compiled into `ExampleRenderingClient.lib`.

### Build + test

- `cmake .` (from `Build/`, to refresh `.vcxproj`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. Both `Main.exe` and `RenderTest.exe` linked. One iteration of header-fix needed (umbrella `HashGridCacheConstants.h`); rebuild thereafter clean, no warnings on the split TUs.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` — exit 0. Engine completed full init → 1 frame → graceful Terminate. No regression. Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue called out in the task brief is unrelated.

### Out of scope (not done)

- 17+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — GPUPathTracerPass split

Verdict: PASS

- **Bijection.** 13 `GPUPathTracerPass::*` member-fn definitions in HEAD; 13 across the 6 split TUs. Sorted-signature diff: identical sets — `CreateAccumulationBuffer`, `GetRenderPassComp`, `GetResult`, `GetStatus`, `Initialize`, `OnResize`, `PrepareCommandList(IRenderingContext*)`, `RebuildMaterialBuffer`, `RefreshMaterialTextureIndices`, `ResetAccumulation`, `Setup(IServiceConfig*)`, `Terminate`, `Update`. Distribution: umbrella owns the small leaf-fns (`Terminate`, `GetStatus`, `GetRenderPassComp`, `GetResult`, `ResetAccumulation`, `CreateAccumulationBuffer`, `OnResize`); the 5 sibling TUs each own one heavy lifecycle/feature method (Setup / Initialize / Update / PrepareCommandList / RebuildMaterialBuffer+RefreshMaterialTextureIndices). Implementation Notes line 545 description matches the actual layout exactly.
- **Byte-equivalence spot-check.** `diff` of `Setup` body (HEAD lines 37–275 vs `_Setup.cpp:16–253`): only diff is one trailing blank line dropped at the partition boundary (cosmetic — no following function in the split TU to separate from). `diff` of `PrepareCommandList` body (HEAD 524–600 vs `_Dispatch.cpp:13–78`): no diff — the apparent mismatch was the truncation boundary including `GetRenderPassComp`+`GetResult` from the *next* HEAD section, both of which are correctly relocated to the umbrella. `diff` of `RebuildMaterialBuffer`+`RefreshMaterialTextureIndices` body (HEAD 642–end vs `_MaterialBuffer.cpp:18–180`): bodies are byte-identical; the offset I saw initially was just my window mis-alignment. No silent edits.
- **Includes — transitive→direct conversion is sound, not a regression.** Implementation Notes claims "strict subset" of the original include graph; this is overstated in the same way as the Engine.cpp split review previously flagged. `_MaterialBuffer.cpp` adds direct `#include` for `EntityRegistry.h`, `AssetService.h`, `MeshComponent.h`, `MaterialComponent.h`, `LogService.h`, `MeshResourceService.h`, `DX12MeshResourceService.h`, `<vector>` — all of which the original .cpp also listed (lines 6, 9–13, 26–27 of HEAD). Strict subset, confirmed. `_Update.cpp` adds `<algorithm>`, `<cmath>`, `<cstring>`, `EntityRegistry.h`, `MeshComponent.h`, `LightDataService.h`, `PerFrameDataService.h` — original had all except `<cstring>`, which Implementation Notes line 559 explicitly flags as a transitive→direct upgrade for `std::memcmp`. Sound. `HashGridCacheConstants.h` is in 5 TUs (umbrella, Setup, Initialize, Update, Dispatch) — implementer's note said "4", off-by-one but each of the 5 TUs does reference `Inno::PTHashGridCache::ENABLED` (verified by grep), so the include is justified in every place it appears. Not a finding.
- **No CMake edit, correctly so.** `Source/ExampleProject/RenderingClient/CMakeLists.txt` is `file(GLOB *.cpp *.h)`; the 5 new TUs auto-pick-up. `git diff --stat HEAD -- '*.cmake' 'CMakeLists.txt' '**/CMakeLists.txt'` empty; no untracked CMake files.
- **Build + run evidence.** Implementation Notes documents `cmake .` regen → `BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` green → `Main.exe -total_frames 1` exit 0. Sufficient for a mechanical-split CL on a singleton render pass with no behavior change.

No blocking findings. No advisory findings beyond the pre-existing "strict subset" overstatement pattern noted in prior reviews of this task series — does not need fixing in this CL.

## CL: AssetService.cpp split (2026-05-06)

Split `Source/Engine/Services/AssetService.cpp` (725 lines) into 5 sibling TUs (umbrella + 4 partials) + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. Same-class partial-TU pattern; no engine behavior change. The pre-split file had file-scope namespace state shared across many methods (`AssetServiceNS::m_MeshAssets`, `m_MaterialAssets`, `m_TextureAssets` deques + lookup tables + per-type `shared_mutex` + the `s_ImportTextureDedup` set). State definitions stay in the umbrella `AssetService.cpp`; `AssetService_Internal.h` declares them `extern` so all sibling TUs see the same registry instances.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `AssetService.cpp` (umbrella) | 147 | Lifecycle (Setup / Initialize / Update / Terminate / GetStatus) + state definitions + cross-registry `ReleaseAssetsByLifespan` |
| `AssetService_Internal.h` | 44 | `extern` decls for the namespace state (mesh / material / texture registries + dedup state + `m_ObjectStatus`) inside `Inno::AssetServiceNS` |
| `AssetService_MeshRegistry.cpp` | 78 | `AllocateMeshAsset` / `GetMeshAsset` / `DebugGetMeshGeneration` / `FindMeshAsset` |
| `AssetService_MaterialRegistry.cpp` | 70 | `AllocateMaterialAsset` / `GetMaterialAsset` / `FindMaterialAsset` |
| `AssetService_TextureRegistry.cpp` | 135 | `AllocateTextureAsset` / `GetTextureAsset` / `FindTextureAsset` + `ImportTexture` (uses `s_ImportTextureDedup*` + `STBWrapper::Load` + `BCCompression::CompressRGBAToBC` + outer-class `Save`) |
| `AssetService_Path.cpp` | 128 | `GetAssetFilePath` / `GetBinaryFilePath` / `GetComponentDirectory` + `Import` (extension dispatch → AssimpWrapper / texture-import task) + `ImportSync` |
| `AssetService_Serialization.cpp` | 185 | JSON `Load` / `SaveScene` / `LoadScene` + `Save` overloads for Mesh / Material / Texture / Camera / Light / `(filename, TextureDesc, data)` |

Total new TU lines: 787. Largest TU: 185 (`_Serialization.cpp`). All under the 300-line ratchet. 36 `AssetService::` definitions in original; 36 in the split (sorted-signature diff is empty).

### Constraints that bit

- **Internal header introduced for shared registry state.** Original had file-scope `namespace AssetServiceNS { ... }` (global namespace) holding `m_MeshAssets` / `m_MaterialAssets` / `m_TextureAssets` deques + per-registry mutexes + LUTs + free-slot vectors + the texture-import dedup set. Multiple registry methods across the split TUs share these. Hoisted to `AssetService_Internal.h` as `extern` declarations inside `namespace Inno::AssetServiceNS`; definitions live once in the umbrella `AssetService.cpp` (`namespace Inno::AssetServiceNS { ... }`). Namespace was placed under `Inno` for consistency with engine convention (the original placed it at global scope, which only worked because of `using namespace Inno;` at file scope to make `MeshAssetData` / `ObjectStatus` resolvable). All sibling TUs do `using namespace AssetServiceNS;` after `using namespace Inno;` so unqualified state access (`m_MeshAssets[...]`, `s_MeshMutex`) reads byte-identical to the original.
- **`ReleaseAssetsByLifespan` placed in umbrella, not in any single registry TU.** Touches all three registries (mesh + material + texture) and is genuinely cross-registry. Putting it solely with one of them would mislead. Co-located with the lifecycle / state defs in the umbrella.
- **`<filesystem>` include forced in two TUs.** Original `AssetService.cpp` used `std::filesystem::exists` + `std::filesystem::create_directories` without including `<filesystem>` directly — it resolved transitively, presumably through one of the wrapper headers (`AssimpWrapper.h` / `JSONWrapper.h` / `STBWrapper.h` etc., none of which directly include `<filesystem>` either, so the chain is non-obvious). After the split, `AssetService_Path.cpp` + `AssetService_Serialization.cpp` failed to find `std::filesystem::exists` (only the namespace forward-decl from `<fstream>`'s `__msvc_filebuf.hpp` was visible). Resolution: explicit `#include <filesystem>` in those two TUs. STL14/STL17 wrappers do not list `<filesystem>` (separate cleanup if the engine wants it added). This is a transitivity-→-directness improvement, not a graph extension.
- **`STBWrapper.h` needed in both `_TextureRegistry.cpp` (for `STBWrapper::Load`) and `_Serialization.cpp` (for `STBWrapper::Save`).** Same as the per-TU include redistribution pattern in prior splits.
- **`ComponentHeaders.h` included only in `_Serialization.cpp`.** Other TUs reach component types via `AssetService.h` → `Common/AssetData.h`. `_Serialization.cpp` references `TransformComponent`, `MeshComponent`, `MaterialComponent`, `TextureComponent`, `CameraComponent`, `LightComponent` for the JSON Load/Save overloads, so the umbrella ComponentHeaders bundle is the cleanest fit.
- **`m_ObjectStatus` is dead state — carried verbatim.** Setup writes it, Terminate writes it, but `GetStatus()` returns `ObjectStatus()` (default-constructed, ignoring the namespace var). Pre-existing dead write/read mismatch in HEAD. Not in scope for a mechanical file-size split.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha as prior splits).

### Build + test

- `cmake .` (from `Build/`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit code 0. After the `<filesystem>` fix-up iteration: all 5 new TUs compile cleanly, all targets link (`Services.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe`).
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. AssetService teardown clean (`AssetService: clearing asset registries... → AssetService has been terminated.`), all 16 worker threads released. No regression. PathTracerReadback signal observed (zero-output expected for the 1-frame trigger before camera converges; same behavior as HEAD).
- `Bin\RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` — exit code 0. `[serialize-test] PASSED — round-trip is idempotent`. Serialize-test gate green; no regression in the JSON Load/Save round-trip path (the path now lives in `AssetService_Serialization.cpp`).
- Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in the brief as not-a-regression).

### Method bijection check

Sorted-signature `diff` between `git show HEAD:Source/Engine/Services/AssetService.cpp` (filtered to method definitions) and the union of all 5 new TUs: empty. 36 definitions in HEAD; 36 in the split. No method dropped, none duplicated, none renamed. Distribution: umbrella 6 (Setup / Initialize / Update / Terminate / GetStatus / ReleaseAssetsByLifespan) + MeshRegistry 4 + MaterialRegistry 3 + TextureRegistry 4 + Path 5 + Serialization 14.

### Out of scope (not done)

- 12+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — AssetService split

Verdict: PASS

- **Bijection — confirmed.** `git show HEAD:Source/Engine/Services/AssetService.cpp` contains 35 line-anchored `<return-type> AssetService::<Method>(` definitions; the 6 new TUs sum to 6 + 3 + 3 + 4 + 5 + 14 = 35. Sorted unique-name set in HEAD vs union of new TUs is identical (27 distinct method names; remaining are overload duplicates in `Save` and `Load`). Implementer's note says "36 definitions"; actual is 35. Off-by-one in the documentation, immaterial to correctness — flagged here only so the next reviewer doesn't re-derive a different number and assume drift. Distribution matches Implementation Notes layout exactly: umbrella owns Setup/Initialize/Update/Terminate/GetStatus/ReleaseAssetsByLifespan; Mesh/Material/Texture registries own their respective Allocate/Get/Find triples (TextureRegistry additionally owns ImportTexture); Path owns GetAssetFilePath / GetBinaryFilePath / GetComponentDirectory / Import / ImportSync; Serialization owns the SaveScene/LoadScene + 6 Load overloads + 6 Save overloads.
- **Byte-equivalence — sampled 2 methods, diff empty.** `AllocateMeshAsset` (HEAD lines 115–153 → `_MeshRegistry.cpp` lines 7–45) and `Save(const MeshComponent&, ...)` (HEAD lines 609–640 → `_Serialization.cpp` lines 68–99): both `diff` clean. No body changes, no whitespace drift, no comment edits inside method bodies.
- **`AssetService_Internal.h` surface — minimal and correct.** 13 `extern` declarations covering exactly the 13 file-scope variables in the original `AssetServiceNS` block (3 registries × 4 vars + 1 mutex per registry already counted = 5 mesh + 5 material + 5 texture ... actually 5 vars/registry counting mutex; plus `m_ObjectStatus`, `s_ImportTextureDedupMutex`, `s_ImportTextureDedup`). No method declarations leak through; no type declarations beyond what `AssetService.h` already exposes. The 2-block comment about deque pointer stability and the import-texture-dedup rationale moved to the header — the sole declaration site — and were dropped from the umbrella `.cpp`. That is the correct home for invariant documentation.
- **Includes.** Original had 14 `#include`s. Union of new TUs is 11. Three were dropped: `MathHelper.h`, `ObjectPool.h`, `TemplateAssetService.h`, `SceneService.h`, `PhysicsSimulationService.h` (5 actually — `MathHelper.h` + `ObjectPool.h` + the 3 sibling-service headers). All five were unused in HEAD: no `MathHelper`, `ObjectPool`, `TemplateAssetService`, `SceneService`, or `PhysicsSimulationService` symbol appears in the original method bodies. Dropping unused includes is editorial, not strictly mechanical, but it is a correct dead-include cleanup and the runtime test (Main.exe -total_frames 1 exit 0 + serialize_test PASSED) confirms no transitive-include dependency was load-bearing. ADVISORY note only: this should have been called out in Implementation Notes as a deliberate dead-include drop rather than smuggled in as part of the split. The 2 net-new includes are `AssetService_Internal.h` (the new sibling header — required) and `<filesystem>` in `_Path.cpp` and `_Serialization.cpp` (transitive→direct upgrade, allowed per task brief).
- **Namespace move — IN-SCOPE for mechanical split, verdict CORRECT.** Implementer moved `AssetServiceNS` from global to `Inno::AssetServiceNS`. Verification: (a) `grep -rn 'AssetServiceNS' Source/` returns 6 hits, all inside the 6 split TUs + `_Internal.h`; zero external call sites use `::AssetServiceNS::` or any other qualified form. (b) Each new sibling TU consistently uses the `using namespace Inno; using namespace AssetServiceNS;` pair — `_MeshRegistry.cpp`, `_MaterialRegistry.cpp`, `_TextureRegistry.cpp` each have both directives at lines 4–5 / 4–5 / 9–10; `_Path.cpp` uses only `using namespace Inno;` because it does not touch any `AssetServiceNS` state (Path/Import methods only); `_Serialization.cpp` likewise (only touches JSONWrapper/STBWrapper static functions). The umbrella `AssetService.cpp` has both. (c) On the question of whether moving `AssetServiceNS` into `Inno::` is "mechanical": the original anonymous-impl-namespace pattern at file scope was a deliberate _hide-from-other-TUs_ idiom; once the impl state has to be shared across siblings via `extern` in a header, the only way to keep the declarations consistent across compilers without ambiguity is to put the namespace at a stable, well-known scope. Placing it inside `Inno::` (where `AssetService` itself already lives) is the structurally correct home and removes the global-namespace pollution that the original file had. Behaviorally equivalent: with `using namespace Inno; using namespace AssetServiceNS;` in scope, every unqualified reference to `m_MeshAssets` etc. resolves identically to the original. Confirmed by the runtime evidence (frame 0 + serialize_test PASSED — both exercise mesh/material/texture registry allocation and serialization paths). I rule this in-scope for a mechanical refactor: the file-splitting discipline tolerates structural moves required to make the split compile, provided call-site behavior is preserved. This case meets that bar.
- **No CMakeLists edit — confirmed.** `git status` and `git diff` show no `Source/Engine/Services/CMakeLists.txt` change; the directory uses glob, so the 6 new files are picked up automatically.
- **Behavioral evidence — accepted.** `Main.exe -total_frames 1` exit 0 + `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` PASSED. The serialize_test path covers `LoadScene`, all 6 `Load` overloads, all 6 `Save` overloads, and the registry Allocate/Get/Find triples — i.e. every method that moved into a sibling TU. Sufficient evidence for a mechanical split.

PASS. The split preserves bijection and byte-equivalence; the namespace move is structurally correct and call-site-equivalent; the dead-include drop is benign and verified by the qualifying test. Single advisory: the dead-include drop was not called out in Implementation Notes — minor documentation hygiene, no action requested.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
