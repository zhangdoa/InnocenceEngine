# E2E Rendering Test Harness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add pixel-level validation to RenderTest.exe: restore the commented-out DX12 ReadTextureBackToCPU, add a `pixel_readback` test case that renders and validates a known-color region, exit with code 2 on validation failure.

**Architecture:** DX12RenderingServer::ReadTextureBackToCPU is restored by uncommenting and fixing two issues (DX12TextureComponent cast removed, m_PixelDataSize replaced with DX12Helper::GetTexturePixelDataSize). The `pixel_readback` test case reuses the existing `draw_instanced` render pass (solid-red triangle) and adds a readback + validation step after rendering. Failure sets a flag; Terminate() returns false; WinMain/main returns exit code 2.

**Tech Stack:** C++17, Direct3D 12, existing `DX12Helper::GetTexturePixelDataSize`, `TObjectPool` from engine. No new shaders needed (reuse `drawInstanced.vert/frag` which already output solid red).

---

## File Map

**DX12 readback restoration:**
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Public.cpp` — uncomment ReadTextureBackToCPU, fix two issues

**Test client extension:**
- Modify: `Source/TestClient/TestRenderingClient.h` — add `PixelReadback` enum value, `m_ValidationPassed` flag, accessor
- Modify: `Source/TestClient/TestRenderingClient.cpp` — add Setup/Initialize/Execute/Terminate for pixel_readback case; call ReadTextureBackToCPU and validate

**Exit code:**
- Modify: `Source/Engine/Platform/WinMain/WinMain.cpp` (or wherever RenderTest.exe's main is) — return 2 when `!renderingClient->GetValidationPassed()`

---

## Task 1: Restore DX12 ReadTextureBackToCPU

**Files:**
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Public.cpp`

The implementation is entirely commented out (lines 48–260). `DX12DeviceMemory::m_ReadBackHeapBuffer` already exists in `DX12Headers.h`. Two bugs caused the comment-out:
1. `auto l_rhs = reinterpret_cast<DX12TextureComponent*>(TextureComp)` — `DX12TextureComponent` no longer exists; use `TextureComp` directly.
2. `l_rhs->m_PixelDataSize` — field does not exist; replace with `DX12Helper::GetTexturePixelDataSize(TextureComp->m_TextureDesc)`.

Only implement the non-DepthStencil + non-Float16 path for now — the test uses the default RGBA8/UNORM render target that `GetDefaultRenderPassDesc()` creates.

- [ ] **Step 1: Read the full ReadTextureBackToCPU function**

```
Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Public.cpp lines 42–260
```

Understand the two lambdas: `f_DefaultToReadbackHeap` (GPU barrier + CopyTextureRegion) and `f_ReadbackToHostHeap` (Map + memcpy). Both are correct and just need uncommenting.

- [ ] **Step 2: Read DX12Helper_Texture.h and verify GetTexturePixelDataSize signature**

```
Source/Engine/RenderingServer/DX12/DX12Helper_Texture.h
```

Confirm: `uint32_t DX12Helper::GetTexturePixelDataSize(TextureDesc textureDesc)` exists and takes a `TextureDesc`.

- [ ] **Step 3: Uncomment the entire function body**

Remove all `//` comment prefixes from lines 48–255. Then apply the two fixes:

**Fix 1** — replace the `DX12TextureComponent` cast:
```cpp
// Before (now deleted):
// auto l_rhs = reinterpret_cast<DX12TextureComponent*>(TextureComp);

// Replace every use of l_rhs with TextureComp directly:
auto l_rhs = TextureComp;  // TextureComponent* is the concrete type
```

**Fix 2** — replace `l_rhs->m_PixelDataSize`:
```cpp
// Before:
auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

// After:
auto l_pixelDataSize = DX12Helper::GetTexturePixelDataSize(TextureComp->m_TextureDesc);
auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_pixelDataSize, l_pixelCount);
```

Apply this fix to both occurrences (depth/stencil path and color path).

Keep the DepthStencil and Float16 conversion blocks commented out — they're not needed for the test and are untested. Only the non-DepthStencil, non-Float16 path (`l_format = l_rhs->m_DX12TextureDesc.Format` path) needs to be active. Simplify by replacing the outer if/else with just the color path:

