# Unified Exposure & Tone Mapping Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Route both rasterized and path tracer HDR output through the same auto-exposure and tone mapping pipeline, eliminating the dedicated path tracer tone map pass.

**Architecture:** The path tracer's ToneMap pass is removed. `GPUPathTracerPass::GetResult()` returns the raw HDR accumulation buffer. `DefaultRenderingClient` routes this buffer (or the TAA result for rasterized) into the shared histogram -> average -> FinalBlend post-processing tail. The GPU execution blocks for the rasterized passes are guarded by `!m_GPUPathTracerActive`.

**Tech Stack:** HLSL compute shaders, DX12 command lists, C++17

---

### Task 1: Strip ToneMap from GPUPathTracerPass

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h`
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`
- Delete: `Source/Shaders/HLSL/GPUPathTracerToneMap.comp`

- [ ] **Step 1: Remove ToneMap members from the header**

In `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h`, remove these lines:

```cpp
// Remove these declarations:
CommandListComponent* GetToneMapCommandList();

// Tonemap compute pass
RenderPassComponent*    m_ToneMapRenderPassComp = nullptr;
ShaderProgramComponent* m_ToneMapSPC            = nullptr;
CommandListComponent*   m_ToneMapCommandList    = nullptr;

TextureComponent*   m_ToneMapOutput      = nullptr;
```

- [ ] **Step 2: Remove ToneMap Setup code from GPUPathTracerPass.cpp**

In `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`, in `Setup()`, remove lines 122-161 (the entire `// --- ToneMap SPC ---` through `m_ToneMapCommandList` initialization):

```cpp
// DELETE from "// --- ToneMap SPC ---" through:
// m_ToneMapCommandList->m_Type = GPUEngineType::Compute;
```

- [ ] **Step 3: Remove ToneMap Initialize code**

In `Initialize()`, remove these three lines (around line 220-222):

```cpp
// DELETE:
g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ToneMapSPC);
g_Engine->Get<RenderPassResourceService>()->Initialize(m_ToneMapRenderPassComp);
g_Engine->Get<CommandListResourceService>()->Initialize(m_ToneMapCommandList);
```

Also remove the `m_ToneMapOutput` creation block (lines 237-254):

```cpp
// DELETE from "// ToneMapOutput: LDR RGBA UByte, ComputeOnly UAV" through:
// g_Engine->Get<TextureResourceService>()->Initialize(m_ToneMapOutput);
```

- [ ] **Step 4: Remove ToneMap Terminate code**

In `Terminate()`, remove these lines (around 332-339):

```cpp
// DELETE:
if (m_ToneMapOutput)
    g_Engine->Get<TextureResourceService>()->Delete(m_ToneMapOutput);

g_Engine->Get<CommandListResourceService>()->Delete(m_ToneMapCommandList);
g_Engine->Get<RenderPassResourceService>()->Delete(m_ToneMapRenderPassComp);
g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ToneMapSPC);
```

- [ ] **Step 5: Remove ToneMap from PrepareCommandList**

In `PrepareCommandList()`, remove the `m_ToneMapOutput` transition from the Graphics CL (line 382):

```cpp
// DELETE:
l_fmService->TryToTransitState(m_ToneMapOutput, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
```

Remove the entire ToneMap CL block (lines 401-414):

```cpp
// DELETE from "// ToneMap CL: ToneMapOutput already in ReadWrite" through:
// l_fmService->CommandListEnd(m_ToneMapRenderPassComp, m_ToneMapCommandList);
```

The AccumulationBuffer transition from ReadWrite to ReadOnly (line 403) needs to stay, but move it into the ray tracing Compute CL instead. Add this line right before the existing `CommandListEnd` for the Compute CL (before line 399):

```cpp
l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
```

- [ ] **Step 6: Update GetResult and remove GetToneMapCommandList**

Change `GetResult()` to return the accumulation buffer:

```cpp
GPUResourceComponent* GPUPathTracerPass::GetResult()
{
    return m_AccumulationBuffer;
}
```

