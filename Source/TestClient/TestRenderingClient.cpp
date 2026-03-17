#include "TestRenderingClient.h"
#include "../Engine/Engine.h"
#include "../Engine/Interface/IWindowSystem.h"
#include "../Engine/Services/SceneService.h"

using namespace Inno;

struct TestRenderingClient::DrawInstancedResources
{
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

bool TestRenderingClient::Setup_DrawInstanced()          { return true; }
bool TestRenderingClient::Initialize_DrawInstanced()     { return true; }
bool TestRenderingClient::PrepareCommands_DrawInstanced() { return true; }
bool TestRenderingClient::ExecuteCommands_DrawInstanced()
{
    CountFrameAndTerminateIfDone();
    return true;
}
bool TestRenderingClient::Terminate_DrawInstanced() { return true; }
