# Path Tracer: Firefly Fix, BRDF Alignment, and Unified Post-Processing

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix path tracer fireflies, align its BRDF model with the rasterized pipeline's physics, and route its output through the existing post-processing chain (auto-exposure, ACES tonemap, accurate sRGB) instead of a custom ToneMap pass.

**Architecture:** The path tracer's RayGen shader gets inline BRDF fixes and a radiance clamp. The custom `GPUPathTracerToneMap.comp` pass is removed entirely. Instead, `DefaultRenderingClient` feeds the AccumulationBuffer into PostTAA -> LuminanceHistogram -> LuminanceAverage -> FinalBlend, skipping only the TAA pass (path tracing is inherently anti-aliased by jittered rays).

**Tech Stack:** HLSL (DXR ray tracing shaders, compute shaders), C++ (render pass management, command list submission)

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | Modify | Firefly clamp + BRDF alignment |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h` | Modify | Remove ToneMap members |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` | Modify | Remove ToneMap setup/init/prepare/terminate, change GetResult to return AccumulationBuffer |
| `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp` | Modify | Route path tracer through unified post-processing |
| `Source/Shaders/HLSL/GPUPathTracerToneMap.comp` | Delete | No longer needed |

---

### Task 1: Fix Fireflies with Radiance Clamp

**Files:**
- Modify: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl:206`

The GGX importance sampling can produce extreme throughput values when the PDF denominator is near-zero. Clamping per-sample radiance before accumulation eliminates visible fireflies (bright isolated pixels).

- [ ] **Step 1: Add radiance clamp before accumulation**

In `GPUPathTracerRayGen.hlsl`, replace line 217:

```hlsl
// Before:
AccumBuffer[pixel] = lerp(prev, float4(radiance, 1.0f), t);

// After:
float3 clampedRadiance = min(radiance, 100.0f);
AccumBuffer[pixel] = lerp(prev, float4(clampedRadiance, 1.0f), t);
```

The clamp value of 100.0 is generous enough to preserve HDR range for auto-exposure while preventing the 10,000+ spikes from degenerate sampling.

- [ ] **Step 2: Build**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/HLSL2DXIL.ps1"
```

Expected: Shader compilation succeeds (GPUPathTracerRayGen.hlsl compiles to DXIL).

- [ ] **Step 3: Commit**

```bash
git add Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl
git commit -m "fix: clamp path tracer radiance to eliminate fireflies"
```

---

### Task 2: Align BRDF with Rasterized Pipeline Physics

**Files:**
- Modify: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl:55-96` (inline BRDF functions)

The path tracer currently uses uncorrelated Schlick-GGX geometry and simple Lambertian diffuse. The rasterized pipeline (`common/BSDF.hlsl`) uses height-correlated Smith GGX, Disney 2015 Burley diffuse, and Fresnel with F90. We align the path tracer's inline BRDF to match. We keep the functions self-contained (no `#include "common/BSDF.hlsl"`) because the path tracer's BRDF evaluation context (Monte Carlo integration) differs from the rasterized pipeline's direct evaluation.

Also replace the hardcoded light direction with the scene's actual sun from `g_Frame.sun_direction` and `g_Frame.sun_illuminance`.

- [ ] **Step 1: Replace geometry function with height-correlated Smith GGX**

In `GPUPathTracerRayGen.hlsl`, replace the two functions `GeometrySchlickGGX` and `GeometrySmith` (lines 64-76) with:

```hlsl
float GeometrySmithGGXCorrelated(float NdotL, float NdotV, float alpha)
{
    float alpha2 = alpha * alpha;
    float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0f - alpha2) + alpha2);
    float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0f - alpha2) + alpha2);
    return 0.5f / max(lambdaV + lambdaL, 0.0001f);
}
```

- [ ] **Step 2: Replace Fresnel with F90 variant**

Replace `FresnelSchlick` (lines 78-81) with:

```hlsl
float3 FresnelSchlick(float cosTheta, float3 F0, float F90)
{
    return F0 + (F90 - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}
```

- [ ] **Step 3: Add Disney 2015 Burley diffuse function**

