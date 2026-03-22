#pragma once
#include "../Engine/Interface/IRenderingClient.h"
#include "../Engine/RenderingServer/IRenderingServer.h"

namespace Inno
{
    class TestRenderingClient : public IRenderingClient
    {
    public:
        bool Setup(IServiceConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool PrepareCommands() override;
        bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

        bool GetValidationPassed() const override { return m_ValidationPassed; }

    private:
        enum class TestCase { Unknown, BareBoot, DrawInstanced, PixelReadback };
        static TestCase ParseTestCase(const char* name);

        bool Setup_BareBoot();
        bool Setup_DrawInstanced();
        bool Initialize_DrawInstanced();
        bool PrepareCommands_DrawInstanced();
        bool ExecuteCommands_DrawInstanced();
        bool Terminate_DrawInstanced();

        bool Setup_PixelReadback();
        bool Initialize_PixelReadback();
        bool ExecuteCommands_PixelReadback();
        void ValidatePixelReadback(TextureComponent* rt, const std::vector<Vec4>& pixels);

        void CountFrameAndTerminateIfDone();
        static constexpr uint32_t k_TargetFrames = 10;
        uint32_t m_FramesAfterLoad = 0;

        bool m_ValidationPassed = true;

        TestCase m_TestCase = TestCase::Unknown;
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;

        struct DrawInstancedResources;
        DrawInstancedResources* m_DrawInstanced = nullptr;
    };
}