Delete the `GetToneMapCommandList()` method entirely:

```cpp
// DELETE:
CommandListComponent* GPUPathTracerPass::GetToneMapCommandList()
{
    return m_ToneMapCommandList;
}
```

- [ ] **Step 7: Delete the ToneMap shader file**

Delete `Source/Shaders/HLSL/GPUPathTracerToneMap.comp` and `Bin/Shaders/DXIL/GPUPathTracerToneMap.comp.dxil`.

- [ ] **Step 8: Build**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Build succeeds. Any compile errors from references to removed methods will be caught in Task 2.

- [ ] **Step 9: Commit**

```bash
git add -A Source/DefaultClient/RenderingClient/GPUPathTracerPass.h \
           Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp
git rm Source/Shaders/HLSL/GPUPathTracerToneMap.comp
git commit -m "refactor: strip ToneMap pass from GPUPathTracerPass

GetResult() now returns the raw HDR accumulation buffer.
The unified FinalBlendPass handles all tone mapping."
```

---

### Task 2: Route HDR source in PrepareCommands

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp`

- [ ] **Step 1: Change the path tracer branch in PrepareCommands**

Currently (around line 256-261):

```cpp
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    GPUPathTracerPass::Get().PrepareCommandList();
    m_Canvas = GPUPathTracerPass::Get().GetResult();
    m_CanvasOwner = GPUPathTracerPass::Get().GetRenderPassComp();
    return true;
}
```

Replace with — prepare the path tracer but do NOT return early or set canvas:

```cpp
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    GPUPathTracerPass::Get().PrepareCommandList();
}
```

- [ ] **Step 2: Guard the rasterized passes**

Wrap the rasterized pass preparation (from the one-shot BRDF block through TAAPass, lines 267-314) with a guard. The block starts at `if (m_ExecuteOneShotCommands)` and ends after `TAAPass::Get().PrepareCommandList(...)`:

```cpp
if (!m_GPUPathTracerActive)
{
    if (m_ExecuteOneShotCommands)
    {
        // ... existing BRDF one-shot code ...
    }

    // ... existing SunShadow through TAAPass preparation ...
    TAAPass::Get().PrepareCommandList(&l_TAAPassRenderingContext);
}
```

- [ ] **Step 3: Route the HDR source for histogram and FinalBlend**

Replace the current histogram and FinalBlend input selection (around lines 318-326) with mode-aware routing:

```cpp
GPUResourceComponent* l_hdrSource = nullptr;
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    l_hdrSource = GPUPathTracerPass::Get().GetResult();
}
else
{
    l_hdrSource = TAAPass::Get().GetResult();
}

LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
l_LuminanceHistogramPassRenderingContext.m_input = l_hdrSource;
LuminanceHistogramPass::Get().PrepareCommandList(&l_LuminanceHistogramPassRenderingContext);

LuminanceAveragePass::Get().PrepareCommandList();

FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
l_FinalBlendPassRenderingContext.m_input = l_hdrSource;
FinalBlendPass::Get().PrepareCommandList(&l_FinalBlendPassRenderingContext);
```

- [ ] **Step 4: Set canvas to FinalBlend for both paths**

The `m_Canvas` assignment at line 264-265 already points to FinalBlend. Remove the path-tracer-specific canvas override (already removed in Step 1). Verify that `m_Canvas` and `m_CanvasOwner` are always set to FinalBlend's result:

```cpp
m_Canvas = FinalBlendPass::Get().GetResult();
m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();
```

This line should remain as the single assignment before the rasterized guard block.

- [ ] **Step 5: Build**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Build succeeds.

- [ ] **Step 6: Commit**

```bash
git add Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
git commit -m "refactor: route path tracer HDR through shared post-processing

PrepareCommands no longer returns early for path tracer mode.
Rasterized passes are guarded by !m_GPUPathTracerActive.
Histogram, average, and FinalBlend always run with the
mode-appropriate HDR source."
```

---

### Task 3: Route HDR source in ExecuteCommands

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp`

