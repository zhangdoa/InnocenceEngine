#pragma once
#include "../Engine/Interface/ILogicClient.h"

namespace Inno
{
    class TestLogicClient : public ILogicClient
    {
    public:
        bool Setup(ISystemConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;
        const char* GetApplicationName() override;

    private:
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;
    };
}
