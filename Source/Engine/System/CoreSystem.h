#pragma once
#include "ICoreSystem.h"

INNO_CONCRETE InnoCoreSystem : INNO_IMPLEMENT ICoreSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCoreSystem);

	bool setup(void* appHook, void* extraHook, char* pScmdline) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	ITimeSystem* getTimeSystem() override;
	ILogSystem* getLogSystem() override;
	IMemorySystem* getMemorySystem() override;
	ITaskSystem* getTaskSystem() override;
	ITestSystem* getTestSystem() override;
	IFileSystem* getFileSystem() override;
	IGameSystem* getGameSystem() override;
	IAssetSystem* getAssetSystem() override;
	IPhysicsSystem* getPhysicsSystem() override;
	IInputSystem* getInputSystem() override;
	IWindowSystem* getWindowSystem() override;
	IRenderingFrontend* getRenderingFrontend() override;
	IRenderingBackend* getRenderingBackend() override;

	InitConfig getInitConfig() override;
	float getTickTime() override;
};