```cpp
// Only color (non-depth) path is active; DepthStencil to be re-enabled separately
l_format = l_rhs->m_DX12TextureDesc.Format;
f_DefaultToReadbackHeap(l_DeviceMemory->m_DefaultHeapBuffer,
                         l_DeviceMemory->m_ReadBackHeapBuffer,
                         l_footprints, l_format, D3D12_RESOURCE_STATE_COMMON);
auto l_pixelDataSize = DX12Helper::GetTexturePixelDataSize(TextureComp->m_TextureDesc);
auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_pixelDataSize, l_pixelCount);

l_result.resize(l_pixelCount);
// convert based on pixel data type
if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
{
    for (size_t i = 0; i < l_pixelCount; ++i)
    {
        const unsigned char* pixelData = &l_rawResult[i * 16];
        float r, g, b, a;
        memcpy(&r, pixelData + 0,  4);
        memcpy(&g, pixelData + 4,  4);
        memcpy(&b, pixelData + 8,  4);
        memcpy(&a, pixelData + 12, 4);
        l_result[i] = Vec4(r, g, b, a);
    }
}
else  // UByte8 / UNORM
{
    for (size_t i = 0; i < l_pixelCount; ++i)
    {
        const unsigned char* pixelData = &l_rawResult[i * 4];
        l_result[i] = Vec4(pixelData[0] / 255.0f,
                           pixelData[1] / 255.0f,
                           pixelData[2] / 255.0f,
                           pixelData[3] / 255.0f);
    }
}
```

Note: check what `TexturePixelDataType` values are defined in GraphicsPrimitive.h before writing this step.

- [ ] **Step 4: Add DX12Helper_Texture.h include if not already present**

Check the top of `DX12RenderingServer_EngineComponent_Public.cpp` for existing includes. If `DX12Helper_Texture.h` is not included, add it:
```cpp
#include "DX12Helper_Texture.h"
```

- [ ] **Step 5: Build the DX12 rendering server**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: no errors. If `DX12TextureComponent` symbol errors appear, ensure step 3 Fix 1 was applied.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Public.cpp
git commit -m "feat: restore DX12 ReadTextureBackToCPU — fix DX12TextureComponent cast and m_PixelDataSize"
```

---

## Task 2: Add pixel_readback test case

**Files:**
- Modify: `Source/TestClient/TestRenderingClient.h`
- Modify: `Source/TestClient/TestRenderingClient.cpp`

The `pixel_readback` test case renders a red triangle (same as `draw_instanced`) then reads back the first render target, samples the center region, and asserts red pixels.

The key insight: after `Initialize(renderPass)`, the render target is at `renderPass->m_OutputMergerTarget->m_ColorOutputs[0]` (a `TextureComponent*`). The render pass width/height comes from `m_RenderPassDesc.m_RenderTargetDesc.Width` (check the exact field name in `GraphicsPrimitive.h`).

- [ ] **Step 1: Read TestRenderingClient.h and TestRenderingClient.cpp in full**

```
Source/TestClient/TestRenderingClient.h
Source/TestClient/TestRenderingClient.cpp
```

Understand the existing `DrawInstanced` test case structure. The `pixel_readback` case reuses the same resource struct (`DrawInstancedResources`) and most of the same setup.

- [ ] **Step 2: Update TestRenderingClient.h**

Add `PixelReadback` to the `TestCase` enum. Add `m_ValidationPassed` flag and accessor:

```cpp
enum class TestCase { Unknown, BareBoot, DrawInstanced, PixelReadback };

// ... existing members ...

bool GetValidationPassed() const { return m_ValidationPassed; }

