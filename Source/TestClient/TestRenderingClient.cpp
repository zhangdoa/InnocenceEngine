#include "TestRenderingClient.h"
#include "../Engine/Engine.h"
#include "../Engine/Interface/IWindowSystem.h"
#include "../Engine/RenderingServer/IRenderingServer.h"
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
    if (strcmp(name, "bareboot")       == 0) return TestCase::BareBoot;
    if (strcmp(name, "draw_instanced") == 0) return TestCase::DrawInstanced;
    return TestCase::Unknown;
}

bool TestRenderingClient::Setup(ISystemConfig*)
{
    m_TestCase = ParseTestCase(g_Engine->getInitConfig().testCase);

    switch (m_TestCase)
    {
    case TestCase::BareBoot:      return Setup_BareBoot();
    case TestCase::DrawInstanced: return Setup_DrawInstanced();
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
    default: return true;
    }
}

bool TestRenderingClient::Update() { return true; }

bool TestRenderingClient::PrepareCommands()
{
    if (m_TestCase == TestCase::DrawInstanced)
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
    default:
        return true;
    }
}

bool TestRenderingClient::Terminate()
{
    if (m_TestCase == TestCase::DrawInstanced)
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
        g_Engine->getWindowSystem()->Terminate();
    }
}

bool TestRenderingClient::Setup_BareBoot() { return true; }

bool TestRenderingClient::Setup_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();

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
    auto l_rs = g_Engine->getRenderingServer();
    l_rs->Initialize(m_DrawInstanced->ShaderProgram);
    l_rs->Initialize(m_DrawInstanced->RenderPass);
    l_rs->Initialize(m_DrawInstanced->CommandList);
    return true;
}

bool TestRenderingClient::PrepareCommands_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();
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
    auto l_rs = g_Engine->getRenderingServer();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->Execute(l_cl, GPUEngineType::Graphics);
    l_rs->SignalOnGPU(l_rp, GPUEngineType::Graphics);

    CountFrameAndTerminateIfDone();
    return true;
}

bool TestRenderingClient::Terminate_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();
    l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    l_rs->Delete(m_DrawInstanced->CommandList);
    l_rs->Delete(m_DrawInstanced->RenderPass);
    l_rs->Delete(m_DrawInstanced->ShaderProgram);
    return true;
}