- [ ] **Step 1: Replace the path tracer execution block**

The current path tracer execution block (lines 364-448) executes ray tracing + ToneMap CLs and then returns early. Replace it. Remove the `return true;` at line 448 and the ToneMap CL execution (lines 379-383). Also remove the readback/validation block (lines 385-446) — it reads from `m_ToneMapOutput` which no longer exists. The validation can be re-added later reading from FinalBlend if needed.

Replace the entire block (lines 364-449) with:

```cpp
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    auto l_renderPass = GPUPathTracerPass::Get().GetRenderPassComp();

    // Graphics CL: transition accumulation buffer
    auto l_graphicsCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Graphics);
    l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
    l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
    l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

    // Compute CL: ray tracing dispatch (also transitions AccumBuffer to ReadOnly at end)
    auto l_computeCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Compute);
    l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
    l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
}
```

Note: no `return true;` — execution falls through to the shared post-processing passes.

- [ ] **Step 2: Guard rasterized execution passes**

Wrap the rasterized GPU execution blocks (SunShadowCulling through TAAPass, lines 451-680) in a guard:

```cpp
if (!m_GPUPathTracerActive)
{
    if (SunShadowCullingPass::Get().GetStatus() == ObjectStatus::Activated)
    {
        // ... existing SunShadow execution ...
    }

    // ... all passes through TAAPass ...

    if (TAAPass::Get().GetStatus() == ObjectStatus::Activated)
    {
        // ... existing TAA execution ...
    }
}
```

The histogram, average, and FinalBlend blocks (lines 682-722) stay OUTSIDE the guard — they always run.

- [ ] **Step 3: Fix the histogram GPU wait dependency**

The histogram execution block (line 684) waits on TAAPass:

```cpp
l_hwService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
```

When the path tracer is active, TAA didn't run, so we need to wait on the path tracer instead:

```cpp
if (m_GPUPathTracerActive)
    l_hwService->WaitOnGPU(GPUPathTracerPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
else
    l_hwService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
```

- [ ] **Step 4: Fix the FinalBlend GPU wait dependency**

The FinalBlend execution block (lines 709-710) waits on both TAAPass and LuminanceAverage. When path tracer is active, replace the TAA wait with path tracer wait:

```cpp
if (m_GPUPathTracerActive)
    l_hwService->WaitOnGPU(GPUPathTracerPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
else
    l_hwService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
l_hwService->WaitOnGPU(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
```

- [ ] **Step 5: Build**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Build succeeds.

- [ ] **Step 6: Commit**

```bash
git add Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
git commit -m "refactor: unify GPU execution for path tracer and rasterized pipelines

Path tracer no longer returns early from ExecuteCommands.
Rasterized passes guarded by !m_GPUPathTracerActive.
Histogram and FinalBlend wait on the correct producer
(path tracer or TAA) based on active mode."
```

---

### Task 4: Test all four tiers

- [ ] **Step 1: Compile shaders (GPUPathTracerToneMap.comp was deleted)**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/HLSL2DXIL.ps1"
```

Expected: No errors. The deleted shader is simply not compiled.

- [ ] **Step 2: RenderTest regression**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: Exit code 0.

- [ ] **Step 3: Main.exe integration test**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: Exit code 0.

- [ ] **Step 4: Scene reload test**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: Exit code 0.

- [ ] **Step 5: Interactive path tracer toggle test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario toggle_pathtracer
```

Expected: PASS. Path tracer output should now have auto-exposure applied — no white blowout.

- [ ] **Step 6: Full interactive test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario full
```

Expected: PASS. Camera movement, path tracer toggle, and scene reload all work without crashes.

- [ ] **Step 7: Clean up the deleted compiled shader if still present**

```bash
rm -f "C:/GitRepo/InnocenceEngine/Bin/Shaders/DXIL/GPUPathTracerToneMap.comp.dxil"
```
