#pragma once
#include "interface/ICoreSystem.h"

#include "platform/InnoSystemHeader.h"

IMemorySystem* g_pMemorySystem;
ILogSystem* g_pLogSystem;
INNO_TASK_SYSTEM* g_pTaskSystem;
ITimeSystem* g_pTimeSystem;
IGameSystem* g_pGameSystem;
IAssetSystem* g_pAssetSystem;
IVisionSystem* g_pVisionSystem;
IPhysicsSystem* g_pPhysicsSystem;

class CoreSystem : public ICoreSystem
{
public:
	CoreSystem() {};
	~CoreSystem() {};
#if defined(INNO_RENDERER_DX)
	void setup() override {};
	void setup(void* appInstance, char* commandLineArg, int showMethod) override;
#else
	void setup() override;
#endif
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

CoreSystem g_CoreSystem;
ICoreSystem* g_pCoreSystem = &g_CoreSystem;