Add after the `FresnelSchlick` function:

```hlsl
float DisneyDiffuse2015(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float F_l = pow(1.0f - NdotL, 5.0f);
    float F_v = pow(1.0f - NdotV, 5.0f);
    float retroReflect = 2.0f * LdotH * LdotH * linearRoughness;
    float FLambert = (1.0f - 0.5f * F_l) * (1.0f - 0.5f * F_v);
    float FRetroReflection = retroReflect * (F_l + F_v + F_l * F_v * (retroReflect - 1.0f));
    return FLambert + FRetroReflection;
}
```

- [ ] **Step 4: Rewrite CookTorranceGGX to use aligned physics**

Replace the existing `CookTorranceGGX` function (lines 83-96) with:

```hlsl
float3 CookTorranceGGX(float3 N, float3 V, float3 L, float3 albedo, float metalness, float roughness)
{
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float LdotH = max(dot(L, H), 0.0f);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    float alpha = roughness * roughness;

    float  D = DistributionGGX(N, H, roughness);
    float  G = GeometrySmithGGXCorrelated(NdotL, NdotV, alpha);
    float3 F = FresnelSchlick(LdotH, F0, 1.0f);

    // Specular: D * G * F (correlated Smith already includes 1/(4*NdotL*NdotV) denominator)
    float3 specular = D * G * F;

    // Diffuse: Disney 2015 Burley with energy conservation
    float3 kD = (1.0f - F) * (1.0f - metalness);
    float  diffuseTerm = DisneyDiffuse2015(NdotV, NdotL, LdotH, roughness);
    float3 diffuse = kD * albedo * diffuseTerm / 3.14159265f;

    return (diffuse + specular) * NdotL;
}
```

Note: The correlated Smith GGX function returns `0.5 / (lambdaV + lambdaL)` which already incorporates the `1/(4*NdotL*NdotV)` denominator from the Cook-Torrance formula, so we do NOT divide by `4*NdotV*NdotL` again.

- [ ] **Step 5: Update importance sampling bounce throughput to use aligned physics**

Replace the bounce throughput calculation in the main loop (lines 197-206) with:

```hlsl
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
        float3 F  = FresnelSchlick(max(dot(H, V), 0.0f), F0, 1.0f);
        float  D  = DistributionGGX(N, H, roughness);
        float  alpha = roughness * roughness;
        float  G  = GeometrySmithGGXCorrelated(NdotL, max(dot(N, V), 0.0f), alpha);
        float  NdotH = max(dot(N, H), 0.0f);
        float  VdotH = max(dot(V, H), 0.0f);
        float  pdf   = (D * NdotH) / (4.0f * VdotH + 0.0001f);

        // Correlated Smith G already includes 1/(4*NdotL*NdotV)
        float3 specular = D * G * F;
        throughput *= specular * NdotL / max(pdf, 0.0001f);
```

- [ ] **Step 6: Replace hardcoded light with scene sun**

Replace lines 173-174:

```hlsl
        // Before:
        float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
        float  lightIntensity = 3.0f;

        // After:
        float3 lightDir = normalize(g_Frame.sun_direction.xyz);
        float3 lightIlluminance = g_Frame.sun_illuminance.xyz;
```

And update the direct lighting line (line 188):

```hlsl
        // Before:
        radiance += throughput * CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIntensity;

        // After:
        radiance += throughput * CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
```

- [ ] **Step 7: Build shaders**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/HLSL2DXIL.ps1"
```

Expected: Shader compilation succeeds.

- [ ] **Step 8: Commit**

```bash
git add Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl
git commit -m "refactor: align path tracer BRDF with rasterized pipeline physics

