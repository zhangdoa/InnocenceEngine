#pragma once
#include "interface/ICoreManager.h"

#include "platform/InnoManagerHeader.h"

#ifdef INNO_PLATFORM_WIN32
#include "platform/InnoManagerHeaderWin32.h"
#endif

IMemoryManager* g_pMemoryManager;
ILogManager* g_pLogManager;
ITaskManager* g_pTaskManager;
ITimeManager* g_pTimeManager;
IRenderingManager* g_pRenderingManager;
IAssetManager* g_pAssetManager;
IGameManager* g_pGameManager;

class CoreManager : public ICoreManager
{
public:
	CoreManager() {};
	~CoreManager() {};
	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

protected:
	void setStatus(objectStatus objectStatus) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::vector<std::unique_ptr<IManager>> m_childEventManager;
};

CoreManager g_CoreManager;
ICoreManager* g_pCoreManager = &g_CoreManager;

