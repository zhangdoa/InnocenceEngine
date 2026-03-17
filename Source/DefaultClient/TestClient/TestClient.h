#pragma once
#include "../../Engine/Interface/IRenderingClient.h"
#include "../../Engine/Common/STL14.h"

namespace Inno
{
    class TestClient : public IRenderingClient
    {
    public:
        bool Setup(ISystemConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool PrepareCommands() override;
        bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

    private:
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;
        uint32_t m_FrameCount = 0;
        uint32_t m_FramesAfterLoad = 0;
        std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
        bool m_ShouldTerminate = false;
    };
}
