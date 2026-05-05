---
id: TASK-219
title: 'Rolling cleanup: downsize oversized source files past the 300-line ratchet'
status: To Do
assignee: []
created_date: '2026-05-05 16:48'
updated_date: '2026-05-05 19:44'
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