private:
bool m_ValidationPassed = true;  // starts true; set false on validation failure
```

- [ ] **Step 3: Add ParseTestCase entry in TestRenderingClient.cpp**

```cpp
if (strcmp(name, "pixel_readback") == 0) return TestCase::PixelReadback;
```

- [ ] **Step 4: Add Setup_PixelReadback()**

Reuses `drawInstanced.vert/frag` (already outputs solid red triangle). Resource names differ from `draw_instanced` to avoid LUT collisions.

**Critical:** Set the render target to 256×256 with an explicit RGBA8 UNORM format. This size guarantees row stride (256 × 4 = 1024 bytes) is a multiple of DX12's 256-byte alignment, so `ReadTextureBackToCPU`'s `l_pixelCount * l_pixelDataSize` byte copy is correct with no row-pitch padding gaps.

Before writing this function, read `Source/Engine/Common/GraphicsPrimitive.h` to confirm the field names for setting render target width/height and pixel data type in `RenderPassDesc`. Look for fields like `m_RenderTargetDesc`, `m_TextureDesc`, or equivalent on `RenderPassDesc`.

```cpp
bool TestRenderingClient::Setup_PixelReadback()
{
    auto l_rs = g_Engine->getRenderingServer();

    m_DrawInstanced = new DrawInstancedResources();

    m_DrawInstanced->ShaderProgram = l_rs->AddShaderProgramComponent("TestPixelReadback/");
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_VSPath = "drawInstanced.vert/";
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_PSPath = "drawInstanced.frag/";

    m_DrawInstanced->RenderPass = l_rs->AddRenderPassComponent("TestPixelReadback/");

    auto l_desc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
    l_desc.m_RenderTargetCount = 1;
    l_desc.m_UseDepthBuffer    = false;
    l_desc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_PrimitiveTopology = PrimitiveTopology::Point;

    // Override to small fixed-size RGBA8 target.
    // 256×256 at 4 bytes/pixel = 1024-byte row stride (multiple of DX12's 256-byte alignment).
    // Adjust the field names below to match RenderPassDesc's actual render target size/format fields.
    l_desc.m_RenderTargetDesc.Width             = 256;
    l_desc.m_RenderTargetDesc.Height            = 256;
    l_desc.m_RenderTargetDesc.PixelDataFormat   = TexturePixelDataFormat::RGBA8;
    l_desc.m_RenderTargetDesc.PixelDataType     = TexturePixelDataType::UByte8;

    m_DrawInstanced->RenderPass->m_RenderPassDesc = l_desc;
    m_DrawInstanced->RenderPass->m_ShaderProgram  = m_DrawInstanced->ShaderProgram;

    m_DrawInstanced->CommandList = l_rs->AddCommandListComponent("TestPixelReadback/Graphics/");
    m_DrawInstanced->CommandList->m_Type = GPUEngineType::Graphics;

    return true;
}
```

> If `RenderPassDesc` does not have a `m_RenderTargetDesc` sub-struct, check how `draw_instanced`'s render pass description creates its render target size in `GetDefaultRenderPassDesc()` and apply the same override pattern to set width=256, height=256. The RGBA8/UByte8 format is the default — confirm and document in a comment.

- [ ] **Step 5: Add Initialize_PixelReadback() — same as DrawInstanced**

```cpp
bool TestRenderingClient::Initialize_PixelReadback()
{
    return Initialize_DrawInstanced();
}
```

Reuse `Initialize_DrawInstanced()` — it just calls Initialize on shader/renderpass/commandlist.

- [ ] **Step 6: Add ExecuteCommands_PixelReadback()**

Render 2 frames to ensure the GPU has completed, then readback and validate on frame 2:

```cpp
bool TestRenderingClient::ExecuteCommands_PixelReadback()
{
    auto l_rs = g_Engine->getRenderingServer();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->Execute(l_cl, GPUEngineType::Graphics);
    l_rs->SignalOnGPU(l_rp, GPUEngineType::Graphics);

    ++m_FramesAfterLoad;

    if (m_FramesAfterLoad == k_TargetFrames)
    {
        // Wait for GPU to finish before reading back
        l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);

        auto* l_renderTarget = l_rp->m_OutputMergerTarget->m_ColorOutputs[0];
        auto  l_pixels       = l_rs->ReadTextureBackToCPU(l_rp, l_renderTarget);

        if (l_pixels.empty())
        {
            Log(Error, "TestRenderingClient: ReadTextureBackToCPU returned empty result.");
            m_ValidationPassed = false;
        }
        else
        {
            ValidatePixelReadback(l_renderTarget, l_pixels);
        }

        Log(Success, "TestRenderingClient (pixel_readback): completed. Validation: ",
            m_ValidationPassed ? "PASSED" : "FAILED");
        g_Engine->getWindowSystem()->Terminate();
    }

    return true;
}
```

- [ ] **Step 7: Add ValidatePixelReadback() helper**

Sample the center 32×32 region and assert all pixels are approximately red (R≈1.0, G≈0.0, B≈0.0).
The triangle (vertices at NDC -0.5,-0.5 / 0.0,0.5 / 0.5,-0.5) covers the lower-center and center
of the screen. A 32×32 sample centered at (128,128) in a 256×256 target is safely inside the triangle.

`ReadTextureBackToCPU` returns `Vec4` with channels already normalized to [0,1] (UByte8 path divides
by 255.0f). So R=1.0 means a full red byte, and the threshold ±0.1 accounts for any minor
format conversion rounding.

```cpp
void TestRenderingClient::ValidatePixelReadback(TextureComponent* rt, const std::vector<Vec4>& pixels)
{
    const uint32_t w    = rt->m_TextureDesc.Width;   // 256
    const uint32_t h    = rt->m_TextureDesc.Height;  // 256
    const uint32_t cx   = w / 2;                     // 128
    const uint32_t cy   = h / 2;                     // 128
    const uint32_t half = 16;                         // sample 32×32 center

    for (uint32_t y = cy - half; y < cy + half; ++y)
    {
        for (uint32_t x = cx - half; x < cx + half; ++x)
        {
            const auto& p = pixels[y * w + x];
            if (p.x < 0.9f || p.y > 0.1f || p.z > 0.1f)
            {
                Log(Error, "Pixel (", x, ",", y, ") expected red, got (",
                    p.x, ",", p.y, ",", p.z, ",", p.w, ")");
                m_ValidationPassed = false;
                return;  // report first failure only
            }
        }
    }
}
```

Note: declare `ValidatePixelReadback` as a private method in the header.

- [ ] **Step 8: Wire into the Setup/Initialize/PrepareCommands/ExecuteCommands/Terminate dispatch methods**

Add `PixelReadback` cases to each switch/if-else in the dispatch methods:

```cpp
// In Setup():
case TestCase::PixelReadback: return Setup_PixelReadback();

