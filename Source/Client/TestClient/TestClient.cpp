#include "TestClient.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Engine.h"
#include "../../Engine/Interface/IWindowSystem.h"

using namespace Inno;

static constexpr uint32_t k_MinFramesAfterLoad = 10;
static constexpr float    k_TimeoutSeconds      = 30.0f;

bool TestClient::Setup(ISystemConfig*)
{
    m_ObjectStatus = ObjectStatus::Activated;
    Log(Success, "TestClient: Setup complete.");
    return true;
}

bool TestClient::Initialize()
{
    m_StartTime = std::chrono::steady_clock::now();
    Log(Success, "TestClient: Initialized. Will terminate after ", k_MinFramesAfterLoad, " frames post-load or ", k_TimeoutSeconds, "s timeout.");
    return true;
}

bool TestClient::Update()
{
    if (m_ShouldTerminate)
        g_Engine->getWindowSystem()->Terminate();
    return true;
}

bool TestClient::PrepareCommands()
{
    return true;
}

bool TestClient::ExecuteCommands(IRenderingConfig*)
{
    ++m_FrameCount;

    auto elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - m_StartTime).count();

    if (elapsed >= k_TimeoutSeconds)
    {
        Log(Warning, "TestClient: Timeout reached (", elapsed, "s). Terminating.");
        m_ShouldTerminate = true;
        return true;
    }

    bool isLoading = g_Engine->Get<SceneService>()->IsLoading();
    if (!isLoading)
        ++m_FramesAfterLoad;

    if (m_FramesAfterLoad >= k_MinFramesAfterLoad)
    {
        Log(Success, "TestClient: Completed ", m_FrameCount, " total frames, ",
            m_FramesAfterLoad, " post-load. Terminating cleanly.");
        m_ShouldTerminate = true;
    }

    return true;
}

bool TestClient::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestClient::GetStatus()
{
    return m_ObjectStatus;
}