Use height-correlated Smith GGX, Disney 2015 Burley diffuse, Fresnel with
F90=1.0, and scene sun light instead of hardcoded direction."
```

---

### Task 3: Unified Post-Processing Pipeline

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h`
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp`
- Delete: `Source/Shaders/HLSL/GPUPathTracerToneMap.comp`

The path tracer currently runs its own ToneMap compute pass with hardcoded exposure=1.0 and `pow(x, 1/2.2)` gamma. We remove this entirely and route the HDR AccumulationBuffer through the existing post-processing chain: PostTAA -> LuminanceHistogram -> LuminanceAverage -> FinalBlend. This gives the path tracer auto-exposure, proper ACES tonemapping, and accurate linear-to-sRGB conversion.

#### Sub-task 3a: Remove ToneMap pass from GPUPathTracerPass

- [ ] **Step 1: Remove ToneMap members from header**

In `GPUPathTracerPass.h`, remove these declarations:

```cpp
// Remove these method declarations:
CommandListComponent* GetToneMapCommandList();
CommandListComponent* GetToneMapCommandList_Graphics();

// Remove these member variables:
RenderPassComponent*    m_ToneMapRenderPassComp = nullptr;
ShaderProgramComponent* m_ToneMapSPC            = nullptr;
CommandListComponent*   m_ToneMapCommandList          = nullptr;
CommandListComponent*   m_ToneMapCommandList_Graphics = nullptr;
TextureComponent*   m_ToneMapOutput      = nullptr;
```

- [ ] **Step 2: Update GetResult to return AccumulationBuffer**

In `GPUPathTracerPass.h`, remove the `GetAccumulationBuffer()` declaration (no longer needed since `GetResult()` will return it directly).

In `GPUPathTracerPass.cpp`, change `GetResult()`:

```cpp
GPUResourceComponent* GPUPathTracerPass::GetResult()
{
    return m_AccumulationBuffer;
}
```

Remove the `GetAccumulationBuffer()`, `GetToneMapCommandList()`, and `GetToneMapCommandList_Graphics()` method implementations.

- [ ] **Step 3: Remove ToneMap setup code from Setup()**

In `GPUPathTracerPass.cpp::Setup()`, remove lines 122-164 (everything from `// --- ToneMap SPC ---` through `m_ToneMapCommandList->m_Type = GPUEngineType::Compute;`).

- [ ] **Step 4: Remove ToneMap initialization from Initialize()**

In `GPUPathTracerPass.cpp::Initialize()`, remove:
- Lines 196-199 (ToneMap SPC/RenderPass/CommandList initialization)
- Lines 214-225 (m_ToneMapOutput texture creation and initialization)

- [ ] **Step 5: Remove ToneMap from PrepareCommandList()**

In `GPUPathTracerPass.cpp::PrepareCommandList()`, remove lines 370-388 (everything from `// ToneMap Graphics CL` to the end of the function, before the `return true`).

After the ray tracing compute dispatch (line 368), add a barrier to transition AccumulationBuffer back to ReadOnly for the post-processing chain:

```cpp
    l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

    // Transition AccumulationBuffer to ReadOnly for downstream post-processing
    l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 1);
    l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadWrite, Accessibility::ReadOnly);
    l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);
```

Wait — the existing code already has a separate Graphics CL at lines 349-352 that transitions to ReadWrite, and the ToneMap Graphics CL at 371-374 transitions back to ReadOnly. We need to keep the ReadWrite→ReadOnly transition but do it in a second Graphics CL submission instead of the ToneMap one. Looking at the command list infrastructure, the existing `m_CommandListComp_Graphics` is reused per-frame. We need to record a second pass on it.

Actually, re-examining: the existing code records `m_CommandListComp_Graphics` once (ReadOnly→ReadWrite), then records `m_ToneMapCommandList_Graphics` (ReadWrite→ReadOnly for accum, ReadOnly→ReadWrite for tonemap output). After removing ToneMap, we need the AccumulationBuffer in ReadOnly state for PostTAA to read it. We should transition it back in a second use of `m_CommandListComp_Graphics` or simply leave the transition to the PostTAA pass which already transitions its input.

Looking at PostTAAPass::PrepareCommandList (line 111), it transitions its input from `WriteOnly` to `ReadOnly`. But our AccumulationBuffer will be in `ReadWrite` (UAV) state after ray tracing. We need to transition it to a state that PostTAA expects.

The simplest approach: after the compute CL ends, record a second graphics CL pass that transitions AccumulationBuffer from ReadWrite to WriteOnly (which is what PostTAA expects its input to be in):

