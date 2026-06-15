#pragma once
#include "../Interface/IService.h"
#include "ConfigurationService.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace Inno
{
    class AuditDumpService : public IService
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE_AND_NON_MOVABLE(AuditDumpService);

        bool Setup(IServiceConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

    private:
        void RunDump();

        bool m_Enabled = false;
        std::atomic<bool> m_SceneLoaded{false};
        uint32_t m_FrameCount = 0;
        uint32_t m_TriggerAtFrame = 0;
        std::vector<AuditPassEntry> m_Passes;

        ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
        std::function<void()> m_SceneLoadedCallback;
    };
}
