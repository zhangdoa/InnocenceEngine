# Rendering Client Audit Implementation Plan

> **Path note (TASK-200, 2026-04-29):** Any `Res/Shaders/HLSL/` references in this plan are wrong. Current shader source lives in `Source/Shaders/HLSL/`; the `Res/Shaders/` tree is an obsolete shadow that has been deleted.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Audit all 17 active render passes in DefaultRenderingClient for D3D12 correctness and visual validity, fixing each before advancing.

**Architecture:** Add a one-shot `-audit` flag to the engine that triggers `AuditDump()` inside `DefaultRenderingClientImpl` on frame 5. Each dump calls `ReadTextureBackToCPU` on each pass's primary render target and saves as HDR to `Bin/`. Claude reads the images and identifies broken passes. Fix cycle repeats per pass until the full pipeline produces correct output.

**Tech Stack:** C++17, Direct3D 12, engine `ReadTextureBackToCPU` + `AssetService::Save`, existing HDR save path (Float32 override pattern established in TestRenderingClient).

---

## File Map

| File | Change |
|------|--------|
| `Source/Engine/Engine.h` | Add `bool isAudit = false;` to `InitConfig` |
| `Source/Engine/Engine.cpp` | Parse `-audit` flag in `ParseInitConfig` |
| `Source/DefaultClient/RenderingClient/DefaultRenderingClient.h` | Declare `AuditDump()` on impl class |
| `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp` | Implement `AuditDump()`; call from `ExecuteCommands` |

Per-pass fix files are unknown until Task 3 identifies them.

---

## Task 1 — Add `-audit` flag to the engine

**Files:**
- Modify: `Source/Engine/Engine.h`
- Modify: `Source/Engine/Engine.cpp`

- [ ] **1.1** In `Engine.h`, add `isAudit` to `InitConfig`:

```cpp
struct InitConfig
{
    EngineMode engineMode = EngineMode::Host;
    GraphicsService graphicsService = GraphicsService::DX12;
    LogLevel logLevel = LogLevel::Success;
    bool isHeadless = false;
    bool isOffscreen = false;
    bool isAudit = false;   // <-- add this line
    char testCase[64] = {};
};
```

- [ ] **1.2** In `Engine.cpp`, inside `ParseInitConfig`, locate where `offscreen` is parsed (uses `args.find("offscreen")`). Add the audit flag parsing immediately after:

```cpp
if (args.find("audit") != std::string::npos)
{
    initConfig.isAudit = true;
    Log(Success, "Audit mode: will dump all pass outputs on frame 5.");
}
```

- [ ] **1.3** Build and verify it compiles:
```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```
Expected: zero errors.

- [ ] **1.4** Commit:
```
git add Source/Engine/Engine.h Source/Engine/Engine.cpp
git commit -m "feat: add -audit flag to InitConfig for pass output dumping"
```

---

## Task 2 — Add `AuditDump()` to DefaultRenderingClient

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.h`
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp`

- [ ] **2.1** Read the full `DefaultRenderingClient.cpp` to understand:
  - Where `ExecuteCommands` is defined on `DefaultRenderingClientImpl`
  - What member variables track frame count (search for `m_Execute`, `m_Frame`, counter variables)
  - Which pass singletons are already included at the top

- [ ] **2.2** Add `AuditDump()` declaration to the private impl class in `DefaultRenderingClient.cpp` (the impl class body, not the public header — the impl is defined in the .cpp file itself).

- [ ] **2.3** Add includes needed by `AuditDump()` at the top of `DefaultRenderingClient.cpp` if not already present:

```cpp
#include "../../Engine/Services/IGraphicsService.h"
#include "../../Engine/Services/AssetService.h"
// Individual pass headers — check which are already included:
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "FinalBlendPass.h"
```

- [ ] **2.4** Implement `AuditDump()`. Add this function body to `DefaultRenderingClient.cpp`:

```cpp
void DefaultRenderingClientImpl::AuditDump()
{
    auto l_rs = g_Engine->getGraphicsService();

    // Wait for all GPU work to complete before reading back.
    l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Compute),  GPUEngineType::Compute);

    auto SaveRT = [&](const char* filename, RenderPassComponent* rp, uint32_t colorIndex = 0)
    {
        if (!rp || !rp->m_OutputMergerTarget) { Log(Warning, "AuditDump: null RenderPassComp for ", filename); return; }
        auto* l_rt = rp->m_OutputMergerTarget->m_ColorOutputs[colorIndex];
        if (!l_rt) { Log(Warning, "AuditDump: null color output [", colorIndex, "] for ", filename); return; }
        auto l_pixels = l_rs->ReadTextureBackToCPU(rp, l_rt);
        if (l_pixels.empty()) { Log(Error, "AuditDump: empty readback for ", filename); return; }
        TextureDesc l_desc = l_rt->m_TextureDesc;
        l_desc.PixelDataType = TexturePixelDataType::Float32;
        g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
        Log(Success, "AuditDump: saved ", filename);
    };

    auto SaveTexture = [&](const char* filename, RenderPassComponent* rp, TextureComponent* tc)
    {
        if (!tc) { Log(Warning, "AuditDump: null TextureComponent for ", filename); return; }
        auto l_pixels = l_rs->ReadTextureBackToCPU(rp, tc);
        if (l_pixels.empty()) { Log(Error, "AuditDump: empty readback for ", filename); return; }
        TextureDesc l_desc = tc->m_TextureDesc;
        l_desc.PixelDataType = TexturePixelDataType::Float32;
        g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
        Log(Success, "AuditDump: saved ", filename);
    };

    // 1-2: BRDF LUT passes (compute — one-shot; outputs are textures, not RT color slots)
    // NOTE: If BRDFLUTPass/BRDFLUTMSPass expose their output via GetResult() returning
    // TextureComponent*, use SaveTexture. Read BRDFLUTPass.h/.cpp to verify getter name.
    // Placeholder — update after reading pass headers:
    // SaveTexture("audit_01_BRDFLUTPass.hdr",   BRDFLUTPass::Get().GetRenderPassComp(),   BRDFLUTPass::Get().GetResult());
    // SaveTexture("audit_02_BRDFLUTMSPass.hdr",  BRDFLUTMSPass::Get().GetRenderPassComp(), BRDFLUTMSPass::Get().GetResult());

    // 3: Shadow map
    SaveRT("audit_03_SunShadow_depth.hdr", SunShadowGeometryProcessPass::Get().GetRenderPassComp(), 0);

    // 4: Opaque G-buffer (multiple render targets)
    // Read OpaquePass.cpp to confirm which RT slot maps to albedo/normal/MRA.
    SaveRT("audit_04a_Opaque_RT0.hdr", OpaquePass::Get().GetRenderPassComp(), 0);
    SaveRT("audit_04b_Opaque_RT1.hdr", OpaquePass::Get().GetRenderPassComp(), 1);
    SaveRT("audit_04c_Opaque_RT2.hdr", OpaquePass::Get().GetRenderPassComp(), 2);

    // 5: SSAO (compute output — check SSAOPass.h for GetResult() returning TextureComponent*)
    // SaveTexture("audit_05_SSAO.hdr", SSAOPass::Get().GetRenderPassComp(), SSAOPass::Get().GetResult());

    // 8: Light pass outputs
    SaveTexture("audit_08a_Light_Luminance.hdr",    LightPass::Get().GetRenderPassComp(), LightPass::Get().GetLuminanceResult());
    SaveTexture("audit_08b_Light_Illuminance.hdr",  LightPass::Get().GetRenderPassComp(), LightPass::Get().GetIlluminanceResult());

    // 9: Sky
    SaveRT("audit_09_Sky.hdr", SkyPass::Get().GetRenderPassComp(), 0);

    // 10: TAA resolved
    SaveRT("audit_10_TAAPass.hdr", TAAPass::Get().GetRenderPassComp(), 0);

    // 11: Final blend (SDR output)
    SaveRT("audit_11_FinalBlend.hdr", FinalBlendPass::Get().GetRenderPassComp(), 0);

    Log(Success, "AuditDump complete — check Bin/*.hdr");
}
```

> **Note:** Several passes (BRDFLUTPass, BRDFLUTMSPass, SSAOPass, TiledFrustumGenerationPass, LightCullingPass) are compute-only and may not have `m_OutputMergerTarget`. Before building, read each of those pass .h/.cpp files to find their actual output getter (likely `GetResult()` returning `TextureComponent*` or `GPUResourceComponent*`). Fill in the commented placeholders accordingly.

- [ ] **2.5** In `DefaultRenderingClientImpl::ExecuteCommands`, add the audit trigger. Find the frame counter (likely `m_ExecuteOneShotCommands` or a similar frame tracking variable — read the function body). Add after the main execute block:

```cpp
if (g_Engine->getInitConfig().isAudit)
{
    static uint32_t s_AuditFrameCounter = 0;
    if (++s_AuditFrameCounter == 5)
    {
        AuditDump();
    }
}
```

- [ ] **2.6** Build:
```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```
Expected: zero errors.

- [ ] **2.7** Commit:
```
git add Source/DefaultClient/RenderingClient/DefaultRenderingClient.h \
        Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
git commit -m "feat: add AuditDump() to DefaultRenderingClient for pass-by-pass visual validation"
```

---

## Task 3 — First audit run: capture all outputs

- [ ] **3.1** Launch Main.exe with `-audit` and let it run until AuditDump fires, then close:
```
powershell.exe -NoProfile -NonInteractive -Command "
  Set-Location 'C:\GitRepo\InnocenceEngine\Bin'
  Start-Process -FilePath 'RelWithDebInfo\Main.exe' \
    -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -audit' \
    -Wait -NoNewWindow
"
```
Let it run for ~10 seconds (enough for 5 frames + dump), then terminate if it doesn't self-close.

- [ ] **3.2** List all generated HDR files:
```
ls C:/GitRepo/InnocenceEngine/Bin/*.hdr
```

- [ ] **3.3** Read each HDR file with the Read tool (Claude interprets images). For each dump:
  - Is the image all-black? → pass is producing nothing
  - Is it garbage/noise? → state mismatch or uninitialized resource
  - Is it visually meaningful? → pass is working

- [ ] **3.4** Capture the D3D12 debug log from the run. The log goes to stdout — capture with:
```
powershell.exe -NoProfile -NonInteractive -Command "
  Set-Location 'C:\GitRepo\InnocenceEngine\Bin'
  & 'RelWithDebInfo\Main.exe' '-mode 0 -renderer 0 -loglevel 0 -audit' 2>&1
" | Out-File C:/GitRepo/InnocenceEngine/Build/audit_run.txt
grep -E "(ERROR|WARNING|FENCE|ALLOCATOR)" C:/GitRepo/InnocenceEngine/Build/audit_run.txt
```

- [ ] **3.5** From the image inspection and D3D12 log, identify:
  - Which is the **first** pass with a broken output
  - Which passes are producing correct output
  - Which FENCE_ZERO_WAIT warnings correspond to which pass

Document findings before touching any code.

---

## Task 4+ — Per-pass fix cycle (repeat for each broken pass)

This task is a template. Instantiate one copy per broken pass found in Task 3, starting with the earliest broken pass in pipeline order.

**Before starting each fix cycle, apply `superpowers:systematic-debugging`.**

- [ ] **Fix.1** Read the broken pass's `.cpp` and its shader(s) in `Res/Shaders/HLSL/`
- [ ] **Fix.2** Form a hypothesis about the root cause (wrong resource state? uninitialized buffer? broken sync? shader bug?)
- [ ] **Fix.3** Implement the minimal fix
- [ ] **Fix.4** Build:
```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```
- [ ] **Fix.5** If shaders changed, recompile:
```
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"
```
- [ ] **Fix.6** Re-run audit and read the updated dump for this pass:
```
# (same launch command as Task 3.1)
```
Read the HDR image. Verify no D3D12 errors for this pass.
- [ ] **Fix.7** Once pass is clean: commit, then advance to the next pass in pipeline order.

---

## Pipeline Execution Order (audit sequence)

Work through passes in this order. Mark each ✅ when both GPU-clean and visually correct:

- [ ] 1. BRDFLUTPass
- [ ] 2. BRDFLUTMSPass
- [ ] 3. SunShadowGeometryProcessPass
- [ ] 4. OpaquePass (albedo, normal, MRA, depth)
- [ ] 5. SSAOPass
- [ ] 6. TiledFrustumGenerationPass
- [ ] 7. LightCullingPass
- [ ] 8. LightPass (luminance + illuminance)
- [ ] 9. SkyPass
- [ ] 10. PreTAAPass
- [ ] 11. TAAPass
- [ ] 12. PostTAAPass
- [ ] 13. LuminanceHistogramPass
- [ ] 14. LuminanceAveragePass
- [ ] 15. FinalBlendPass

**Definition of done:** FinalBlendPass produces a correctly lit, non-black, non-garbage final frame. No D3D12 errors or unexpected warnings from any active pass.
