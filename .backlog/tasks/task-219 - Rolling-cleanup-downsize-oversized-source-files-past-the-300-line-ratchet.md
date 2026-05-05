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
