#pragma once
#include "../Engine/Interface/IRenderingClient.h"

namespace Inno
{
    class TestRenderingClient : public IRenderingClient
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
        enum class TestCase { BareBoot, DrawInstanced, Unknown };
        static TestCase ParseTestCase(const char* name);

        bool Setup_BareBoot();
        bool Setup_DrawInstanced();
        bool Initialize_DrawInstanced();
        bool PrepareCommands_DrawInstanced();
        bool ExecuteCommands_DrawInstanced();
        bool Terminate_DrawInstanced();

        void CountFrameAndTerminateIfDone();
        static constexpr uint32_t k_TargetFrames = 10;
        uint32_t m_FramesAfterLoad = 0;

        TestCase m_TestCase = TestCase::Unknown;
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;

        struct DrawInstancedResources;
        DrawInstancedResources* m_DrawInstanced = nullptr;
    };
}
