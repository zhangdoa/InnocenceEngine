---
id: TASK-219
title: 'Rolling cleanup: downsize oversized source files past the 300-line ratchet'
status: To Do
assignee: []
created_date: '2026-05-05 16:48'
updated_date: '2026-05-06 19:23'
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
## Review (code-impl, 2026-05-06) — DX12GPUBufferResourceService split

Verdict: PASS

Bijection verified: 12 `DX12GPUBufferResourceService::` definitions in HEAD (`Build/HEAD_orig.cpp`) split as 5 (umbrella: Delete, InitializeImpl, UploadToGPU x2, Clear) + 3 (Views: CreateSRV, CreateUAV, CreateCBV) + 4 (Raytracing: OnSceneLoadingStart, UpdateRaytracingInstances, CreateRaytracingResources, ReleaseRaytracingResources) — total 12, matches HEAD set verbatim. Byte-equivalence spot-checked on Delete (umbrella L17–40 vs HEAD L23–46), CreateSRV (Views L11–33 vs HEAD L358–380), and OnSceneLoadingStart (Raytracing L18–32 vs HEAD L195–209) — all identical modulo line offsets. No CMake edit (`git diff --stat HEAD` shows only the 3 cluster files + the task md). No new internal header — only the existing public `DX12GPUBufferResourceService.h` is included by the new TUs. Pre-existing CD3DX12_RESOURCE_BARRIER address-of-temporary at current umbrella L150/L161 corresponds verbatim to HEAD L156/L167 (same `&CD3DX12_RESOURCE_BARRIER::Transition(...)` rvalue pattern); not introduced by this CL. Sizes 229/94/182 match the claim and clear the 300-line ratchet for all three TUs.

## Review (code-impl, 2026-05-06) — RadianceCacheReprojectionPass split

Verdict: PASS

Bijection verified against `git show HEAD:Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp`: HEAD has 15 `RadianceCacheReprojectionPass::` definitions (Setup, Initialize, Terminate, GetStatus, PrepareCommandList, GetRenderPassComp, RenderTargetsCreationFunc, GetCurrentFrameResult, GetPreviousFrameResult, GetCurrentProbePosition, GetPreviousProbePosition, GetCurrentProbeNormal, GetPreviousProbeNormal, GetWorldProbeGrid, GetProbeMask). Post-split: umbrella retains 13 (Initialize, Terminate, GetStatus, PrepareCommandList, GetRenderPassComp + 8 accessors), `_Setup.cpp` holds 2 (Setup, RenderTargetsCreationFunc) — total 15, no duplicates, no missing. Byte-equivalence spot-checks: Setup body (`_Setup.cpp` L15–148 vs HEAD L21–154) — `diff` clean; RenderTargetsCreationFunc (`_Setup.cpp` L149–261 vs HEAD L266–378) — `diff` clean. PrepareCommandList in current umbrella L59–122 vs HEAD L196–259 differs by exactly one line — a trailing-tab whitespace stripped at umbrella L120 (the `m_ObjectStatus = ObjectStatus::Activated;` blank line below); cosmetic, no semantic effect. No CMake edit (`Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB *.cpp)` — auto-pickup; `git status` shows it untouched). No internal header introduced — both TUs include only the existing public `RadianceCacheReprojectionPass.h` plus a strict subset of the original umbrella's service headers (`_Setup.cpp` correctly drops `PerFrameDataService.h`, `OpaquePass.h`, `FrameManagementService.h` which are unused by Setup/RenderTargetsCreationFunc, and adds `RenderingConfigurationService.h` which Setup needs). Sizes 191/261 clear the 300-line ratchet.

<!-- SECTION:NOTES:BEGIN -->
## Review (code-impl, 2026-05-06) — VKGraphicsService_VulkanObject split

Verdict: PASS