```cpp
    l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

    // Second graphics CL: transition AccumulationBuffer for post-processing readback
    l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 1);
    l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadWrite, Accessibility::WriteOnly);
    l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);
```

Note: The `1` in CommandListBegin is the submission index — we're recording a second submission on the same command list component.

- [ ] **Step 6: Remove ToneMap cleanup from Terminate()**

In `GPUPathTracerPass.cpp::Terminate()`, remove:
- `m_ToneMapOutput` deletion (line 305-306 area)
- ToneMap command list, render pass, and SPC deletions (lines 308-311)

- [ ] **Step 7: Build C++ and shaders**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Build succeeds. Check with:
```
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

- [ ] **Step 8: Commit ToneMap removal**

```bash
git add Source/DefaultClient/RenderingClient/GPUPathTracerPass.h Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp
git commit -m "refactor: remove custom ToneMap pass from GPU path tracer

GetResult() now returns the HDR AccumulationBuffer directly.
ToneMap SPC, render pass, command lists, and output texture are removed."
```

#### Sub-task 3b: Route path tracer through unified post-processing

- [ ] **Step 9: Modify PrepareCommands in DefaultRenderingClient**

In `DefaultRenderingClient.cpp`, replace the path tracer early-return block (lines 259-265):

```cpp
// Before:
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    GPUPathTracerPass::Get().PrepareCommandList();
    m_Canvas = GPUPathTracerPass::Get().GetResult();
    m_CanvasOwner = GPUPathTracerPass::Get().GetRenderPassComp();
    return true;
}
```

With:

```cpp
if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
{
    GPUPathTracerPass::Get().PrepareCommandList();

    // Feed HDR AccumulationBuffer through unified post-processing (skip TAA)
    PostTAAPassRenderingContext l_PostTAAPassRenderingContext;
    l_PostTAAPassRenderingContext.m_input = GPUPathTracerPass::Get().GetResult();
    PostTAAPass::Get().PrepareCommandList(&l_PostTAAPassRenderingContext);

    LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
    l_LuminanceHistogramPassRenderingContext.m_input = PostTAAPass::Get().GetResult();
    LuminanceHistogramPass::Get().PrepareCommandList(&l_LuminanceHistogramPassRenderingContext);

    LuminanceAveragePass::Get().PrepareCommandList();

    FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
    l_FinalBlendPassRenderingContext.m_input = PostTAAPass::Get().GetResult();
    FinalBlendPass::Get().PrepareCommandList(&l_FinalBlendPassRenderingContext);

    m_Canvas = FinalBlendPass::Get().GetResult();
    m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();
    return true;
}
```

- [ ] **Step 10: Modify ExecuteCommands in DefaultRenderingClient**

In `DefaultRenderingClient.cpp::ExecuteCommands`, the path tracer block (lines 369-454) currently submits ray tracing CLs + ToneMap CLs + validation readback. Replace with ray tracing CLs + post-processing CLs.

Replace the ToneMap execution section (lines 384-394) with post-processing execution:

```cpp
        // Post-processing: PostTAA -> LuminanceHistogram -> LuminanceAverage -> FinalBlend
        // PostTAA
        auto l_postTAARenderPass = PostTAAPass::Get().GetRenderPassComp();
        l_hwService->WaitOnGPU(l_postTAARenderPass, GPUEngineType::Graphics, GPUEngineType::Compute);
        auto l_postTAAGraphicsCL = PostTAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
        l_hwService->Execute(l_postTAAGraphicsCL, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_postTAARenderPass, GPUEngineType::Graphics);
        l_hwService->WaitOnGPU(l_postTAARenderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
        auto l_postTAAComputeCL = PostTAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
        l_hwService->Execute(l_postTAAComputeCL, GPUEngineType::Compute);
        l_hwService->SignalOnGPU(l_postTAARenderPass, GPUEngineType::Compute);

        // LuminanceHistogram
        auto l_lumHistRenderPass = LuminanceHistogramPass::Get().GetRenderPassComp();
        l_hwService->WaitOnGPU(l_lumHistRenderPass, GPUEngineType::Graphics, GPUEngineType::Compute);
        auto l_lumHistGraphicsCL = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Graphics);
        l_hwService->Execute(l_lumHistGraphicsCL, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_lumHistRenderPass, GPUEngineType::Graphics);
        l_hwService->WaitOnGPU(l_lumHistRenderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
        auto l_lumHistComputeCL = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Compute);
        l_hwService->Execute(l_lumHistComputeCL, GPUEngineType::Compute);
        l_hwService->SignalOnGPU(l_lumHistRenderPass, GPUEngineType::Compute);

        // LuminanceAverage
        auto l_lumAvgRenderPass = LuminanceAveragePass::Get().GetRenderPassComp();
        l_hwService->WaitOnGPU(l_lumAvgRenderPass, GPUEngineType::Compute, GPUEngineType::Compute);
        auto l_lumAvgComputeCL = LuminanceAveragePass::Get().GetCommandListComp(GPUEngineType::Compute);
        l_hwService->Execute(l_lumAvgComputeCL, GPUEngineType::Compute);
        l_hwService->SignalOnGPU(l_lumAvgRenderPass, GPUEngineType::Compute);

        // FinalBlend
        auto l_finalBlendRenderPass = FinalBlendPass::Get().GetRenderPassComp();
        l_hwService->WaitOnGPU(l_finalBlendRenderPass, GPUEngineType::Graphics, GPUEngineType::Compute);
        auto l_finalBlendGraphicsCL = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Graphics);
        l_hwService->Execute(l_finalBlendGraphicsCL, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_finalBlendRenderPass, GPUEngineType::Graphics);
        l_hwService->WaitOnGPU(l_finalBlendRenderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
        auto l_finalBlendComputeCL = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Compute);
        l_hwService->Execute(l_finalBlendComputeCL, GPUEngineType::Compute);
        l_hwService->SignalOnGPU(l_finalBlendRenderPass, GPUEngineType::Compute);
