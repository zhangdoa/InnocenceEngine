#pragma once
#include "../Interface/IService.h"

#include <cstdint>

namespace Inno
{
    class GPUResourceComponent;
    class RenderPassComponent;

    class ScreenCaptureService : public IService
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(ScreenCaptureService);

        bool Setup(IServiceConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;



        void RequestCapture();
        void TryWriteAutoCapture();

    private:
        void ResolveCanvas();
        void HandleScreenCapture();
        void HandleAutoCaptureTriggers();

        void AlignTrackerForMidFrameReadback();

        bool WriteCaptureToFile(const char* filename);

        bool IsSerializeTestMode() const;
        uint32_t ResolveTriggerFrame() const;
        bool IsInsideDumpWindow(uint32_t frameCount) const;
        void RunDumpFrame(uint32_t frameCount);
        void RunOneShotIfReady(uint32_t triggerAtFrame);

        bool m_saveScreenCapture = false;

        uint32_t m_autoCaptureFrameCount = 0;
        bool m_autoCaptureWritten = false;

        GPUResourceComponent* m_Canvas = nullptr;
        RenderPassComponent* m_CanvasOwner = nullptr;

        ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
    };
}