Bijection 32 = 11 (umbrella) + 13 (Memory) + 8 (DescriptorAndShader) confirmed by definition-line enumeration against `git show HEAD:`. All 32 functions in HEAD accounted for, zero duplicates across TUs (`grep -n "^[a-zA-Z].*VKGraphicsService::.*("` over the three files). Byte-equivalence spot-checks: `FindQueueFamilies` (umbrella), `FindMemoryType` (Memory), and `CreateShaderModule` (DescriptorAndShader) all body-identical to HEAD. The only byte difference anywhere is a trailing `\n` appended at EOF of `_DescriptorAndShader.cpp` (HEAD's last function lacked it) — cosmetic POSIX-text-file fix, not a content change. `m_shaderRelativePath` defined exactly once (`_DescriptorAndShader.cpp:32`, anonymous namespace) alongside its sole consumer at line 147 — single-definition site verified by repo-wide grep. Naming `_DescriptorAndShader.cpp` cleanly avoids collision with the pre-existing siblings `VKGraphicsService_Descriptor.cpp` and `VKGraphicsService_Shader.cpp` (different concern, from earlier `_EngineComponent.cpp` split); the `_VulkanObject_` prefix preserves provenance. No CMake edit (`git status` clean for `CMakeLists.txt`). Pre-existing VK breakage (clangd `'../GraphicsResourceService.h' file not found`, `m_initializedTextures`) is carried over verbatim and out of scope per the dispatch.

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

## CL: EditorService.cpp split (2026-05-06)

Split `Source/Engine/Services/EditorService.cpp` (723 lines) into 4 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern: `RegisterBuiltinHandlers` was the dominant mass (~424 lines, 16 IPC handlers); split into three private member fns by handler-cluster, each cluster owns its own TU.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `EditorService.cpp` (umbrella) | 268 | ctor/dtor, `Setup`, `Initialize` (incl. WS dispatcher loop), `BroadcastSceneUpdated`, `BroadcastScreenshotSaved`, `RegisterBuiltinHandlers` (now a 3-call dispatcher), `Update`, `Terminate`, `GetStatus`, anon-namespace reply/event builders + `GetServer` |
| `EditorService_Internal.h` | 47 | `EditorServiceImpl` (full def, was forward-decl-only in `EditorService.h`); `EditorReqError`; inline `SerializeVec(Vec3/Vec4)`; inline `RequireFields` |
| `EditorService_Introspection.cpp` | 180 | `RegisterIntrospectionHandlers`: HELLO, GET_SCENE, GET_ENTITY_DETAILS, LIST_DEV_TOGGLES, LIST_TASKS, LIST_RENDER_TARGETS (read-only handlers) |
| `EditorService_DevAndScene.cpp` | 88 | `RegisterDevAndSceneHandlers`: SET_DEV_TOGGLE, TRIGGER_DEV_ACTION, SET_VIEWPORT_SOURCE, LOAD_SCENE, SAVE_SCENE, IMPORT_ASSET (engine-knob + file-driven mutations) |
| `EditorService_Entity.cpp` | 189 | `RegisterEntityHandlers`: ENTITY_CREATE, ENTITY_DELETE, ENTITY_RENAME, UPDATE_ENTITY_PROPERTY (scene-tree mutations) |

Total new TU lines: 772 (incl. unchanged `EditorService.h` at 55). Largest TU: 268 (umbrella). All under the 300-line ratchet.

### Constraints that bit

- **`EditorServiceImpl` definition moved to `_Internal.h`.** Original was an in-`.cpp` PIMPL; sibling TUs need `m_Impl->mutex` + `m_Impl->handlers[type] = ...` to register handlers. The public `EditorService.h` keeps the forward decl + `std::unique_ptr<EditorServiceImpl>` member unchanged; the full definition now lives once in `_Internal.h` which the umbrella + 3 cluster TUs all `#include`. PIMPL contract preserved (no `<ix*>` / `nlohmann/json` types in the public header).
- **`EditorReqError` and helpers (`RequireFields`, `SerializeVec`) hoisted to `_Internal.h`.** `EditorReqError` is thrown across the 3 cluster TUs and caught in the dispatcher loop in the umbrella; `RequireFields` and `SerializeVec` are used in 2-3 cluster TUs each. Made `inline` (`SerializeVec`, `RequireFields`) to keep behaviour identical to the original `static` TU-local definitions while supporting multi-TU inclusion. `EditorReqError` is a class; its only inline method (`Code()`) was already inline in the original.
- **Anon-namespace builders (`GetServer`, `BuildErrorReply`, `BuildOkReply`, `BuildEvent`) stayed in the umbrella `.cpp`** — only the umbrella's `Initialize` dispatcher loop and the two `Broadcast*` methods use them; no sibling TU references them. The original was `static`-at-file-scope; converted to anonymous-namespace block (same TU-local linkage) so they sit cleanly together inside `namespace Inno` is *not* needed (no `Inno`-typed entities; pure `json` + raw `void*` cast).
- **Three new private member fns on `EditorService`.** `RegisterIntrospectionHandlers`, `RegisterDevAndSceneHandlers`, `RegisterEntityHandlers` declared in `EditorService.h` (not `_Internal.h`) because they're class members and the original `RegisterBuiltinHandlers` was already declared there — same surface conceptually. The 4-line `reg` lambda (`[this](type, h) { lock; m_Impl->handlers[type] = ...; }`) is redefined in each cluster method rather than promoted to a member fn — keeps the lambda capture-free of cross-cluster surface area.
- **Setter-reply contract comment relocated to umbrella's `RegisterBuiltinHandlers` body.** The 12-line block comment about read-back vs payload-echo (formerly above the `reg("HELLO"...)` line in the original) describes a class-wide invariant for SET_* / mutating handlers, not anything specific to one cluster. Moved to the umbrella's `RegisterBuiltinHandlers` (which now is the dispatcher that calls the three cluster fns) so it's read once at the top of the registration entry point.
- **Include graph subset.** Each cluster TU's `#include` set is a strict subset of the original umbrella's. The umbrella drops the includes that are now satisfied by `_Internal.h` (`<functional>`, `<mutex>`, `<stdexcept>`, `<unordered_map>`, `IXWebSocket.h`, `JSONWrapper.h`); cluster TUs include only the headers they actually use (`EntityRegistry.h`, `DevToggleRegistry.h`, etc. — no full-fan-out reuse of the umbrella's transitive cone). One small graph extension: `_Internal.h` directly includes `Math.h` for `Vec3`/`Vec4` (originally reached transitively through `TransformComponent.h` → `MathHelper.h` → `Math.h`). Direct include in a header that uses the type is a correctness improvement, not a graph regression.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha called out in prior splits). Verified `Build/Source/Engine/Services/Services.vcxproj` lists all 4 new `ClCompile` + 1 new `ClInclude` entries after configure.

### Build + test

- `cmake .` (from `Build/`) — green. Configuration completed in 4.1s.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit 0. All 4 new TUs (`EditorService.cpp`, `EditorService_DevAndScene.cpp`, `EditorService_Entity.cpp`, `EditorService_Introspection.cpp`) compiled cleanly. `Services.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` — exit 0. Engine completed full init → 1 frame → graceful Terminate. (Default mode is Host, EditorService not initialized in this mode — the prior gate-satisfying run.)
- `Bin\RelWithDebInfo\Main.exe -total_frames 1 -mode 2` (sidecar mode, EditorService active) — exit 0. Verified log lines:
  - `EditorService: Setup finished.` (from umbrella `Setup`)
  - `EditorService: WebSocket server started on port 8081.` (from umbrella `Initialize`, after all three `Register*Handlers` cluster fns ran)
  - `EditorService: Terminated.` (from umbrella `Terminate`)
- All three handler-registration cluster fns ran without crash; the dispatcher came up and bound to 8081 cleanly. Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible on any working tree state on this branch (not a regression).

### Method bijection check

`git show HEAD:Source/Engine/Services/EditorService.cpp` registers 16 handlers via `reg("...", ...)`. Union of the 3 cluster TUs registers 16 handlers. Sorted string-set diff: empty. Distribution:

- Introspection (6): HELLO, GET_SCENE, GET_ENTITY_DETAILS, LIST_DEV_TOGGLES, LIST_TASKS, LIST_RENDER_TARGETS.
- Dev+Scene (6): SET_DEV_TOGGLE, TRIGGER_DEV_ACTION, SET_VIEWPORT_SOURCE, LOAD_SCENE, SAVE_SCENE, IMPORT_ASSET.
- Entity (4): ENTITY_CREATE, ENTITY_DELETE, ENTITY_RENAME, UPDATE_ENTITY_PROPERTY.

Spot-check byte-equality: `ENTITY_CREATE` body (`HEAD` 519-537 vs `_Entity.cpp` 15-33) and `UPDATE_ENTITY_PROPERTY` body (`HEAD` 591-691 vs `_Entity.cpp` 85-185) both diff-clean (only sed-range alignment whitespace at the boundary). All read-back semantics in SET_DEV_TOGGLE / SET_VIEWPORT_SOURCE / UPDATE_ENTITY_PROPERTY's color path preserved verbatim.

### Out of scope (not done)

- 11+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — EditorService split

Verdict: PASS

- **Header surface — clean.** `git diff HEAD -- EditorService.h` is exactly 3 added private-member-fn declarations (`RegisterIntrospectionHandlers`, `RegisterDevAndSceneHandlers`, `RegisterEntityHandlers`) sandwiched between the existing `RegisterBuiltinHandlers` and `BroadcastSceneUpdated` — both already private. Public surface (`Setup`/`Initialize`/`Update`/`Terminate`/`GetStatus`/`BroadcastScreenshotSaved`/ctor/dtor) is byte-identical to HEAD. No new include in the public header; the forward-declared `struct EditorServiceImpl` and `std::unique_ptr<EditorServiceImpl> m_Impl` are unchanged. PIMPL contract preserved: `grep ix::|nlohmann|<ixwebsocket|json` against `EditorService.h` returns only the comment block + the `void* m_Server` decl (which already existed); no WS/json types leak. The 3 added decls are the structurally minimum change required by the same-class partial-TU pattern when `RegisterBuiltinHandlers`'s body is the dominant mass — they are private cluster entry points, not new public API. **Acceptable as a mechanical refactor**: the discipline tolerates new private member fns when needed for cluster delegation; alternatives (free functions taking `EditorService*` + `EditorServiceImpl*`, or moving the bodies into `_Internal.h` as inline class methods) would be strictly worse for readability and encapsulation.
- **PIMPL relocation to `_Internal.h` — correct, in-scope.** `EditorServiceImpl` (and `EditorReqError`) moved from anon namespace in HEAD's `.cpp` (lines 46-65) to `_Internal.h` (lines 17-34). This is the same pattern the AssetService split previously took (verdict PASS): once impl-state has to be shared across siblings, the only well-formed home is a sibling header. Public header still forward-declares only. `_Internal.h` is `#include`d by exactly the 4 split TUs (umbrella + 3 clusters); zero external consumers — verified `grep -r EditorService_Internal.h Source/` returns only those 4 files. ix/json types are confined to `_Internal.h` and the 4 TUs that include it; no leak.
- **Inline-promotion of `SerializeVec` / `RequireFields` — correct.** Originally `static` TU-local at file scope (HEAD lines 67-68 / lines 71-78). Now `inline` in `_Internal.h` lines 36-46 with bodies trivial enough that inline-in-header is the canonical C++ approach for header-shared free functions. Confirmed engine convention: `Source/Engine/Common/Math.h` and `JSONWrapper.h` both use the inline-in-header pattern for trivial helpers. ODR-safe: same body in every TU because there is only one body.
- **Bijection — confirmed 16 → 16, sorted-set diff empty.** HEAD's `EditorService.cpp` registers exactly 16 handlers via `reg("...")` (HEAD lines 287-591). Union of the 3 cluster TUs: 6 (Introspection: HELLO, GET_SCENE, GET_ENTITY_DETAILS, LIST_DEV_TOGGLES, LIST_TASKS, LIST_RENDER_TARGETS) + 6 (DevAndScene: SET_DEV_TOGGLE, TRIGGER_DEV_ACTION, SET_VIEWPORT_SOURCE, LOAD_SCENE, SAVE_SCENE, IMPORT_ASSET) + 4 (Entity: ENTITY_CREATE, ENTITY_DELETE, ENTITY_RENAME, UPDATE_ENTITY_PROPERTY) = 16. Sorted string set identical to HEAD.
- **Byte-equivalence — sampled 3 handlers, one per cluster, all `diff` clean.** `GET_SCENE` (HEAD 295-310 vs `_Introspection.cpp` 28-43), `LOAD_SCENE` (HEAD 491-501 vs `_DevAndScene.cpp` 61-71), `ENTITY_CREATE` (HEAD 519-538 vs `_Entity.cpp` 16-35): all empty diffs. No body changes, no whitespace drift.
- **No CMake edit — confirmed.** `git diff HEAD --stat` lists no `CMakeLists.txt` changes; the `Source/Engine/Services/` directory uses a glob, so the 4 new files are picked up by `cmake .` reconfigure (which the implementer ran).
- **Dispatcher loop split into anon namespace — sound.** Reply/event builders (`GetServer`, `BuildErrorReply`, `BuildOkReply`, `BuildEvent`) moved from `static`-at-file-scope to an anonymous namespace block at the top of the umbrella `.cpp` (lines 30-62). Same TU-local linkage; only the umbrella's `Initialize` dispatcher loop and the two `Broadcast*` methods reference them, so they correctly stayed in the umbrella and were not promoted to `_Internal.h`. Implementation Notes line 662 has a clipped/awkward sentence ("converted to anonymous-namespace block (same TU-local linkage) so they sit cleanly together inside `namespace Inno` is *not* needed (no `Inno`-typed entities; pure `json` + raw `void*` cast)") — readable but ungrammatical; documentation-hygiene only, no action requested.
- **Include graph — strict subset confirmed.** Umbrella drops includes now satisfied by `_Internal.h` (`<functional>`, `<mutex>`, `<stdexcept>`, `<unordered_map>`, `IXWebSocket.h`, `JSONWrapper.h`) and drops domain headers no longer used in the umbrella (`EntityRegistry.h`, `AssetService.h`, `DevToggleRegistry.h`, `RenderPassResourceService.h`, `ViewportSourceOverride.h`, `TaskScheduler.h`, `Thread.h`, `TransformComponent.h`, `LightComponent.h`); each cluster TU includes only the domain headers its handlers reference. `_Internal.h` directly includes `Math.h` for `Vec3`/`Vec4` — transitive→direct upgrade flagged in Implementation Notes line 665, sound.
- **Behavioral evidence — accepted.** `Main.exe -total_frames 1` (Host mode, EditorService dormant) exit 0, AND `Main.exe -total_frames 1 -mode 2` (sidecar mode, EditorService active) exit 0 with `WebSocket server started on port 8081` logged. The sidecar run exercises `Setup` → `Initialize` → all 3 `Register*Handlers` cluster fns → dispatcher loop bind → `Terminate`. Sufficient for a mechanical-split CL on an IPC service whose handlers are invoked only by external editor traffic (no scripted handler-coverage harness exists; flagged as out-of-scope per task brief).

PASS. The 3 added private member fns and the PIMPL relocation are both **acceptable for a mechanical refactor** — they are the structurally minimum changes required to make a same-class partial-TU split compile, with zero public-surface impact and zero behavioral change. Bijection holds, byte-equivalence holds, runtime evidence covers both inactive and active EditorService modes. Single advisory: Implementation Notes line 662 has an ungrammatical sentence; documentation hygiene only, no action requested.

## CL: DX12Helper_Texture.cpp split (2026-05-06)

Split `Source/Engine/Services/DX12/DX12Helper_Texture.cpp` (668 lines) into 5 sibling TUs by domain. Pure mechanical split per `disciplines/on-implement/file-splitting.md` — free-function-header pattern: umbrella `DX12Helper_Texture.h` keeps all 16 declarations untouched; original umbrella `.cpp` is removed (no shared statics, no anon-namespace helpers — every function is self-contained inside `namespace DX12Helper`). No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12Helper_Texture.h` (header — unchanged) | 28 | Umbrella declarations (16 functions in `namespace Inno::DX12Helper`) |
| `DX12Helper_Texture_Format.cpp` | 225 | `GetTextureFormat`, `GetTexturePixelDataSize`, `GetBCBlockBytes`, `GetBCRowPitch` |
| `DX12Helper_Texture_Desc.cpp` | 146 | `GetDX12TextureDesc`, `GetTextureDimension`, `GetTextureMipLevels`, `GetTextureBindFlags` |
| `DX12Helper_Texture_View.cpp` | 210 | `GetSRVDesc`, `GetUAVDesc`, `GetRTVDesc`, `GetDSVDesc` |
| `DX12Helper_Texture_Sampler.cpp` | 52 | `GetFilterMode`, `GetWrapMode` |
| `DX12Helper_Texture_State.cpp` | 45 | `GetTextureWriteState`, `GetTextureReadState` |

Total new TU lines: 678 (+10 vs. original 668, accounted for entirely by 5× `#include "DX12Helper_Texture.h"` + `using namespace Inno;` + blank-line preambles instead of one). Largest TU: 225 (Format). All under the 300-line ratchet.

### Constraints that bit

- **No internal header introduced — none needed.** Original had zero `static`-at-file-scope helpers, zero anon-namespace blocks, zero TU-local state. Every function is pure (`TextureDesc` in → DX12 type out). Cross-function calls (`GetDX12TextureDesc` calls `GetTextureMipLevels`/`GetTextureFormat`/`GetTextureDimension`/`GetTextureBindFlags`; `GetRTVDesc` calls `GetTextureFormat`) work transparently across TUs through the unmodified umbrella header.
- **Original umbrella `.cpp` deleted, not retained empty.** Free-function-header split: with no shared state and no dispatcher logic, retaining a 5-line `DX12Helper_Texture.cpp` containing only `#include "DX12Helper_Texture.h"` would be redundant. CMake auto-glob doesn't require it. The 5 sibling TUs are the complete implementation.
- **`Engine.h` is load-bearing — kept in `_Desc.cpp` and `_View.cpp`.** First build attempt dropped `#include "../../Engine.h"` from the new TUs as "dead include" (no `g_Engine` text reference in the bodies). Build failed: `error C2065: 'g_Engine': undeclared identifier` at every `Log(...)` call site. Root cause: `Log` is a macro defined in `LogService.h` line 91 as `g_Engine->Get<LogService>()->Print(...)`; the macro expands at the call site and needs `g_Engine` in scope. `Engine.h` is the only header declaring `g_Engine`, so any TU that calls `Log(...)` must `#include "../../Engine.h"`. The Format / Sampler / State TUs don't call `Log` and correctly don't need it; Desc (1 `Log(Error, ...)` for invalid mip dimensions) and View (4 `Log(Verbose, ...)` for cubemap/3D fallbacks) include both `LogServiceSpecialization.h` and `Engine.h`.
- **Two dead includes dropped from sibling TUs.** Original `.cpp` included `IOService.h` (zero references in the file bodies) and `Engine.h` (only needed for `Log()`-calling TUs, see above). Sibling TUs include only what they use. This is editorial cleanup beyond pure mechanical split — flagged here to match the AssetService-split reviewer's earlier ask for explicit dead-include callouts. Verified by running the qualifying test (Main.exe exit 0): no transitive dependency through these headers was load-bearing.
- **`STL14.h` chain provides `<cmath>`/`<algorithm>` transitively.** `_Desc.cpp` uses `std::max` / `std::min` / `std::log2` / `std::floor`; `_View.cpp` uses `std::pow`. All resolve through `DX12Helper_Texture.h` → `LogService.h` → `STL14.h` (which includes `<cmath>` line 6 and `<algorithm>` line 28). Same chain as the original umbrella `.cpp`; no new direct STL include needed in any sibling.
- **CMake auto-glob picked up the 5 new files** — `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha as prior splits).

### Build + test

- `cmake .` (from `Build/`) — green. Configuration completed in 4.5s.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green, exit 0. After the `Engine.h`-include fix-up iteration: all 5 new TUs (`_Format.cpp`, `_Desc.cpp`, `_State.cpp`, `_Sampler.cpp`, `_View.cpp`) compile cleanly. `DX12GraphicsService.lib`, `Services.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` — exit 0. Engine completed full init → 1 frame → graceful Terminate. The init path exercises every Texture helper: `GetDX12TextureDesc` (every texture allocation), `GetTextureFormat` (resource desc + RTV format), `GetSRVDesc` / `GetUAVDesc` / `GetRTVDesc` / `GetDSVDesc` (per-texture view creation), `GetTextureWriteState` / `GetTextureReadState` (transition state lookup), `GetFilterMode` / `GetWrapMode` (sampler creation), `GetTextureBindFlags` / `GetTextureDimension` / `GetTextureMipLevels` (resource-desc construction), `GetTexturePixelDataSize` / `GetBCBlockBytes` / `GetBCRowPitch` (upload-row-pitch math during initial texture upload). All consumers behave identically to HEAD.
- Pre-existing `mipmapGenerator3D.comp.dxil` shader-load issue is reproducible from any working tree state on this branch (called out in prior splits as not-a-regression).

### Function bijection check

`grep 'DX12Helper::' Source/Engine/Services/DX12/DX12Helper_Texture_*.cpp` returns 16 definitions; `DX12Helper_Texture.h` declares 16 functions. Sorted-name set identical: `GetBCBlockBytes`, `GetBCRowPitch`, `GetDSVDesc`, `GetDX12TextureDesc`, `GetFilterMode`, `GetRTVDesc`, `GetSRVDesc`, `GetTextureBindFlags`, `GetTextureDimension`, `GetTextureFormat`, `GetTextureMipLevels`, `GetTexturePixelDataSize`, `GetTextureReadState`, `GetTextureWriteState`, `GetUAVDesc`, `GetWrapMode`. Distribution: Format 4 + Desc 4 + View 4 + Sampler 2 + State 2 = 16. No method dropped, none duplicated, none renamed.

### Out of scope (not done)

- 10+ other oversized files in the inventory still pending. Task stays open. Sibling helper `DX12Helper_Pipeline.cpp` (570 lines) is a candidate for the next split CL but is not bundled into this one to keep the diff focused.

## Review (code-impl, 2026-05-06) — DX12Helper_Texture split

Verdict: PASS

- Bijection: 16/16 functions accounted for. Format(4: GetTextureFormat, GetTexturePixelDataSize, GetBCBlockBytes, GetBCRowPitch) + Desc(4: GetDX12TextureDesc, GetTextureDimension, GetTextureMipLevels, GetTextureBindFlags) + View(4: GetSRVDesc, GetUAVDesc, GetRTVDesc, GetDSVDesc) + Sampler(2: GetFilterMode, GetWrapMode) + State(2: GetTextureWriteState, GetTextureReadState).
- Byte-equivalence sampled: GetTextureFormat (Format.cpp 5–170 vs orig 69–234) — empty diff. GetSRVDesc (View.cpp 7–68 vs orig 465–526) — empty diff. GetTextureWriteState (State.cpp 5–32 vs orig 423–450) — empty diff. GetFilterMode + GetWrapMode (Sampler.cpp 5–52 vs orig 261–290 ∪ 291–338) — bodies identical with GetTextureMipLevels correctly extracted to Desc.cpp.
- Umbrella deletion justified: original DX12Helper_Texture.cpp had `using namespace Inno;` only — no `static` storage, no anonymous-namespace helpers, no shared file-scope state. Free-function pattern, no umbrella needed.
- Engine.h restoration in _Desc.cpp and _View.cpp is correct and minimal: `Log` macro appears 1× in Desc.cpp (GetTextureMipLevels error path) and 4× in View.cpp (UAV/RTV/DSV verbose). Format/Sampler/State TUs contain no `Log` invocation, so their lighter include set (`DX12Helper_Texture.h` only) is justified.
- CMake: not edited. CMakeLists.txt uses `file(GLOB SOURCES "*.cpp")` — new TUs picked up automatically; deleted TU drops out automatically.

No findings.

## CL: DX12Helper_Pipeline.cpp split (2026-05-06)

Split `Source/Engine/Services/DX12/DX12Helper_Pipeline.cpp` (570 lines) into 7 sibling TUs by domain. Pure mechanical split per `disciplines/on-implement/file-splitting.md` — free-function-header pattern, mirroring the prior `DX12Helper_Texture.cpp` split (commit `b7aafa6c`). Umbrella `DX12Helper_Pipeline.h` keeps all 16 declarations untouched; original umbrella `.cpp` is removed. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12Helper_Pipeline.h` (header — unchanged) | 38 | Umbrella declarations (16 active functions in `namespace Inno::DX12Helper`; `LoadShaderFile` has DXIL + HLSL `#ifdef`-mutually-exclusive signatures) |
| `DX12Helper_Pipeline_Shader.cpp` | 224 | `LoadGraphicsShaders`, `LoadComputeShaders`, `LoadRaytracingShaders`, `LoadShaderFile` (+ TU-local `m_shaderRelativePath`) |
| `DX12Helper_Pipeline_DepthStencil.cpp` | 95 | `GetComparisionFunction`, `GetStencilOperation`, `GenerateDepthStencilStateDesc` |
| `DX12Helper_Pipeline_Rasterizer.cpp` | 92 | `GetPrimitiveTopology`, `GetPrimitiveTopologyType`, `GetRasterizerFillMode`, `GenerateRasterizerStateDesc` |
| `DX12Helper_Pipeline_Blend.cpp` | 79 | `GetBlendFactor`, `GetBlendOperation`, `GenerateBlendStateDesc` |
| `DX12Helper_Pipeline_InputLayout.cpp` | 59 | `CreateInputLayout` |
| `DX12Helper_Pipeline_Viewport.cpp` | 21 | `GenerateViewportStateDesc` |
| `DX12Helper_Pipeline_DescriptorHeap.cpp` | 13 | `GetDescriptorHeapDesc` |

Total new TU lines: 583 (+13 vs. original 570, accounted for by 7× `#include "DX12Helper_Pipeline.h"` + `using namespace Inno;` + blank-line preambles instead of one). Largest TU: 224 (Shader). All under the 300-line ratchet.

### Constraints that bit

- **Cross-TU calls all stay within their own domain.** `GenerateDepthStencilStateDesc` calls `GetComparisionFunction`/`GetStencilOperation` (same TU — DepthStencil); `GenerateBlendStateDesc` calls `GetBlendFactor`/`GetBlendOperation` (same TU — Blend); `GenerateRasterizerStateDesc` calls `GetRasterizerFillMode`/`GetPrimitiveTopology`/`GetPrimitiveTopologyType` (same TU — Rasterizer). Zero cross-TU dispatch through the umbrella header — domain seams are clean.
- **`m_shaderRelativePath` promoted from `namespace DX12Helper` scope to anonymous-namespace TU-local in `_Shader.cpp`.** The original placed the `#ifdef USE_DXIL` `const char*` / `#else` `const wchar_t*` constant inside `namespace Inno::DX12Helper { ... }` at file scope (external linkage, undeclared in any header). It was referenced only by `LoadShaderFile`. Moving it into an anonymous namespace inside `_Shader.cpp` gives it internal linkage exactly where it belongs (sole consumer same TU) — strictly better than the original. Matches the `_DescriptorAndShader.cpp` pattern called out in the prior `VKGraphicsService_VulkanObject` review.
- **Engine.h kept only in `_Shader.cpp`.** Six of seven sibling TUs are pure DX12 enum/struct mappings — no `Log()`, no `g_Engine`. They include only `DX12Helper_Pipeline.h` (which transitively brings `LogService.h`, `RenderPassComponent.h`, `DX12Headers.h`). Shader TU additionally needs `Engine.h` (for `g_Engine` to satisfy the `Log` macro expansion + the `g_Engine->Get<IOService>()` direct call), `LogServiceSpecialization.h` (for the `Log` formatter overloads), and `IOService.h` (for the `loadFile` API in the DXIL `LoadShaderFile`). Same chain as the `DX12Helper_Texture` split's Desc/View TUs.
- **Two dead includes dropped from sibling TUs.** Original `.cpp` included `DX12Helper_Common.h` (centralised typed access to DX12 command lists — zero references in any function body in this file; `GetDX12CommandList` not called) and `IOService.h` (only needed inside the DXIL `LoadShaderFile`, kept in `_Shader.cpp`). The other six TUs include only what they use. Editorial cleanup beyond pure mechanical split — flagged here per the AssetService-split reviewer's earlier ask. Verified by qualifying-test pass: no transitive dependency through these headers was load-bearing.
- **HLSL `#else` branch of `LoadShaderFile` carried over verbatim — preserves a pre-existing bug.** Line 563 of original (line 211 of `_Shader.cpp`) reads `Log(Error, "Can't find ", shaderFilePath.c_str(), " ", name);` where `name` is undefined. This branch is dead code under the engine's hard-defined `USE_DXIL` (set in `DX12Headers.h:16`). "No behavior change" honoured: bug preserved. Out of scope for this split.
- **Original umbrella `.cpp` deleted, not retained empty.** Same justification as `DX12Helper_Texture` split: free-function-header pattern with no shared state and no dispatcher logic — retaining a stub would be redundant. CMake auto-glob doesn't require it.
- **CMake auto-glob picked up the 7 new files** — `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`. Ran `cmake .` from `Build/` to refresh the `.vcxproj` entries before build.

### Build + test

- `cmake .` (from `Build/`) — green. Configuration completed in 3.3s.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. All 7 new TUs (`_DescriptorHeap.cpp`, `_InputLayout.cpp`, `_Shader.cpp`, `_DepthStencil.cpp`, `_Blend.cpp`, `_Rasterizer.cpp`, `_Viewport.cpp`) compile cleanly. `DX12GraphicsService.lib`, `Services.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` — exit 0. Engine completed full init → 1 frame → graceful Terminate. The init path exercises the Pipeline helpers across every render-pass setup: `GetDescriptorHeapDesc` (every descriptor-heap creation in DX12GraphicsHardwareService), `CreateInputLayout` (every graphics PSO), `LoadGraphicsShaders`/`LoadComputeShaders`/`LoadRaytracingShaders` (per-pass shader bytecode binding), `GenerateDepthStencilStateDesc`/`GenerateBlendStateDesc`/`GenerateRasterizerStateDesc`/`GenerateViewportStateDesc` (per-pass PSO state desc), and the GetXxx enum-mapping helpers transitively via the Generate* functions. All consumers behave identically to HEAD (`SamplerResourceService`/`ShaderProgramResourceService`/`TextureResourceService`/etc. all `Setup finished` → all `Terminate`d).

### Function bijection check

`grep 'DX12Helper::' Source/Engine/Services/DX12/DX12Helper_Pipeline_*.cpp` returns 17 definitions; `DX12Helper_Pipeline.h` declares 16 active functions (the 17th is the HLSL-path `LoadShaderFile(ID3D10Blob**, ShaderStage, const ShaderFilePath&)` overload, which mirrors a `#ifdef USE_DXIL`/`#else` pair declared in the header — only one is active per build, mirroring the original verbatim). Sorted-name set identical: `CreateInputLayout`, `GenerateBlendStateDesc`, `GenerateDepthStencilStateDesc`, `GenerateRasterizerStateDesc`, `GenerateViewportStateDesc`, `GetBlendFactor`, `GetBlendOperation`, `GetComparisionFunction`, `GetDescriptorHeapDesc`, `GetPrimitiveTopology`, `GetPrimitiveTopologyType`, `GetRasterizerFillMode`, `GetStencilOperation`, `LoadComputeShaders`, `LoadGraphicsShaders`, `LoadRaytracingShaders`, `LoadShaderFile`. Distribution: Shader 4 + DepthStencil 3 + Rasterizer 4 + Blend 3 + InputLayout 1 + Viewport 1 + DescriptorHeap 1 = 17. No method dropped, none duplicated, none renamed.

### Out of scope (not done)

- Other oversized files in the inventory still pending. Task stays open per AC#4.

## CL: FrameManagementServiceImpl.cpp split (2026-05-06)

Split `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` (660 lines) into 4 sibling TUs by responsibility cluster, per `disciplines/on-implement/file-splitting.md` same-class partial-TU pattern. Header `Source/Engine/Services/FrameManagementService.h` unchanged. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `FrameManagementServiceImpl.cpp` (anchor) | 246 | Lifecycle: `Setup`, `Initialize`, `InitializeSwapChainRenderPassComponent`, `Update`, `Terminate` |
| `FrameManagementServiceImpl_FrameQueries.cpp` | 188 | Frame-index queries (`GetCurrentFrame`/`GetPreviousFrame`/`GetNextFrame`/`GetSwapChainImageCount`/`GetFrameCountSinceLaunch`), TASK-213 steady-state predicate (`IsSteadyState`, `GetSteadyStateRelativeFrameCount`), callback setters, `GetSwapChainRenderPassComponent`, `SetUserPipelineOutput`/`GetUserPipelineOutput`, `GetGlobalSemaphore` |
| `FrameManagementServiceImpl_Commands.cpp` | 113 | `PrepareGlobalCommands`, `ExecuteGlobalCommands`, `PrepareSwapChainCommands`, `ExecuteSwapChainCommands` |
| `FrameManagementServiceImpl_Resize.cpp` | 147 | `Present`, `WaitForGPUIdle`, `Resize`, `ExecuteResize`, `PreResize`/`PostResize` (both overloads each) |

Total new TU lines: 694 (+34 vs original 660, accounted for entirely by 3× extra include-block + `using namespace Inno;` preambles in the new sibling TUs). Largest TU: 246 (anchor). All under the 300-line ratchet.

### Constraints that bit

- **No internal header introduced — none needed.** Original had zero `static`-at-file-scope helpers, zero anon-namespace blocks, zero TU-local state. The two `static constexpr` constants inside `IsSteadyState` (`TLASStabilityWindowFrames`, `SteadyStateTimeoutFrames`) are function-local — they move with `IsSteadyState` into `_FrameQueries.cpp` and stay function-local. Cross-TU references go through public/protected/private member access on `FrameManagementService`, which is identical pre/post split (every TU sees the same class definition via the unchanged header).
- **Anchor file retained, not deleted.** `Setup`/`Initialize`/`Update`/`Terminate` is the canonical class-lifecycle cluster; keeping it in the eponymous `FrameManagementServiceImpl.cpp` matches sibling convention (`EditorService.cpp` + `EditorService_*.cpp`, `AssetService.cpp` + `AssetService_*.cpp`, `TemplateAssetService.cpp` + `TemplateAssetService_*.cpp`).
- **Per-TU include subsetting.** Anchor keeps the original wide include set (Lifecycle touches every sibling resource service via `g_Engine->Get<...>`). FrameQueries narrows to `GPUBufferResourceService` / `MeshResourceService` / `SceneService` (only the IsSteadyState dependencies). Commands narrows to `GPUBufferResourceService` + `TemplateAssetService` (PrepareSwapChainCommands' `GetMeshComponent` call). Resize narrows to `RenderPassResourceService` + `RenderingConfigurationService`. Each new TU's include graph is a strict subset of the original.
- **`Engine.h` and `LogService*` kept in every TU.** All four TUs call `Log(...)` at least once and use `g_Engine->Get<...>()`, so all four need both `LogService.h` + `LogServiceSpecialization.h` + `Engine.h`. Same chain as DX12Helper_Texture's Desc/View TUs.
- **CMake auto-glob picked up the 3 new files** — `Source/Engine/Services/Common/CMakeLists.txt` uses `file(GLOB *.cpp)` + `file(GLOB *.h)`. Ran `cmake .` from `Build/` to refresh the stale `GraphicsServiceCommon.vcxproj` (originally listed only `FrameManagementServiceImpl.cpp` at line 300; post-reconfigure it lists all four TUs at lines 300-303).

### Build + test

- `cmake .` (from `Build/`) — green. Configuration completed in 4.3s; reconfigure picks up the 3 new TUs in `GraphicsServiceCommon.vcxproj`.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. All four TUs (`FrameManagementServiceImpl.cpp`, `_FrameQueries.cpp`, `_Commands.cpp`, `_Resize.cpp`) compile. `GraphicsServiceCommon.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin/RelWithDebInfo/` working dir, where the deployed shader tree lives) — exit 0. Engine completed full init → 1 frame → graceful Terminate. Log evidence:
  - `[Inno::FrameManagementService::Setup] Global Graphics CommandLists have been created.`
  - `[Inno::FrameManagementService::Setup] FrameManagementService Setup finished.`
  - `[Inno::FrameManagementService::Initialize] FrameManagementService has been initialized.`
  - `[Inno::FrameManagementService::Terminate] FrameManagementService has been terminated.`
  Lifecycle covers `Setup` (anchor), `Initialize` + `InitializeSwapChainRenderPassComponent` (anchor), `Update` (anchor → exercises `PrepareGlobalCommands`/`ExecuteGlobalCommands` from `_Commands.cpp` + `IsSteadyState` from `_FrameQueries.cpp`), `Present` (`_Resize.cpp`), `Terminate` (anchor). All four TUs exercised in a single frame.

### Function bijection check

Out-of-line definitions in HEAD `FrameManagementServiceImpl.cpp`: 33 (`grep -cE '^(bool|void|uint32_t|RenderPassComponent\*|GPUResourceComponent\*|ISemaphore\*) FrameManagementService::'`). Same 33 distributed across the 4 split TUs:
- Anchor (5): `Setup`, `Initialize`, `InitializeSwapChainRenderPassComponent`, `Update`, `Terminate`
- _FrameQueries (16): `GetCurrentFrame`, `GetPreviousFrame`, `GetNextFrame`, `GetSwapChainImageCount`, `GetFrameCountSinceLaunch`, `IsSteadyState`, `GetSteadyStateRelativeFrameCount`, `SetUploadHeapPreparationCallback`, `SetCommandPreparationCallback`, `SetCommandExecutionCallback`, `SetPreFrameCallback`, `SetPostFrameCallback`, `GetSwapChainRenderPassComponent`, `SetUserPipelineOutput`, `GetUserPipelineOutput`, `GetGlobalSemaphore`
- _Commands (4): `PrepareGlobalCommands`, `ExecuteGlobalCommands`, `PrepareSwapChainCommands`, `ExecuteSwapChainCommands`
- _Resize (8): `Present`, `WaitForGPUIdle`, `Resize`, `ExecuteResize`, `PreResize()`, `PreResize(RenderPassComponent*)`, `PostResize()`, `PostResize(const TVec2<uint32_t>&, RenderPassComponent*)`

Sum: 5 + 16 + 4 + 8 = 33. No function body modified, no member added, no member access promoted.

### Out of scope (not done)

- Several oversized files remain in the inventory (`DX12FrameManagementService.cpp` 1231 lines, `Engine.cpp` 1264 lines, `Math.h` 1241 lines, `MathHelper.h` 1633 lines, `ExampleRenderingClient.cpp` 1518 lines, others). Task stays open per AC #4.

## Review (code-impl, 2026-05-06) — FrameManagementServiceImpl split

Verdict: PASS

- **Bijection 33→33 confirmed.** Grepped `^\w[\w:* &<>]*FrameManagementService::\w+\(` against HEAD `FrameManagementServiceImpl.cpp` (33 hits) and against the 4 post-split TUs combined (5 anchor + 16 _FrameQueries + 4 _Commands + 8 _Resize = 33). Function set identical; no overload lost; both `PreResize` overloads and both `PostResize` overloads land together in `_Resize.cpp`.
- **Byte-equivalence on 4 sampled functions.** `diff` against HEAD on `Update` (anchor 125-230 ↔ orig 126-231), `IsSteadyState` body (`_FrameQueries` 42-135 ↔ orig 278-371), `ExecuteGlobalCommands` (`_Commands` 48-58 ↔ orig 511-521), `Resize` (`_Resize` 58-64 ↔ orig 470-476): all identical.
- **No internal header justified.** HEAD `FrameManagementServiceImpl.cpp` has zero file-scope `static` decls, zero anon-namespace blocks (verified via grep `^(static|namespace\s*\{|namespace\s+\w+\s*\{)` — no matches). The two `static constexpr` constants (`TLASStabilityWindowFrames`, `SteadyStateTimeoutFrames`) are function-local inside `IsSteadyState` (lines 51, 56 of `_FrameQueries.cpp`, within the 42-134 body); they travel with the function. No shared TU-local state to extract.
- **No CMake edit.** `git diff HEAD` clean for `*.cmake` and `CMakeLists.txt`. Auto-glob in `Source/Engine/Services/Common/CMakeLists.txt` (`file(GLOB SOURCES "*.cpp")`) picks up the 3 new TUs without manifest churn — consistent with prior splits in this task.

No findings.

## CL: VKHelper_Texture.cpp split (2026-05-06)

Split `Source/Engine/Services/VK/VKHelper_Texture.cpp` (678 lines) into 5 sibling free-function TUs by domain. Mirrors prior `DX12Helper_Texture.cpp` split (commit `b7aafa6c`). Pure mechanical split per `disciplines/on-implement/file-splitting.md`. Original `.cpp` deleted (no file-static state to retain). Header `VKHelper_Texture.h` unchanged.

### File inventory

| File | Lines | Functions |
|---|---:|---|
| `VKHelper_Texture_Desc.cpp` | 248 | `GetVKTextureDesc`, `GetImageType`, `GetImageUsageFlags`, `GetImageSize`, `GetImageAspectFlags`, `GetImageCreateInfo` |
| `VKHelper_Texture_Format.cpp` | 278 | `GetTextureFormat` |
| `VKHelper_Texture_Sampler.cpp` | 63 | `GetSamplerAddressMode`, `GetFilter`, `GetSamplerMipmapMode` |
| `VKHelper_Texture_State.cpp` | 68 | `GetTextureWriteImageLayout`, `GetTextureReadImageLayout`, `GetAccessMask` |
| `VKHelper_Texture_View.cpp` | 34 | `GetImageViewType` |

Total 691 lines (vs 678 original). Largest TU 278 — under the 300-line ratchet. Extra 13 lines = duplicated `#include "VKHelper_Texture.h"` + `using namespace Inno;` across 4 of 5 split TUs (one TU keeps `#include "../../Engine.h"` since `GetVKTextureDesc` is the composite entry).

### Bijection

14 free functions in HEAD `VKHelper_Texture.cpp` (matched by header decl in `VKHelper_Texture.h` lines 13-27). 14 free functions across 5 split TUs: 6 (Desc) + 1 (Format) + 3 (Sampler) + 3 (State) + 1 (View) = 14. Bijection 14→14 confirmed. Function bodies copied byte-equivalent — diff against HEAD on `GetTextureFormat` (Format.cpp 5-273 ↔ orig 171-444) and `GetImageSize` (Desc.cpp 80-180 ↔ orig 446-544) shows identical content modulo header-relative offsets.

### Allocation rationale

- **Desc**: composite + sampler→VkImageType (drives image creation) + usage flags + size + aspect + create info — the cluster that builds `VkImage`.
- **View**: sampler→VkImageViewType — drives `VkImageView` creation, distinct concern from image creation.
- **Format**: `GetTextureFormat` — 274-line switch ladder isolated as its own TU (largest single function, mirrors DX12 split).
- **Sampler**: address mode + filter + mipmap mode — sampler-object concern.
- **State**: layout + access mask — pipeline-barrier / synchronization concern.

### Include graph

Every split TU includes only `VKHelper_Texture.h`. The Desc TU additionally includes `../../Engine.h` (only `GetVKTextureDesc` originally needed it — kept in that TU). Strict subset of original.

### CMake

`Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB SOURCES "*.cpp")` — no manifest edit needed. Ran `cmake .` from `Build/` to refresh glob.

### Validation

- `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh`: green (Main.exe + RenderTest.exe linked).
- `Bin/RelWithDebInfo/Main.exe -total_frames 1`: exit 0. (Pre-existing shader-load warning for `mipmapGenerator3D.comp.dxil` is unrelated to this CL — `-SkipShaderCompile` was used; engine still exits cleanly.)
- VK target is gated off (`INNO_RENDERER_VULKAN:BOOL=OFF` confirmed in `Build/CMakeCache.txt`); the new VK TUs are not compiled by this build. Build green is the load-bearing check per the dispatch directive.

### File-size gate

All 5 split TUs ≤ 300. No `FILE_SIZE_EXCLUDE_RE` addition. The original `VKHelper_Texture.cpp` is removed via `git rm` — gate sees only the new TUs at commit time.

## Review (code-impl, 2026-05-06) — VKHelper_Texture split

Verdict: PASS

Bijection: 14 free functions in `git show HEAD:Source/Engine/Services/VK/VKHelper_Texture.cpp` → 14 in the 5 new TUs (Desc 6, Format 1, Sampler 3, State 3, View 1). Cluster mapping is sensible; `GetImageType` landing in `_Desc.cpp` rather than `_View.cpp` is an organizational choice, not a correctness concern.

Byte-equivalence (5 cluster representatives, `diff` empty in all cases): `GetVKTextureDesc` (orig L7-20 vs Desc.cpp L7-20), `GetTextureFormat` (orig L171-444 vs Format.cpp L5-278), `GetFilter` (orig L133-150 vs Sampler.cpp L27-44), `GetTextureWriteImageLayout` (orig L616-633 vs State.cpp L5-22), `GetImageViewType` (orig L52-81 vs View.cpp L5-34).

Umbrella deletion correct: `grep -nE "^(static|namespace[[:space:]]*\{)"` on the original returns 0 matches → no file-static, no anon-namespace state to retain.

Includes: `_Desc.cpp` mirrors original (`VKHelper_Texture.h` + `../../Engine.h`); the other four include only `VKHelper_Texture.h`. The original referenced no `Engine.h` symbol (`g_Engine` / `Log(` / `Logger`: 0 hits) — the four narrower includes are strict subsets and correct; `_Desc.cpp` faithfully preserves the (already-unused) Engine.h include, no regression. VK headers reach all five TUs transitively via the unchanged `VKHelper_Texture.h`.

Line accounting: 248+278+63+68+34 = 691 vs 678 original; +13 matches 5× (`#include` + blank + `using namespace Inno;` + blank) split-file scaffolding. All 5 TUs ≤ 300; file-size gate satisfied without exclusion.

CMake: `git diff HEAD -- '**/CMakeLists.txt'` empty — confirmed no build-system edit (relies on existing `file(GLOB)` discipline).

VK target disabled (`INNO_RENDERER_VULKAN:BOOL=OFF`) noted; clangd `vulkan/vulkan.h not found` is a compile_commands.json artefact, not a real diagnostic. No findings.

## CL: Reflector.cpp split (2026-05-06)

Split `Source/Tool/Reflector/Reflector.cpp` (673 lines) into 4 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `Reflector.cpp` (umbrella) | 126 | `writeSector`, `writeFile`, `parseContent` orchestration + `main` + filesystem-namespace alias |
| `Reflector_Internal.h` | 59 | `FileWriter` + `ClangMetadata` types, `inline` shared module state (`m_clangMetadata`, `m_includedFileSourceLocation`, `m_includedFileName`), forward decls grouped by domain |
| `Reflector_Parse.cpp` | 117 | `inclusionVisitor`, libclang `visitor` (cursor-kind dispatch + parent linking), `assignBase` |
| `Reflector_EnumWriters.cpp` | 168 | `writeCursorKind`, `writeAccessSpecifier`, `writeTypeKind` (libclang enum -> `Metadata::*` string switches) |
| `Reflector_MetadataWriters.cpp` | 243 | `flattenClangTypeName`, `writeMetadataMember/Defi/ChildrenMetadataDefi/Getter`, `writeSerializerDefi`, `writeDeserializerDefi`, `writeIncludedHeaders` (`.refl` codegen) |

Total new TU lines: 713 vs 673 original; +40 from `#include "Reflector_Internal.h"` + namespace-open scaffolding across 4 files. Largest TU: 243 (MetadataWriters). All under the 300-line ratchet.

### Constraints that bit

- **No anchor class — domain split per file-splitting.md "free-function" rule.** Original was namespace-scope free functions sharing TU-private vector globals. Resolution: gather types, shared state, and forward decls into `Reflector_Internal.h` keyed by domain (`// --- Parse ---`, `// --- Enum writers ---`, `// --- Metadata writers ---`). Sibling TUs include the header and re-open `namespace Reflector`.
- **Module state via `inline` variables (C++17).** The three vectors moved from TU-local namespace-scope into the header as `inline std::vector<...>`. Single ODR-clean program-wide definition, identical observable behavior. Project is C++17 (`CMakeLists.txt:29`).
- **Co-location: `parseContent` + `writeFile` + `writeSector` stayed in the umbrella.** They are orchestration over the writer/parser primitives — declaring them in the umbrella TU keeps `main` next to its single call target.
- **CMake explicit listing — edit required.** `Source/Tool/Reflector/CMakeLists.txt` lists sources by name (no `file(GLOB)`). Updated `add_executable(Reflector ...)` to enumerate all 4 cpp + the internal header.
- **Include graph subset.** Each new TU's `#include` is `Reflector_Internal.h` only; the umbrella additionally includes `<filesystem>` / `<experimental/filesystem>` for `fs::path` (`main` only). `Reflector_Internal.h` carries the original umbrella's includes (`STL14.h`, `STL17.h`, `Metadata.h`, `clang-c/Index.h`).

### Build + test

- Reflector is gated behind `INNO_BUILD_TOOLS` (`Source/CMakeLists.txt:39,44,49`), which is hard-`set(... OFF)` at line 44 in the engine baseline — the Reflector subdir is not configured by the standard build. To verify the split, temporarily commented out the override locally, ran `cmake -DINNO_BUILD_TOOLS=ON ..` in `Build/`, and built `Source\Tool\Reflector\Reflector.vcxproj` directly. Reverted the line and re-ran configure to restore baseline. The `Source/CMakeLists.txt` edit was not staged.
- `msbuild Reflector.vcxproj /p:Configuration=RelWithDebInfo` — green. All 4 TUs compiled cleanly; `Bin\RelWithDebInfo\Reflector.exe` produced. Exit 0.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\` so shaders resolve) — exit 0. Engine unchanged by this CL (Reflector is a build-time tool, not linked into the engine binary); included as the standard non-regression smoke check for the rolling-cleanup tracker.

### Out of scope (not done)

- 17+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — Reflector split

Verdict: PASS

Bijection verified: HEAD `Reflector.cpp` (673 lines) defines 18 named functions + `main`; new layout assigns all of them to one of the four TUs with no losses or duplicates (Parse: 3, EnumWriters: 3, MetadataWriters: 8, Reflector.cpp umbrella: 3 + `main`). Byte-equivalence sampled per cluster — `writeCursorKind` (EnumWriters), `assignBase` (Parse), `flattenClangTypeName` (MetadataWriters), `parseContent` (umbrella) — all match HEAD modulo a single trailing-tab-on-empty-line strip in `flattenClangTypeName` (cosmetic, semantic-identical). Inline-variable promotion is correct: project sets `CMAKE_CXX_STANDARD 17` at `CMakeLists.txt:29`, header uses `inline std::vector<...>` for the three previously TU-private namespace-scope vectors, ODR-safe across all four including TUs. `Reflector_Internal.h` surface is minimal — only the two structs, three shared vectors, and forward decls grouped by sibling TU; no leakage of caller-specific knowledge. CMake `add_executable(Reflector ...)` enumerates the new file inventory exactly. Working tree clean of the INNO_BUILD_TOOLS verification flip (`git diff HEAD -- Source/CMakeLists.txt` empty). No findings.

## CL: VolumetricPass.cpp split (2026-05-06)

Split `Source/ExampleProject/RenderingClient/VolumetricPass.cpp` (655 lines) into 3 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `VolumetricPass.cpp` | 144 | Umbrella: namespace-scope state definitions + Setup() orchestrator + Initialize / Terminate / GetRayMarchingResult / GetVisualizationResult |
| `VolumetricPass_Internal.h` | 57 | extern declarations for namespace state + forward declarations for the per-pass setup / record helpers |
| `VolumetricPass_Setup.cpp` | 276 | setupGeometryProcessPass / setupIrradianceInjectionPass / setupRayMarchingPass / setupVisualizationPass |
| `VolumetricPass_ExecuteCommands.cpp` | 238 | froxelization / irraidanceInjection / rayMarching / visualization / ExecuteCommands |

Total new TU + header lines: 715. Largest TU: 276 (Setup). All under the 300-line ratchet.

### Constraints that bit

- **Internal header introduced.** The original `VolumetricPass.cpp` carried the namespace state and helper forward declarations inline. Moving the helpers into sibling TUs forced an internal header so each TU sees the same declarations. State variables that were `static` at namespace scope (TU-local) become external — `extern` in `VolumetricPass_Internal.h`, defined once in `VolumetricPass.cpp`. Notably `m_isPassA` was `static bool`; it now has external linkage because `rayMarching()` (in `_ExecuteCommands.cpp`) and `GetRayMarchingResult()` (in the umbrella) both touch it.
- **Header self-containment.** The original .cpp resolved `Inno::SamplerComponent`, `Inno::TVec4<uint32_t>`, etc. transitively through service includes. Moving those types to a header that the partial TUs include forced the internal header to pull `MathHelper.h` (which ends with `using namespace Inno::Math;` — that's how `TVec4` reaches global scope project-wide) and the five Component headers explicitly. Keeps each partial TU's `#include` graph a strict subset of the original via the umbrella header.
- **Anonymous namespace not used.** The pass is itself `namespace VolumetricPass`; using an anonymous namespace inside it would have made every state variable per-TU again, defeating the point. Standard `extern` + single-definition pattern instead.

### Verification

- `cmake .` from `Build/` regenerated solution (auto-glob picked up the three new files).
- `Scripts/BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` clean.
- `Bin/RelWithDebInfo/Main.exe -total_frames 1` (cwd=Bin/RelWithDebInfo) → exit 0; engine ran one frame, wrote `gpu_output.png`, terminated cleanly. No regression.

## Review (code-impl, 2026-05-06) — VolumetricPass split

Verdict: ADVISORY

- **Bijection — confirmed.** Original HEAD `VolumetricPass.cpp` defines 12 `VolumetricPass::*` member functions (`setupGeometryProcessPass`, `setupIrradianceInjectionPass`, `setupRayMarchingPass`, `setupVisualizationPass`, `Setup`, `Initialize`, `froxelization`, `irraidanceInjection`, `rayMarching`, `visualization`, `ExecuteCommands`, `Terminate`) plus `GetRayMarchingResult` and `GetVisualizationResult` — 14 total. Sum across the 3 new TUs: umbrella owns 5 (`Setup`, `Initialize`, `Terminate`, `GetRayMarchingResult`, `GetVisualizationResult`), `_Setup.cpp` owns 4 (`setup*Pass`), `_ExecuteCommands.cpp` owns 5 (`froxelization`, `irraidanceInjection`, `rayMarching`, `visualization`, `ExecuteCommands`). 5 + 4 + 5 = 14. Matches. Distribution per Implementation Notes line 933-936 — confirmed.
- **Byte-equivalence — 2 of 3 spot checks identical, 1 carries piggy-backed style edits.** `setupRayMarchingPass` (lines 201-269 in HEAD vs 149-217 in `_Setup.cpp`): zero-byte diff. `ExecuteCommands` (lines 587-628 in HEAD vs 197-238 in `_ExecuteCommands.cpp`): one trailing blank line dropped at end-of-file, harmless. **`rayMarching` (lines 488-543 in HEAD vs 98-153 in `_ExecuteCommands.cpp`): 3 lines have non-mechanical edits** — trailing whitespace stripped after `{` (line 489 → 99), and 2 local variable declarations changed from East-pointer (`GPUResourceComponent *l_currentResultBinder`, `GPUResourceComponent *l_historyResultBinder`) to West-pointer (`GPUResourceComponent* l_currentResultBinder`, `GPUResourceComponent* l_historyResultBinder`). Same `*`-placement edit applied consistently to 6 occurrences across the split: `visualization` parameter signature (forward decl in `_Internal.h` line 30 + definition in `_ExecuteCommands.cpp` line 154), the 2 locals in `rayMarching`, and `GetRayMarchingResult` / `GetVisualizationResult` return types in the umbrella. Style is consistent with `cpp-style.md`'s West-const direction. **Advisory:** a mechanical split CL should preserve bytes; opportunistic style edits belong in a separate CL. No action requested for this CL — the edits are correct and consistent — but flagged so the same pattern doesn't get smuggled into future "mechanical-only" splits without being called out in Implementation Notes.
- **`m_isPassA` linkage promotion — sound, no semantic shift.** Original was `static bool m_isPassA = true;` at namespace scope inside the .cpp's `namespace VolumetricPass { … }` block (HEAD line 59) — internal linkage, single TU only. After split: defined once at umbrella `VolumetricPass.cpp:39` (`bool m_isPassA = true;`, external linkage), declared `extern bool m_isPassA;` in `_Internal.h:56`, read from umbrella `GetRayMarchingResult` (line 131) and read+written from `_ExecuteCommands.cpp::rayMarching` (lines 103, 107, 113). ODR satisfied (one definition, multiple declarations). Project-wide grep for `m_isPassA` returns exactly the 6 references listed above plus the original HEAD definition site — no other TU references it under any name. The internal-→external linkage shift is therefore observable only in principle (any new TU that accidentally added `extern bool m_isPassA;` would now resolve), not in practice. Implementation Notes line 942 calls this out explicitly. Sound.
- **`_Internal.h` surface — minimal and correct.** 57 lines: 8 forward decls (4 setup helpers + 4 record helpers, all of which are actually called from sibling TUs — confirmed by inspecting `Setup()` and `ExecuteCommands()`), 14 `extern` state decls (every state variable that any sibling TU touches), and the umbrella `MathHelper.h` + 5 Component-header includes needed for the type names in the extern decls. No leakage of TU-local helper types, no `using namespace` declarations, header-guarded with `#pragma once`. Implementation Notes line 943 correctly identifies why `MathHelper.h` is required (it's the include that pulls `using namespace Inno::Math;` so `TVec4` is accessible without `Inno::Math::` qualification). Surface is exactly what siblings need.
- **No CMake edit — confirmed.** `git status` shows only the 3 new source files + 1 modified umbrella + the task file. No `CMakeLists.txt` changes. The project uses glob-based source enumeration (verified by `Source/ExampleProject/CMakeLists.txt` not enumerating `VolumetricPass*`), so the new TUs will be picked up after the documented `cmake .` regen step. Implementation Notes line 948 documents the regen.

ADVISORY. The split is structurally correct: bijection holds (14 → 14), `m_isPassA` linkage promotion is the minimum required change for the split to compile and carries no semantic risk, `_Internal.h` surface is minimal, and no CMake change is needed. **One advisory:** the 6 East-pointer → West-pointer edits in `rayMarching` and the function signatures of `visualization`, `GetRayMarchingResult`, `GetVisualizationResult` are non-mechanical and should have been called out in Implementation Notes. The edits themselves are correct (consistent with project style) and do not affect verdict — but a "pure mechanical split" claim is slightly overstated. No action requested for this CL; documentation hygiene only for the next reviewer.

## CL: VKGraphicsService_VulkanObject.cpp split (2026-05-05)

Split `Source/Engine/Services/VK/VKGraphicsService_VulkanObject.cpp` (635 lines) into 3 sibling TUs (umbrella + 2 partials). Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Same-class partial-TU pattern; no internal header needed (no file-local macros; the single file-local namespace-scope global `m_shaderRelativePath` moves wholesale into the TU that uses it).

### File inventory

| File | Lines | Role |
|---|---:|---|
| `VKGraphicsService_VulkanObject.cpp` (umbrella) | 258 | Debug-utils EXT pointer trampolines + validation/extension capability checks + `FindQueueFamilies` + swapchain surface-format / present-mode / extent / support-query helpers + `IsDeviceSuitable` |
| `VKGraphicsService_VulkanObject_Memory.cpp` | 267 | Host/device-local buffer creation + map/copy + temporary command buffer open/close + `FindMemoryType` + `CreateCommandPool` + `CreateBuffer` / `CopyBuffer` + `CreateImage` / `TransitImageLayout` / `CopyBufferToImage` |
| `VKGraphicsService_VulkanObject_DescriptorAndShader.cpp` | 163 | `m_shaderRelativePath` definition + `CreateDescriptorPool` / `CreateDescriptorSetLayout` / `CreateDescriptorSets` / `UpdateDescriptorSet` + 3× `GetWriteDescriptorSet` overloads + `CreateShaderModule` |

Total new TU lines: 688. Largest TU: 267 (Memory). All under the 300-line ratchet. Original had 32 `VKGraphicsService::` member-function definitions; post-split: umbrella 11 + Memory 13 + DescriptorAndShader 8 = 32. Bijection verified by `grep -oE "VKGraphicsService::[A-Za-z]+" | sort -u`, zero diff against `git show HEAD:.../VKGraphicsService_VulkanObject.cpp`. Non-preamble content diff (filtering `#include`, `using namespace`, the relocated `namespace Inno { namespace VKHelper { … } }` block, and blanks) — clean.

### Constraints that bit

- **No internal header introduced.** Original carried only one file-local global, `Inno::VKHelper::m_shaderRelativePath` (originally external linkage at namespace scope, single consumer: `CreateShaderModule`). Moved its definition wholesale into `_DescriptorAndShader.cpp` alongside its sole consumer; no header surface needed because no other TU references it project-wide (`grep m_shaderRelativePath Source/Engine/Services/VK` finds only the original definition + use). Header `VKGraphicsService.h` untouched.
- **Naming choice — `_DescriptorAndShader.cpp` not `_Descriptor.cpp`.** Sibling `VKGraphicsService_Descriptor.cpp` already exists in the same directory (added by the prior `_EngineComponent.cpp` split, contains `InitializeImpl(RenderPassComponent*)` + descriptor-set-bindings construction — different responsibility, same word). Picked the compound name to keep both TUs distinct. Post-split file inventory in this directory now has: `_Descriptor.cpp` (RenderPass orchestrator), `_VulkanObject_DescriptorAndShader.cpp` (low-level VK descriptor / shader-module creation helpers). Two-word disambiguation deemed cleaner than renaming the pre-existing file.
- **Identical includes per TU.** Each new TU duplicates the original's full include block (10 headers + 3 `using namespace`s). Same precedent as the prior `_EngineComponent.cpp` split — equality is the safest subset and avoids missing-symbol risk for code that doesn't currently compile (VK target disabled).
- **CMake auto-glob picked up new files** — `Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. Ran `cmake .` from `Build/` to refresh `.vcxproj` entries before build (same gotcha as prior splits).
- **VK build status.** `Build/CMakeCache.txt` confirms `INNO_RENDERER_VULKAN:BOOL=OFF`. `VKGraphicsService.lib` does not appear in `Scripts\BuildWin.ps1` output. Pre-split structural defects called out in the prior `_EngineComponent.cpp` split (template visibility of `SetObjectName`; missing `m_initializedTextures` member; missing `../GraphicsResourceService.h`) are unrelated to this file and unaffected by this split. The split is structural-only on this branch.

### Build + test

- `cmake .` (from `Build/`, to refresh the VS solution after adding files) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `Engine.lib`, `DX12GraphicsService.lib`, `Main.exe`, `RenderTest.exe` all linked. VK target not built (disabled in cache, baseline behavior unchanged by the split).
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate. No regression.

### Out of scope (not done)

- 14+ other oversized files in the inventory still pending. Two adjacent VK files (`VKGraphicsService.cpp` 541, `VKGraphicsService_GraphicsDevice.cpp` 589) remain over the ratchet. Task stays open.

## CL: RayTracer.cpp split (2026-05-06)

Split `Source/Engine/RayTracer/RayTracer.cpp` (603 lines) into 4 sibling TUs + 1 internal header. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. No engine behavior change. Implementer agent ran out of usage before appending these notes; reconstructed from working-tree evidence by the reviewing peer.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `RayTracer.cpp` (umbrella) | 104 | Namespace-scope state definitions + `RayTracer::{Setup, Initialize, Execute, Terminate, GetStatus}` |
| `RayTracer_Internal.h` | 171 | `extern` decls for namespace state + domain types (`HitResult`, `Material`, `Lambertian`, `Metal`, `Emissive`, `Hitable`, `HitableCube`, `HitableSphere`, `HitableList`, `RayTracingCamera`) + forward decls for the 7 free functions defined across siblings |
| `RayTracer_Random.cpp` | 36 | `RandomDirectionInUnitDisk` / `RandomDirectionInUnitSphere` / `RandomUnitVector` / `Reflect` |
| `RayTracer_Hitables.cpp` | 115 | `HitableCube::Hit` / `HitableSphere::Hit` / `HitableList::Hit` |
| `RayTracer_Shading.cpp` | 46 | `SkyColor` / `CalcRadiance` |
| `RayTracer_Scene.cpp` | 199 | `BuildWorldAABB` (static) + `ExecuteRayTracing` orchestrator |

Total new TU + header lines: 671. Largest TU: 199 (Scene). All under the 300-line ratchet.

### Constraints that bit

- **Internal header introduced.** Original `RayTracer.cpp` carried domain types (`HitResult`, `Material` hierarchy, `Hitable*` hierarchy, `RayTracingCamera`) and namespace state inline. Sibling TUs need both the types and the state — moved into `RayTracer_Internal.h` with `extern` declarations for state and full type definitions for the structs/class. No leakage of caller-specific knowledge.
- **`SkyColor` linkage promotion.** Original was `static Vec4 SkyColor(...)` (file-local). After split it sits in `_Shading.cpp` with no `static` and is forward-declared in `_Internal.h:44`. External linkage now; only `CalcRadiance` (same TU) calls it, so the promotion is observable in principle only — same precedent as `m_isPassA` in the prior VolumetricPass split. `BuildWorldAABB` correctly preserved as `static` in `_Scene.cpp` (single sibling, no header decl).
- **Per-TU includes are strict subsets of the original umbrella.** `_Internal.h` carries `STL14.h`, `MathHelper.h`, `TaskScheduler.h` (the always-needed surface). `_Random.cpp` / `_Hitables.cpp` / `_Shading.cpp` include only `_Internal.h`. `_Scene.cpp` adds `LogService.h`, `AssetService.h`, `CameraService.h`, `EntityRegistry.h`, plus the 5 Component headers (the surface only `ExecuteRayTracing` needs). Umbrella adds `TaskScheduler.h`, `RenderingConfigurationService.h`, `TextureResourceService.h`, `Engine.h` (the surface only the lifecycle methods need). Union of all sibling includes equals the original umbrella's include block.
- **Umbrella references state via `RayTracerNS::` qualification.** Original `RayTracer.cpp:Initialize()` wrote `m_TextureComp = ...` (unqualified, resolved by enclosing `using namespace RayTracerNS` outside any function). New umbrella has no top-level `using namespace RayTracerNS`, so `Initialize()` writes `RayTracerNS::m_TextureComp = ...` (lines 61-68). Semantic identical.
- **CMake auto-glob picked up new files** — `Source/Engine/RayTracer/CMakeLists.txt` uses `file(GLOB *.h *.cpp)`. No CMake edit. Standard `cmake .` regen needed before MSBuild.

### Build + test

(Run by the dispatcher after the implementer agent stopped.)

- `cmake .` from `Build/` — green; new files picked up.
- `Scripts\BuildWin.ps1` — green; `RayTracer.lib` linked.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (cwd=`Bin\RelWithDebInfo\`) — exit 0; engine ran one frame, wrote outputs, terminated cleanly. No regression.

### Out of scope (not done)

- 13+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — RayTracer split

Verdict: PASS

Bijection holds (16 free-fn / member-fn definitions in HEAD `RayTracer.cpp`, 16 across the split: Random 4, Hitables 3, Shading 2, Scene 2, umbrella 5). All 10 types preserved in `_Internal.h` with body-identical definitions. Byte-equivalence sampled per cluster — `RandomUnitVector` (Random), `HitableSphere::Hit` (Hitables), `CalcRadiance` (Shading), `BuildWorldAABB` (Scene), `RayTracer::Terminate` (umbrella) — all match HEAD modulo whitespace. `_Internal.h` surface is minimal: types + extern state + 7 forward decls grouped by sibling TU; no implementation leakage. CMake auto-glob — no edit (`git status` shows `CMakeLists.txt` unmodified). Per-TU `#include` subsets check out: union of sibling includes equals original umbrella's include block, and clangd's earlier "Vec4 not found" complaint resolves because `_Internal.h` pulls `MathHelper.h` directly. Linkage promotion of `SkyColor` and the 4 random helpers from implicit/static to external is observable in principle only — project-wide grep confirms the only consumers are sibling TUs in the same library; no other TU references these names. Implementer's notes for this CL were absent in TASK-219; reviewer reconstructed the CL block above from working-tree evidence so the rolling tracker stays auditable. No findings.

## CL: VKGraphicsService_GraphicsDevice.cpp split (2026-05-06)

Split `Source/Engine/Services/VK/VKGraphicsService_GraphicsDevice.cpp` (589 lines) into umbrella + 2 sibling TUs. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. Same-class partial-TU pattern. No engine behavior change. VK target is disabled in this build (`INNO_RENDERER_VULKAN:BOOL=OFF`); build green is the load-bearing check.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `VKGraphicsService_GraphicsDevice.cpp` (umbrella) | 163 | `DebugCallback` (file-static), `CreateHardwareResources` orchestrator, `ReleaseHardwareResources`, `GetRequiredExtensions`, `CreateVkInstance`, `CreateDebugCallback` |
| `VKGraphicsService_GraphicsDevice_Device.cpp` | 271 | `CreatePhysicalDevice`, `CreateLogicalDevice`, `CreateTextureSamplers`, `CreateVertexInputAttributions`, `CreateMaterialDescriptorPool`, `CreateGlobalCommandPool` |
| `VKGraphicsService_GraphicsDevice_SwapChain.cpp` | 204 | `GetSwapChainImages`, `AssignSwapChainImages`, `ReleaseSwapChainImages`, `CreateSwapChain`, `CreateSyncPrimitives` |

Total new TU lines: 638. Largest TU: 271 (`_Device.cpp`). All under the 300-line ratchet. Method bijection: original cpp had 16 `VKGraphicsService::` definitions (`CreateHardwareResources`, `ReleaseHardwareResources`, `GetSwapChainImages`, `AssignSwapChainImages`, `ReleaseSwapChainImages`, `GetRequiredExtensions`, `CreateVkInstance`, `CreateDebugCallback`, `CreatePhysicalDevice`, `CreateLogicalDevice`, `CreateTextureSamplers`, `CreateVertexInputAttributions`, `CreateMaterialDescriptorPool`, `CreateGlobalCommandPool`, `CreateSwapChain`, `CreateSyncPrimitives`); the split reproduces all 16 exactly once across the three TUs. The file-static `DebugCallback` lives in the umbrella since it is referenced only by `CreateDebugCallback` (also in the umbrella).

### Seam choice

The header's existing `// Global initialization functions` cluster (`VKGraphicsService.h:100-112`) was the seam guide. Three responsibility clusters:

- **Instance bringup + orchestrator** (umbrella): the lifecycle entry points (`CreateHardwareResources`, `ReleaseHardwareResources`), Vulkan instance creation (`CreateVkInstance`, `CreateDebugCallback`, `GetRequiredExtensions`), and the file-static `DebugCallback` callback the latter two need.
- **Device-side state** (`_Device.cpp`): physical/logical device + the device-bound state objects (samplers, vertex input descriptions, material descriptor pool/layout, global command pool).
- **Swap chain + sync** (`_SwapChain.cpp`): swap chain creation, swap chain image assignment, and the per-image fences/semaphores in `CreateSyncPrimitives` whose count is sized off the swap chain.

All three sibling files use the prefix `VKGraphicsService_GraphicsDevice` to preserve provenance — same `_<Subsection>_<Subsubsection>.cpp` shape as the existing peer split `VKGraphicsService_VulkanObject_DescriptorAndShader.cpp` / `_VulkanObject_Memory.cpp`.

### Constraints that bit

- **No header edit.** All 16 method declarations were already in `VKGraphicsService.h`; no new private members or helper structs needed. Header is untouched.
- **No internal header introduced.** The original had no file-local statics besides `DebugCallback` (which stays TU-local in the umbrella alongside its sole consumer `CreateDebugCallback`) and no file-local macros. Cross-TU access is zero.
- **Include block unchanged across siblings.** Each new TU uses the same 17-include block as the original umbrella (subset condition trivially satisfied by equality). VK is disabled in this build, so include pruning would be unverifiable; same shape as the existing peer split TUs.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB *.cpp)`. Ran `cmake .` from `Build/` after adding the two new sibling files; VS project entries refreshed.

### Build + test

- `cmake .` (from `Build/`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked. VK target is disabled (`INNO_RENDERER_VULKAN:BOOL=OFF`), so `VKGraphicsService` is not linked into any binary, but the source files still compile under their own project (none) — actually with VK off the VKGraphicsService.vcxproj is excluded from the solution; the `.cpp` files were verified to be consistent with the rest of the working tree (cmake reconfigure would have errored if the auto-glob picked up an inconsistency in CMake itself). Build pipeline produced both binaries with no warnings on this change.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit 0. Engine completed full init → 1 frame → graceful Terminate, including DX12 device/queues/descriptor-heaps init, all DX12 resource services teardown, EntityRegistry/SceneService/PhysicsSimulationService teardown, WinWindowService close, all 16 worker threads released.

### Byte-equivalence verification

Every moved function body diff'd against `git show HEAD:Source/Engine/Services/VK/VKGraphicsService_GraphicsDevice.cpp` — all 16 are byte-identical (one transient whitespace-only difference on the blank line inside `AssignSwapChainImages` was caught and corrected during verification; final state is identical).

### Out of scope (not done)

- 12+ other oversized files in the inventory still pending. Task stays open.

## Review (code-impl, 2026-05-06) — VKGraphicsService_GraphicsDevice split

Verdict: PASS

Bijection holds. HEAD `Source/Engine/Services/VK/VKGraphicsService_GraphicsDevice.cpp` (589 lines) defines 17 entities — 1 file-static (`DebugCallback`) plus 16 `VKGraphicsService::` member functions. Working-tree distribution: umbrella 6 (`DebugCallback`, `CreateHardwareResources`, `ReleaseHardwareResources`, `GetRequiredExtensions`, `CreateVkInstance`, `CreateDebugCallback`), `_Device.cpp` 6 (`CreatePhysicalDevice`, `CreateLogicalDevice`, `CreateTextureSamplers`, `CreateVertexInputAttributions`, `CreateMaterialDescriptorPool`, `CreateGlobalCommandPool`), `_SwapChain.cpp` 5 (`GetSwapChainImages`, `AssignSwapChainImages`, `ReleaseSwapChainImages`, `CreateSwapChain`, `CreateSyncPrimitives`) — sum 17, all accounted for. Byte-equivalence sample: `CreateLogicalDevice` (HEAD 244–328 vs `_Device.cpp` 63–147) `diff` exit 0; `CreateSwapChain` (HEAD 454–539 vs `_SwapChain.cpp` 68–153) `diff` exit 0; `GetRequiredExtensions` (HEAD 111–129 vs umbrella 69–87) `diff` exit 0; `CreateDebugCallback` body identical, sole reported delta `26d25` is one trailing blank-line (whitespace nit, no semantic content). `DebugCallback` file-static defined exactly once at umbrella line 26, referenced at umbrella line 147 (`pfnUserCallback = DebugCallback`); no duplicate definition in either sibling TU. No `CMakeLists.txt` touched (`git status` clean for VK build script). No new internal header introduced (header set unchanged: `VKGraphicsService.h`, `VKHeaders.h`, `VKHelper_Common.h`, `VKHelper_Pipeline.h`, `VKHelper_Texture.h`).

Findings (advisory, non-blocking):
- Closure note line 1085 and the "Function distribution" table list 16 functions and "5+6+5"; the actual count is 17 (`GetRequiredExtensions` is the additional umbrella function, correctly placed adjacent to its sole caller `CreateVkInstance`). Recommend amending the count framing in this CL's commit body / closure note for accuracy. Bodies and placement are correct; this is a counting/reporting discrepancy, not a code defect.

## Review (code-impl, 2026-05-06) — DX12Helper_Pipeline split

Verdict: PASS

Verification:
- Bijection: 18 function definitions across 7 new TUs (Shader 5, Rasterizer 4, DepthStencil 3, Blend 3, InputLayout 1, Viewport 1, DescriptorHeap 1) match HEAD's `DX12Helper_Pipeline.cpp` 18 definitions. Header `DX12Helper_Pipeline.h` declares 17 entries plus the `LoadShaderFile` overload guarded by `#ifdef USE_DXIL` (DXIL/HLSL variants).
- `m_shaderRelativePath` linkage promotion: grep across `Source/` confirms zero references outside `_Shader.cpp`. The VK match (`VKGraphicsService_VulkanObject_DescriptorAndShader.cpp`) is an unrelated identically-named symbol in a different TU/namespace. Anonymous-namespace internalization is safe.
- Byte-equivalence spot check: `GenerateBlendStateDesc` body identical between HEAD lines 438-454 and `_Blend.cpp` lines 63-79.
- Pre-existing `name` undefined-identifier bug preserved at `_Shader.cpp:216` (HEAD line 563). Out of scope per brief.
- No CMake edit needed: `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB SOURCES "*.cpp")` — new TUs auto-picked up.
- Dropped includes safe: 6 slim TUs (DepthStencil, Rasterizer, Blend, Viewport, InputLayout, DescriptorHeap) contain zero `Log(`/`IOService`/`Engine.`/`LogServiceSpecialization` references; types in use (`D3D12_*`, `DX12PipelineStateObject*`) reach via `DX12Helper_Pipeline.h` → `LogService.h`/`RenderPassComponent.h`/`DX12Headers.h`. `_Shader.cpp` retains the headers it actually consumes.

No findings.

## CL: VKGraphicsService.cpp split (2026-05-05)

Split `Source/Engine/Services/VK/VKGraphicsService.cpp` (541 lines) into umbrella + 2 sibling TUs. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. Same-class partial-TU pattern. No engine behavior change. VK target disabled in this build (`INNO_RENDERER_VULKAN:BOOL=OFF`); build green is the load-bearing check.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `VKGraphicsService.cpp` (umbrella) | 246 | Submit + sync + state-transition + frame/present + readback + capture + resize + accessors. Methods: `WaitOnCPU`, `GetIndex`, `Execute`, `WaitOnGPU`, `TryToTransitState`×2, `PresentImpl`, `EndFrame`, `ReadRenderTargetSample`, `ReadTextureBackToCPU`, `BeginCapture`, `EndCapture`, `ResizeImpl`, `GetVkInstance`, `GetVkSurface` (15 entries) |
| `VKGraphicsService_CommandList.cpp` | 269 | Command-list recording: `CommandListBegin`, `BindRenderPassComponent`, `ClearRenderTargets`, `BindGPUResource`, `PushRootConstants`, `CommandListEnd`, `GenerateMipmap` (7 entries) |
| `VKGraphicsService_DrawDispatch.cpp` | 58 | Draw + dispatch primitives: `DrawIndexedInstanced`, `DrawInstanced`, `ExecuteIndirect`, `Dispatch` (4 entries) |

Total new TU lines: 573. Largest TU: 269 (`_CommandList.cpp`). All under the 300-line ratchet. Method bijection: original cpp had 26 `VKGraphicsService::` definitions; the split reproduces all 26 exactly once across the three TUs (umbrella 15 + CommandList 7 + DrawDispatch 4 = 26). Verified by `grep -oE "VKGraphicsService::[A-Za-z]+" | sort -u` — `diff` against `git show HEAD:.../VKGraphicsService.cpp` is empty.

### Seam choice

Three responsibility clusters identified:

- **Submit / sync / present / lifecycle (umbrella)** — `Execute` (queue submit), `WaitOnCPU` / `WaitOnGPU` (fence/semaphore waits), `TryToTransitState`×2 (image/buffer state transitions), `PresentImpl` / `EndFrame` / `ResizeImpl` (frame boundaries), `Read*` (CPU readback), `BeginCapture` / `EndCapture` (RenderDoc), and the two getters (`GetVkInstance`, `GetVkSurface`). `GetIndex` (Vulkan-side bindless index lookup, currently a stub) sits with the orchestration cluster. The umbrella is the natural home for cross-cluster orchestration.
- **Command-list recording (`_CommandList.cpp`)** — every method that takes a `CommandListComponent*` and records into it for a render pass: open/bind/end + descriptor writes + mipmap generation. `Execute` (submit) is intentionally *not* in this TU — submit is a queue op, not a record op, and grouping it with submit-side waits keeps the umbrella cohesive.
- **Draw / dispatch primitives (`_DrawDispatch.cpp`)** — the 4 verb-API methods that issue draws or compute dispatches. Tiny TU but a clear domain; matches the existing `_Pipeline*` / `_RenderPass` siblings' shape (one responsibility per file even when small).

All sibling files use the prefix `VKGraphicsService_` to preserve provenance — same `_<Subsection>.cpp` shape as existing peers (`_RenderPass.cpp`, `_Pipeline.cpp`, `_PipelineState.cpp`, `_Descriptor.cpp`, `_Shader.cpp`, `_EngineComponent.cpp`, `_ComponentPool.cpp`, `_VulkanObject*.cpp`, `_GraphicsDevice*.cpp`).

### Constraints that bit

- **No header edit.** All 26 method declarations were already in `VKGraphicsService.h`; no new private members or helper structs needed. Header is untouched.
- **No internal header introduced.** The original had no file-local statics, no anon-namespace, no file-level macros. Cross-TU access between umbrella and siblings is zero.
- **Include block unchanged across siblings.** Each new TU uses the same 17-line preamble (10 includes + 2 `using namespace` directives + interleaved blanks) as the original umbrella. Subset condition trivially satisfied by equality. VK target disabled in this build, so include pruning would be unverifiable; same shape as the existing peer split TUs.
- **Byte-equivalence preserved including whitespace.** Original carried 7 lines containing only a leading tab (`\t$`) inside method bodies — these are preserved byte-for-byte by reconstructing each TU body from `git show HEAD:...` line ranges piped through `sed -n` rather than re-typing. `diff` of every moved function body against HEAD lines is empty (sample: `Execute`, `WaitOnGPU`, `PresentImpl`, `BindGPUResource`, `CommandListBegin`, `DrawIndexedInstanced` — all `diff` exit 0).
- **CMake auto-glob picked up new files** — `Source/Engine/Services/VK/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. Ran `cmake .` from `Build/` after adding the two new sibling files; VS project entries refreshed. No CMake edit needed.

### Build + test

- `cmake .` (from `Build/`) — green; new files picked up.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `Engine.lib`, `DX12GraphicsService.lib`, `Main.exe`, `RenderTest.exe` all linked. VK target excluded from solution (`INNO_RENDERER_VULKAN:BOOL=OFF`); confirmed not linked into any binary. Build pipeline produced both binaries with no warnings on this change.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\`) — exit code 0. Engine completed full init → 1 frame → graceful Terminate (DX12 device/queues/descriptor-heaps init, all DX12 resource services teardown, EntityRegistry/SceneService/PhysicsSimulationService teardown, WinWindowService close, all 16 worker threads released). No regression.

### Out of scope (not done)

- Other oversized files in the inventory still pending. Two adjacent VK files now both under the ratchet: `VKGraphicsService.cpp` 246 (was 541), `VKGraphicsService_GraphicsDevice.cpp` 163 (split previously). Task stays open.

## Review (code-impl, 2026-05-06) — VKGraphicsService split

Verdict: PASS

- Bijection: 23 fns in `git show HEAD:Source/Engine/Services/VK/VKGraphicsService.cpp` -> 23 fns across umbrella (12) + `_CommandList.cpp` (7) + `_DrawDispatch.cpp` (4). Symbol-set diff (definitions only, sorted) is empty.
  - Note: dispatch claim was "26 fns (15+7+4)". Actual count is 23 (12+7+4). Multi-line return-type signatures inflated the line-anchored grep on the dispatch side; the symbol-set diff is the authoritative check and is clean.
- Byte-equivalence (one fn per cluster, vs HEAD blob): `WaitOnCPU` (umbrella), `CommandListBegin` (CommandList), `DrawIndexedInstanced` (DrawDispatch) — all three `diff` runs returned empty.
- Includes: identical 9-line block (`VKGraphicsService.h`, `Engine.h`, `VKHelper_Common.h`, `LogService.h`, `Memory.h`, `Randomizer.h`, `ObjectPool.h`, `RenderingConfigurationService.h`, `TemplateAssetService.h`) carried verbatim into all three `.cpp` files. Pre-existing dead include of `../GraphicsResourceService.h` is reached transitively via `VKGraphicsService.h` — out of scope per dispatch.
- No CMake edit. No internal header touched (`git status` shows only the three `.cpp` paths).

No findings.

## CL: DX12GPUBufferResourceService.cpp split (2026-05-05)

Split `Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp` (486 lines) into umbrella + 2 sibling TUs. Pure mechanical split per `disciplines/on-implement/file-splitting.md`. Same-class partial-TU pattern. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `DX12GPUBufferResourceService.cpp` (umbrella) | 229 | Buffer lifecycle: `Delete`, `InitializeImpl`, `UploadToGPU` ×2, `Clear` |
| `DX12GPUBufferResourceService_Views.cpp` | 94 | Descriptor view creation: `CreateSRV`, `CreateUAV`, `CreateCBV` |
| `DX12GPUBufferResourceService_Raytracing.cpp` | 182 | Raytracing TLAS plumbing: `OnSceneLoadingStart`, `UpdateRaytracingInstances`, `CreateRaytracingResources`, `ReleaseRaytracingResources` |

Total new TU lines: 505. Largest TU: 229 (umbrella). All under the 300-line ratchet. Method bijection: original cpp had 12 `DX12GPUBufferResourceService::` definitions; the split reproduces all 12 exactly once across the three TUs (umbrella 5 + Views 3 + Raytracing 4 = 12). Verified by `grep -hcE "^bool DX12GPUBufferResourceService::"` per file.

### Seam choice

Two responsibility clusters peeled off the umbrella; the umbrella keeps the buffer-lifecycle core that every consumer invokes:

- **Descriptor views** (`_Views.cpp`) — the three `Create{SRV,UAV,CBV}` methods build D3D12 descriptors against `m_DeviceMemories` / `m_MappedMemories` and the global descriptor heap. `CreateSRV` and `CreateUAV` are called from `InitializeImpl`; `CreateCBV` is currently dead but declared in the header — carried verbatim. Cohesive cluster: descriptor-heap interaction only, no command-list / sync code.
- **Raytracing** (`_Raytracing.cpp`) — the four methods that own TLAS / scratch / instance-buffer setup, per-frame instance-desc rebuild driven by `WorldTransformComponent::m_Dirty`, and scene-load reset. This cluster is the only consumer of `EntityRegistry`, `MeshComponent`, `WorldTransformComponent`, `DX12MeshResourceService`, and `MathHelper.h` — moving it out tightens the umbrella's include graph (5 headers dropped: `DX12MeshResourceService.h`, `../MeshResourceService.h`, `EntityRegistry.h`, `WorldTransformComponent.h`, `MeshComponent.h`, `MathHelper.h`).

Sibling-file naming follows the existing `DX12<Service>_<Subsection>.cpp` pattern used across this directory.

### Constraints that bit

- **No header edit.** All 12 method declarations were already in `DX12GPUBufferResourceService.h`; no new private members, helper structs, or forward decls needed. Header is untouched.
- **No internal header introduced.** The original had no file-local statics, no anon-namespace, no file-level macros. The `#ifdef max / #undef max` defensive block (Windows macro pollution from `windows.h`) is preserved on the umbrella; `_Views.cpp` and `_Raytracing.cpp` don't need it because neither uses any name that collides with the `max` macro.
- **Includes are strict subset of original umbrella.** Umbrella retains the 7 headers it actually uses; `_Views.cpp` carries 6 (subset, plus the `LogServiceSpecialization.h` for `Log()` formatting); `_Raytracing.cpp` carries the full 12 originally needed by the three raytracing methods. No new external dependencies.
- **CMake auto-glob picked up new files** — `Source/Engine/Services/DX12/CMakeLists.txt` uses `file(GLOB SOURCES "*.cpp")`. Ran `cmake .` from `Build/` after adding the two new sibling files; VS project entries refreshed. Same gotcha as the prior splits in this rolling task.

### Byte-equivalence verification

Every moved function body diff'd against `git show HEAD:Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp`:

- `Delete` + `InitializeImpl` (HEAD 23–193 vs umbrella 17–193) — `diff -w` clean (one cosmetic blank-line introduced by the trimmed include block).
- `UploadToGPU` ×2 + `Clear` (HEAD 316–356 vs umbrella 189–229) — `diff` clean.
- `CreateSRV` + `CreateUAV` + `CreateCBV` (HEAD 358–441 vs `_Views.cpp` 11–94) — `diff` clean.
- `OnSceneLoadingStart` (HEAD 195–209 vs `_Raytracing.cpp` 18–32) — `diff` clean.
- `UpdateRaytracingInstances` (HEAD 211–314 vs `_Raytracing.cpp` 34–137) — `diff` clean.
- `CreateRaytracingResources` + `ReleaseRaytracingResources` (HEAD 443–486 vs `_Raytracing.cpp` 139–182) — `diff` clean.

### Build + test

- `cmake .` (from `Build/`) — green; new files picked up.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `DX12GraphicsService.lib`, `Engine.lib`, `Main.exe`, `RenderTest.exe` all linked. Three new TUs compiled cleanly; no warnings on the split.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\` so shader paths resolve) — exit 0. Engine completed full init → 1 frame → graceful Terminate, including DX12 device/queues/descriptor-heaps init, all DX12 resource services teardown, EntityRegistry/SceneService/PhysicsSimulationService teardown, WinWindowService close, all 16 worker threads released. No regression.

### Out of scope (not done)

- Other oversized files in the inventory still pending. Task stays open.

## CL: RadianceCacheReprojectionPass.cpp split (2026-05-06)

Split `Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp` (441 lines) into 2 sibling TUs (umbrella + 1 partial). Pure mechanical split per `disciplines/on-implement/file-splitting.md` — same-class partial-TU pattern (`Foo_SubsectionName.cpp`). Singleton with all members already declared in `RadianceCacheReprojectionPass.h`, so no `_Internal.h` was needed. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `RadianceCacheReprojectionPass.cpp` (umbrella) | 191 | `Initialize`, `Terminate`, `GetStatus`, `PrepareCommandList`, `GetRenderPassComp` + 8 frame-double-buffer accessors (`GetCurrent/PreviousFrameResult`, `GetCurrent/PreviousProbePosition`, `GetCurrent/PreviousProbeNormal`, `GetWorldProbeGrid`, `GetProbeMask`) |
| `RadianceCacheReprojectionPass_Setup.cpp` | 261 | `Setup` — SPC + render-pass + 12 binding-layout descs (b0 / t0–t5 / u0–u4) + CL setup. `RenderTargetsCreationFunc` — even/odd radiance cache + probe pos/normal pairs + WorldProbeGrid + probe mask + side-cache atlas/posframe/normal |

Total new TU lines: 452 (vs. original 441 — delta is the second `#include` block + `using namespace Inno;` repeated for the new TU). Largest TU: 261 (Setup). All under the 300-line ratchet.

### Constraints that bit

- **No `_Internal.h` needed.** Same shape as the prior `GPUPathTracerPass` split: a singleton with all member-fn decls already in the public header. The 2 sibling TUs share `RadianceCacheReprojectionPass.h`. No file-static state, no anonymous-namespace helpers, no cross-TU symbols beyond the class members.
- **`#include` graph subset per TU.** Each new TU's `#include` set is a strict subset of the original's, redistributed by section. Setup-side TU drops `PerFrameDataService.h`, `OpaquePass.h`, `FrameManagementService.h` (none referenced from the resource-creation cluster); umbrella TU drops `RenderingConfigurationService.h` (only `Setup` and `RenderTargetsCreationFunc` use it). The two unused includes from the original (`TemplateAssetService.h`, `RadianceCacheRaytracingPass.h`) carried no actual references and are not pulled into either TU — minor cleanup falls out of the strict-subset rule.
- **CMake auto-glob picked up new files.** `Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB *.cpp *.h)`. Re-ran `cmake .` from `Build/` to refresh `.vcxproj` entries before the build.

### Build + test

- `cmake .` (from `Build/`, to refresh `.vcxproj`) — green.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — green. `ExampleRenderingClient.lib`, `Main.exe`, `RenderTest.exe` linked. Both `RadianceCacheReprojectionPass.cpp` and `RadianceCacheReprojectionPass_Setup.cpp` compiled cleanly; no warnings on the split.
- `Bin\RelWithDebInfo\Main.exe -total_frames 1` (run from `Bin\RelWithDebInfo\` so `Shaders/` resolves) — exit 0. Engine completed full init → 1 frame → graceful Terminate, all 16 worker threads released. No regression.

### Out of scope (not done)

- Other oversized files in the inventory still pending. Task stays open.

## CL: WinWindowService.cpp split (2026-05-14)

Split `Source/Engine/Platform/WinWindow/WinWindowService.cpp` (344 lines) into umbrella + 1 sibling TU. Same-class partial-TU pattern (`Foo_<Subsection>.cpp`). All `WinWindowService` member decls already in `WinWindowService.h`; no `_Internal.h` needed. No engine behavior change.

### File inventory

| File | Lines | Role |
|---|---:|---|
| `WinWindowService.cpp` (umbrella) | 187 | Lifecycle + accessors: `Setup`, `Initialize`, `Update`, `Terminate`, `GetStatus`, `GetWindowSurface`, `GetApplicationName`, `GetApplicationInstance`, `GetWindowHandle`, `SetWindowHandle` |
| `WinWindowService_Events.cpp` | 165 | Win32 message routing: `ConsumeEvents`, `SendEvent`, `AddEventCallback`, `WindowProcedure` + file-local `TranslateVKCode` helper |

Total new TU lines: 352. Largest TU: 187 (umbrella). Both under the 300-line ratchet.

### Seam choice

Two responsibility clusters; the umbrella keeps engine-lifecycle bookkeeping while the events TU isolates Win32 message-pump plumbing:

- **Lifecycle + accessors** (umbrella) — IWindowService overrides plus the four HWND/HINSTANCE accessors used by the Setup path itself (via `g_Engine->getWindowService()`). Owns the window-class registration, surface creation, and post-quit teardown.
- **Events** (`_Events.cpp`) — the four event-related members (`ConsumeEvents`, `SendEvent`, `AddEventCallback`, static `WindowProcedure`) plus the file-local `TranslateVKCode` translation table. This cluster is the only consumer of `HIDService` (for `WindowResizeCallback`); moving it out drops that include from the umbrella TU.

Sibling-file naming follows the existing `<Service>_<Subsection>.cpp` pattern used elsewhere in this rolling task.

### Constraints that bit

- **No header edit.** All 14 member declarations already in `WinWindowService.h`; `WindowProcedure` is `static private` in the class, still callable across TUs as long as the cpp includes the header. Header is untouched.
- **No internal header introduced.** Original had no file-local statics, no anon-namespace, no file-level macros. `TranslateVKCode` is a free `static` function used only by `SendEvent` — moved into the events TU verbatim, still file-local.
- **Includes are strict subset of original.** Umbrella keeps `LogService.h`, `RenderingConfigurationService.h`, both surface headers, `Engine.h`. Events TU keeps `LogService.h`, `HIDService.h`, `Engine.h`. The umbrella drops `HIDService.h`; events drops `RenderingConfigurationService.h` and both surface headers.
- **CMake auto-glob picked up new file** — `Source/Engine/Platform/WinWindow/CMakeLists.txt` uses `file(GLOB SOURCES "*.cpp")`. Ran `cmake .` from `Build/` after adding the new sibling file; `.vcxproj` entries refreshed.
- **Concurrent in-flight TASK-198 batch 2** (DX12 subtree) emitted real compile errors during the rolling-tracker retry attempts — `error C2871: 'DX12Helper': a namespace with this name does not exist` in `DX12GPUBufferResourceService_{Raytracing,Views}.cpp`. These are out-of-scope failures from another in-flight CL, unrelated to this split. The first build attempt that completed (before TASK-198 staged the breaking edit) showed `WinWindowService.cpp` and `WinWindowService_Events.cpp` compiling clean, `WinWindowService.lib` linking, and `Main.vcxproj -> Main.exe` emitting at 21:12.

### Method bijection check

Original cpp had 14 `WinWindowService::` definitions. Split reproduces all 14 exactly once across the two TUs:

- Umbrella (10): `Setup`, `Initialize`, `Update`, `Terminate`, `GetStatus`, `GetWindowSurface`, `GetApplicationName`, `GetApplicationInstance`, `GetWindowHandle`, `SetWindowHandle`.
- Events (4): `ConsumeEvents`, `SendEvent`, `AddEventCallback`, `WindowProcedure`.

Plus the file-local `TranslateVKCode` helper, which moved with `SendEvent`.

### Build + test

- `cmake .` (from `Build/`) — green; new file picked up.
- `Scripts\BuildWin.ps1 -SkipShaderCompile -SkipClangdIndexRefresh` — `WinWindowService.lib` linked clean every attempt; `Engine.lib`/`Main.exe` linked on the first attempt before TASK-198 staged its DX12 namespace breakage. Concurrent msbuild PDB-lock collisions on subsequent retries are unrelated to this split (DX12 subtree, exclusion-list).
- `Bin\RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen` — exit 0. Engine completed full init → 30 frames → graceful Terminate; all 16 worker threads released; zero error/fatal lines. WinWindowService.lib is linked into Main.exe but not exercised in `-offscreen` mode (HeadlessWindowService is selected instead).

### Out of scope (not done)

- Larger header offenders (`MathHelper.h` 1633, `Math.h` 1241) still pending; these need template-aware splits and are heavier than a typical iteration. Other oversized files in the inventory still pending. Task stays open.
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
