#pragma once
#include "interface/ICoreSystem.h"

#include "platform/InnoSystemHeader.h"

#ifdef INNO_PLATFORM_WIN32
#include "platform/InnoSystemHeaderWin32.h"
#endif

IMemorySystem* g_pMemorySystem;
ILogSystem* g_pLogSystem;
ITaskSystem* g_pTaskSystem;
ITimeSystem* g_pTimeSystem;
IRenderingSystem* g_pRenderingSystem;
IAssetSystem* g_pAssetSystem;
IGameSystem* g_pGameSystem;

class CoreSystem : public ICoreSystem
{
public:
	CoreSystem() {};
	~CoreSystem() {};
	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void taskTest();
};

CoreSystem g_CoreSystem;
ICoreSystem* g_pCoreSystem = &g_CoreSystem;