// In Initialize():
case TestCase::PixelReadback: return Initialize_PixelReadback();

// In PrepareCommands():
if (m_TestCase == TestCase::PixelReadback)
    return PrepareCommands_DrawInstanced();  // same commands

// In ExecuteCommands():
case TestCase::PixelReadback: return ExecuteCommands_PixelReadback();

// In Terminate():
if (m_TestCase == TestCase::PixelReadback)
    Terminate_DrawInstanced();  // same teardown
```

- [ ] **Step 9: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

- [ ] **Step 10: Commit**

```bash
git add Source/TestClient/TestRenderingClient.h
git add Source/TestClient/TestRenderingClient.cpp
git commit -m "feat: add pixel_readback test case — render red triangle and validate center pixels via ReadTextureBackToCPU"
```

---

## Task 3: Wire exit code 2 for validation failure

**Files:**
- Locate and modify the WinMain or main entry point used by RenderTest.exe

The RenderTest.exe currently exits with code 1 on D3D12 debug layer errors (from `HasGPUError()`). Pixel validation failures should exit with code 2.

- [ ] **Step 1: Find the RenderTest.exe entry point**

```
Source/Engine/Platform/WinMain/
```

Look for the file that calls `g_Engine->Run()` and checks `HasGPUError()`. This is the place to add the second check.

- [ ] **Step 2: Read the WinMain file to understand the current exit code logic**

The current logic is roughly:
```cpp
int exitCode = 0;
g_Engine->Run();
if (g_Engine->getRenderingServer()->HasGPUError())
    exitCode = 1;
return exitCode;
```

- [ ] **Step 3: Add the validation failure check**

The `TestRenderingClient` needs to be accessible from WinMain. Check how it's currently accessed (likely via `g_Engine->getLogicClient()` or a similar accessor that returns the ILogicClient interface). Cast it to `TestRenderingClient*` to call `GetValidationPassed()`.

If the interface doesn't expose `GetValidationPassed()`, add it to `ILogicClient` as a virtual method with default return `true`, overridden in `TestRenderingClient`.

```cpp
// After g_Engine->Run() completes:
if (g_Engine->getRenderingServer()->HasGPUError())
    exitCode = 1;
else if (auto* l_testClient = dynamic_cast<TestRenderingClient*>(g_Engine->getRenderingClient()))
{
    if (!l_testClient->GetValidationPassed())
        exitCode = 2;
}
```

Note: Check the exact accessor name for the rendering client (`getRenderingClient()` may differ). Read `Engine.h` for the correct API.

- [ ] **Step 4: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

- [ ] **Step 5: Run draw_instanced to verify it still exits 0**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: `0`

- [ ] **Step 6: Run pixel_readback**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test pixel_readback' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: `0` (validation passes — center of the screen is red from the triangle).

If exit code is `2`, the readback returned incorrect pixels. Debug by adding logging of the first few pixels in `ValidatePixelReadback`. Common causes:
- Wrong pixel format assumed in the unpack loop (check `TexturePixelDataType` of the default render target)
- Row pitch padding (DX12 textures have aligned row pitch; `f_ReadbackToHostHeap` should use the footprint pitch, not `width * pixelSize`)

> **Important**: Check the `f_ReadbackToHostHeap` lambda in the restored code — it uses `l_pixelCount * l_pixelDataSize` which assumes no row pitch padding. If the render target width is not a multiple of 256 bytes, rows will have padding. Fix by using `l_footprints[i].Footprint.RowPitch` for the stride when reading back. Read `DX12Helper_Texture.cpp::GetTexturePixelDataSize` to understand the exact layout.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Platform/WinMain/WinMain.cpp  # or the actual entry-point file path
git commit -m "feat: RenderTest exits code 2 on pixel validation failure"
```

---

## Final Verification

- [ ] Both test cases pass:

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test pixel_readback' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: both print `0`.

- [ ] Unit tests pass:

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```