```

Also update the validation readback to read from FinalBlend's result instead of ToneMapOutput. The validation block (lines 396-454) references `GPUPathTracerPass::Get().GetResult()` which now returns AccumulationBuffer (HDR). Change it to read `FinalBlendPass::Get().GetResult()` instead, and update `l_canvasOwner` for the readback to use `FinalBlendPass::Get().GetRenderPassComp()`.

- [ ] **Step 11: Update the second Graphics CL execution**

The existing ExecuteCommands submits the path tracer's second Graphics CL (ToneMapGraphicsCL). After our changes, we need to submit the second Graphics CL from PrepareCommandList step 5 instead. Replace lines 384-388 (ToneMap Graphics CL execution) with:

```cpp
        // Second Graphics CL: transition AccumulationBuffer for post-processing
        l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Graphics, GPUEngineType::Compute);
        l_graphicsCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Graphics);
        l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
```

Wait — there's an issue. `GetCommandListComp(Graphics)` returns the same command list component, but it may have been recorded with two submissions (index 0 and index 1). Need to verify how multiple submissions on the same CommandListComponent work in this engine. 

Actually, looking at the existing rasterized pipeline in ExecuteCommands (the code after line 454), each pass calls `GetCommandListComp(GPUEngineType::Graphics)` and `GetCommandListComp(GPUEngineType::Compute)` separately and the engine handles multiple recordings. The first `Execute` on the Graphics CL submits recording 0, the second `Execute` submits recording 1. This is consistent with how the existing code works (e.g., the ToneMap pass had its own separate CommandListComponent).

Given the complexity of reusing the same CommandListComponent for two submissions, it's simpler to keep the existing pattern: a separate `m_CommandListComp_Graphics2` or simply not record a second graphics pass. Instead, let PostTAAPass handle the transition of its own input. PostTAAPass already transitions its input from WriteOnly to ReadOnly (line 111). If the AccumulationBuffer is in ReadWrite (UAV) state after ray tracing, we just need to match states.

**Revised approach for Step 5:** Instead of recording a second graphics CL, we should adjust the PostTAAPass transition to handle the AccumulationBuffer's actual state. The simplest way: in PrepareCommandList, after the compute CL, don't do any extra transition. In DefaultRenderingClient, when we call PostTAAPass::PrepareCommandList, the PostTAA Graphics CL will transition the input. PostTAA currently transitions from WriteOnly→ReadOnly. The AccumulationBuffer will be in ReadWrite state. We need PostTAA to transition from ReadWrite→ReadOnly instead.

But we can't change PostTAA's hardcoded transition without affecting the rasterized pipeline. Better approach: keep the existing first Graphics CL (ReadOnly→ReadWrite before ray tracing), and just add transition back to WriteOnly at the end of the compute CL by using a UAV barrier, or let the transition happen naturally.

**Simplest correct approach:** Remove the ToneMap code from PrepareCommandList (lines 370-388). The AccumulationBuffer will be left in ReadWrite state after DispatchRays. When PostTAAPass receives it as input and calls `TryToTransitState(input, ..., WriteOnly, ReadOnly)`, the engine's barrier system will see the actual state is ReadWrite and transition accordingly (TryToTransitState uses the tracked state, not the requested source state — that's what "Try" means).

Let me re-examine: actually at line 111, PostTAAPass does `TryToTransitState(input, CL, Accessibility::WriteOnly, Accessibility::ReadOnly)`. The `WriteOnly` here is the expected source state. If the actual tracked state is `ReadWrite`, the engine should still issue the correct barrier. This is engine-dependent behavior that needs verification at runtime.

**Final approach for Step 5:** Simply remove the ToneMap CL recordings from PrepareCommandList. Don't add any extra transition. Let PostTAA handle it. If the barrier tracking doesn't match, we'll catch it in the test step and fix.

- [ ] **Step 12: Delete GPUPathTracerToneMap.comp**

```bash
git rm Source/Shaders/HLSL/GPUPathTracerToneMap.comp
```

- [ ] **Step 13: Build**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Build succeeds.

- [ ] **Step 14: Run integration test**

```
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: Exit code 0. No D3D12 validation errors. Path tracer output goes through auto-exposure and proper tonemapping.

