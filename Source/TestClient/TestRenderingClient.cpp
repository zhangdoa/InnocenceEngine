#include "TestRenderingClient.h"
#include "../Engine/Engine.h"
#include "../Engine/Interface/IWindowService.h"
#include "../Engine/RenderingServer/IGraphicsService.h"
#include "../Engine/Services/SceneService.h"
#include "../Engine/Services/RenderingConfigurationService.h"

using namespace Inno;

struct TestRenderingClient::DrawInstancedResources
{
    RenderPassComponent*    RenderPass    = nullptr;
    ShaderProgramComponent* ShaderProgram = nullptr;
    CommandListComponent*   CommandList   = nullptr;
};

TestRenderingClient::TestCase TestRenderingClient::ParseTestCase(const char* name)
{
    if (strcmp(name, "bareboot")        == 0) return TestCase::BareBoot;
    if (strcmp(name, "draw_instanced")  == 0) return TestCase::DrawInstanced;
    if (strcmp(name, "pixel_readback")  == 0) return TestCase::PixelReadback;
    return TestCase::Unknown;
}

bool TestRenderingClient::Setup(IServiceConfig*)
{
    m_TestCase = ParseTestCase(g_Engine->getInitConfig().testCase);

    switch (m_TestCase)
    {
    case TestCase::BareBoot:       return Setup_BareBoot();
    case TestCase::DrawInstanced:  return Setup_DrawInstanced();
    case TestCase::PixelReadback:  return Setup_PixelReadback();
    default:
        Log(Error, "TestRenderingClient: unknown test case '",
            g_Engine->getInitConfig().testCase, "'");
        return false;
    }
}

bool TestRenderingClient::Initialize()
{
    m_ObjectStatus = ObjectStatus::Activated;
    switch (m_TestCase)
    {
    case TestCase::DrawInstanced: return Initialize_DrawInstanced();
    case TestCase::PixelReadback: return Initialize_PixelReadback();
    default: return true;
    }
}

bool TestRenderingClient::Update() { return true; }

bool TestRenderingClient::PrepareCommands()
{
    if (m_TestCase == TestCase::DrawInstanced || m_TestCase == TestCase::PixelReadback)
        return PrepareCommands_DrawInstanced();
    return true;
}

bool TestRenderingClient::ExecuteCommands(IRenderingConfig*)
{
    switch (m_TestCase)
    {
    case TestCase::BareBoot:
        CountFrameAndTerminateIfDone();
        return true;
    case TestCase::DrawInstanced:
        return ExecuteCommands_DrawInstanced();
    case TestCase::PixelReadback:
        return ExecuteCommands_PixelReadback();
    default:
        return true;
    }
}

