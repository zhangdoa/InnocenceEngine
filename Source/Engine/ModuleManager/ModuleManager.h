#pragma once
#include "../Interface/IModuleManager.h"

class InnoModuleManager : public IModuleManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoModuleManager);

	bool Setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient) override;
	bool Initialize() override;
	bool Run() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	ITimeSystem* getTimeSystem() override;
	ILogSystem* getLogSystem() override;
	IMemorySystem* getMemorySystem() override;
	ITaskSystem* getTaskSystem() override;
	ITestSystem* getTestSystem() override;
	IFileSystem* getFileSystem() override;
	IEntityManager* getEntityManager() override;
	ComponentManager* getComponentManager() override;
	ISceneSystem* getSceneSystem() override;
	IAssetSystem* getAssetSystem() override;
	IPhysicsSystem* getPhysicsSystem() override;
	IEventSystem* getEventSystem() override;
	IWindowSystem* getWindowSystem() override;
	IRenderingFrontend* getRenderingFrontend() override;
	IGUISystem* getGUISystem() override;
	IRenderingServer* getRenderingServer() override;

	InitConfig getInitConfig() override;
	float getTickTime() override;
	const FixedSizeString<128>& getApplicationName() override;
};