- [ ] **Step 15: Run interactive test (path tracer toggle)**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario toggle_pathtracer
```

Expected: Exit code 0. Toggling path tracer on/off works without crash.

- [ ] **Step 16: Commit**

```bash
git add Source/DefaultClient/RenderingClient/GPUPathTracerPass.h Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
git rm Source/Shaders/HLSL/GPUPathTracerToneMap.comp
git commit -m "refactor: route path tracer through unified post-processing pipeline

Remove custom ToneMap pass. AccumulationBuffer feeds directly into
PostTAA -> LuminanceHistogram -> LuminanceAverage -> FinalBlend,
giving the path tracer auto-exposure, ACES tonemapping, and accurate
sRGB conversion."
```

---

### Task 4: Log Post-Processing Quality Issues

Observations from reading the post-processing shaders that should be logged for future improvement:

1. **`finalBlendPass.comp` variable naming**: Uses `bassPass` instead of `basePass` throughout (typo in original code). Not a correctness issue but reduces readability.

2. **`postTAAPass.comp` is a pure passthrough**: The shader just copies `TAAResult.rgb` to output with alpha=1. This is an unnecessary GPU dispatch — could be eliminated by having FinalBlend read TAA output directly, saving one full-screen compute pass and the associated barrier transitions.

3. **`luminanceAveragePass.comp` overwrites history at index 0**: Line 68 (`out_average[0] = adaptedLuminance`) overwrites the oldest history entry with the adapted average. When `frameIndex % numHistoryFrames == 0`, the current frame's raw luminance (written at line 57) is immediately overwritten by the adapted average, losing one history sample. This creates a subtle periodic glitch every 8 frames.

4. **`luminanceHistogramPass.comp` single-thread shared memory clear**: Lines 42-49 use a single thread (groupThreadID 0,0) to clear 256 entries in a loop. This could be parallelized across the 256 threads in the group (16x16=256) for a minor perf win.

- [ ] **Step 1: Log issues to backlog**

Create backlog tasks for each quality issue discovered, or document them in a tracked location for future work.

- [ ] **Step 2: Commit plan notes (if any)**

No code changes needed for this task — just tracking.