bool TestRenderingClient::Terminate()
{
    if (m_TestCase == TestCase::DrawInstanced || m_TestCase == TestCase::PixelReadback)
        Terminate_DrawInstanced();
    delete m_DrawInstanced;
    m_DrawInstanced = nullptr;
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestRenderingClient::GetStatus() { return m_ObjectStatus; }

void TestRenderingClient::CountFrameAndTerminateIfDone()
{
    if (g_Engine->Get<SceneService>()->IsLoading())
        return;

    ++m_FramesAfterLoad;
    if (m_FramesAfterLoad >= k_TargetFrames)
    {
        Log(Success, "TestRenderingClient: completed ", m_FramesAfterLoad,
            " frames. Terminating.");
        g_Engine->getWindowService()->Terminate();
    }
}

bool TestRenderingClient::Setup_BareBoot() { return true; }

bool TestRenderingClient::Setup_DrawInstanced()
{
    auto l_rs = g_Engine->getGraphicsService();

    m_DrawInstanced = new DrawInstancedResources();

    m_DrawInstanced->ShaderProgram = l_rs->AddShaderProgramComponent("TestDrawInstanced/");
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_VSPath = "drawInstanced.vert/";
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_PSPath = "drawInstanced.frag/";

    m_DrawInstanced->RenderPass = l_rs->AddRenderPassComponent("TestDrawInstanced/");

    auto l_desc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
    l_desc.m_RenderTargetCount = 1;
    l_desc.m_UseDepthBuffer    = false;
    l_desc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_PrimitiveTopology = PrimitiveTopology::Point;

    m_DrawInstanced->RenderPass->m_RenderPassDesc = l_desc;
    m_DrawInstanced->RenderPass->m_ShaderProgram  = m_DrawInstanced->ShaderProgram;

    m_DrawInstanced->CommandList = l_rs->AddCommandListComponent("TestDrawInstanced/Graphics/");
    m_DrawInstanced->CommandList->m_Type = GPUEngineType::Graphics;

    return true;
}

bool TestRenderingClient::Initialize_DrawInstanced()
{
    auto l_rs = g_Engine->getGraphicsService();
    l_rs->Initialize(m_DrawInstanced->ShaderProgram);
    l_rs->Initialize(m_DrawInstanced->RenderPass);
    l_rs->Initialize(m_DrawInstanced->CommandList);
    return true;
}

bool TestRenderingClient::PrepareCommands_DrawInstanced()
{
    auto l_rs = g_Engine->getGraphicsService();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->CommandListBegin(l_rp, l_cl, l_rs->GetCurrentFrame());
    l_rs->BindRenderPassComponent(l_rp, l_cl);
    l_rs->ClearRenderTargets(l_rp, l_cl);
    l_rs->DrawInstanced(l_rp, l_cl, 3);
    l_rs->CommandListEnd(l_rp, l_cl);

    return true;
}

bool TestRenderingClient::ExecuteCommands_DrawInstanced()
{
    auto l_rs = g_Engine->getGraphicsService();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->Execute(l_cl, GPUEngineType::Graphics);
    l_rs->SignalOnGPU(l_rp, GPUEngineType::Graphics);

    CountFrameAndTerminateIfDone();
    return true;
}

bool TestRenderingClient::Terminate_DrawInstanced()
{
    auto l_rs = g_Engine->getGraphicsService();
    l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    l_rs->Delete(m_DrawInstanced->CommandList);
    l_rs->Delete(m_DrawInstanced->RenderPass);
    l_rs->Delete(m_DrawInstanced->ShaderProgram);
    return true;
}

bool TestRenderingClient::Setup_PixelReadback()
{
    auto l_rs = g_Engine->getGraphicsService();

    m_DrawInstanced = new DrawInstancedResources();

    m_DrawInstanced->ShaderProgram = l_rs->AddShaderProgramComponent("TestPixelReadback/");
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_VSPath = "drawInstanced.vert/";
    m_DrawInstanced->ShaderProgram->m_ShaderFilePaths.m_PSPath = "drawInstanced.frag/";

    m_DrawInstanced->RenderPass = l_rs->AddRenderPassComponent("TestPixelReadback/");

    auto l_desc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
    l_desc.m_RenderTargetCount = 1;
    l_desc.m_UseDepthBuffer    = false;
    l_desc.m_RenderTargetDesc.Width           = 256;
    l_desc.m_RenderTargetDesc.Height          = 256;
    l_desc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
    l_desc.m_RenderTargetDesc.PixelDataType   = TexturePixelDataType::UByte;
    l_desc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width  = 256.0f;
    l_desc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 256.0f;
    l_desc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_PrimitiveTopology = PrimitiveTopology::Point;

    m_DrawInstanced->RenderPass->m_RenderPassDesc = l_desc;
    m_DrawInstanced->RenderPass->m_ShaderProgram  = m_DrawInstanced->ShaderProgram;

    m_DrawInstanced->CommandList = l_rs->AddCommandListComponent("TestPixelReadback/Graphics/");
    m_DrawInstanced->CommandList->m_Type = GPUEngineType::Graphics;

    return true;
}

bool TestRenderingClient::Initialize_PixelReadback()
{
    return Initialize_DrawInstanced();
}

bool TestRenderingClient::ExecuteCommands_PixelReadback()
{
    auto l_rs = g_Engine->getGraphicsService();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->Execute(l_cl, GPUEngineType::Graphics);
    l_rs->SignalOnGPU(l_rp, GPUEngineType::Graphics);

    if (g_Engine->Get<SceneService>()->IsLoading())
        return true;

    ++m_FramesAfterLoad;

    if (m_FramesAfterLoad == k_TargetFrames)
    {
        l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);

        auto* l_renderTarget = l_rp->m_OutputMergerTarget->m_ColorOutputs[0];
        auto l_pixels = l_rs->ReadTextureBackToCPU(l_rp, l_renderTarget);

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
        g_Engine->getWindowService()->Terminate();
    }

    return true;
}

void TestRenderingClient::ValidatePixelReadback(TextureComponent* rt, const std::vector<Vec4>& pixels)
{
    const uint32_t l_w = rt->m_TextureDesc.Width;
    const uint32_t l_h = rt->m_TextureDesc.Height;

    if (pixels.size() < static_cast<size_t>(l_w) * l_h)
    {
        Log(Error, "ValidatePixelReadback: pixel buffer too small (", pixels.size(),
            " < ", l_w * l_h, ")");
        m_ValidationPassed = false;
        return;
    }

    // drawInstanced.vert uses SV_InstanceID (1 vertex/instance) with PointList topology.
    // 3 instances produce 3 points at NDC: (-0.5,-0.5), (0,0.5), (0.5,-0.5).
    // Screen mapping for 256x256 viewport: (w/4, 3h/4), (w/2, h/4), (3w/4, 3h/4).
    struct Point { uint32_t x, y; };
    const Point l_pts[3] = {
        { l_w / 4,     l_h * 3 / 4 },
        { l_w / 2,     l_h / 4     },
        { l_w * 3 / 4, l_h * 3 / 4 },
    };

    const uint32_t l_radius = 3;

    for (const auto& l_pt : l_pts)
    {
        bool l_found = false;
        for (uint32_t y = l_pt.y - l_radius; y <= l_pt.y + l_radius && !l_found; ++y)
        {
            for (uint32_t x = l_pt.x - l_radius; x <= l_pt.x + l_radius && !l_found; ++x)
            {
                const auto& l_p = pixels[y * l_w + x];
                if (l_p.x > 0.9f && l_p.y < 0.1f && l_p.z < 0.1f)
                    l_found = true;
            }
        }
        if (!l_found)
        {
            const auto& l_p = pixels[l_pt.y * l_w + l_pt.x];
            Log(Error, "Expected red pixel near (", l_pt.x, ",", l_pt.y, "), got (",
                l_p.x, ",", l_p.y, ",", l_p.z, ",", l_p.w, ")");
            m_ValidationPassed = false;
            return;
        }
    }
}
