#include "../../Engine/Engine.h"

using namespace Inno;
;

#include "World.inl"

namespace Inno
{
    class ExampleLogicClientImpl: public ILogicClient
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleLogicClientImpl);

        bool Setup(IServiceConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const char* GetApplicationName() override;

    private:
    	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

        WorldSystem* m_world = nullptr;
    };

    bool ExampleLogicClientImpl::Setup(IServiceConfig* systemConfig)
    {
        if(!m_world)
            m_world = new WorldSystem();

        if(m_world->Setup(systemConfig))
        {
            m_ObjectStatus = ObjectStatus::Created;
            return true;
        }

        return false;
    }

    bool ExampleLogicClientImpl::Initialize()
    {
        if(m_world->Initialize())
        {
            m_ObjectStatus = ObjectStatus::Activated;
            return true;
        }

        return false;
    }

    bool ExampleLogicClientImpl::Update()
    {
        return m_world->Update();
    }

    bool ExampleLogicClientImpl::Terminate()
    {
        if (m_world->Terminate())
            delete m_world;

        ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
        return true;
    }

    ObjectStatus ExampleLogicClientImpl::GetStatus()
    {
        return m_ObjectStatus;
    }

    const char* ExampleLogicClientImpl::GetApplicationName()
    {
        return "LogicClient/";
    }
}