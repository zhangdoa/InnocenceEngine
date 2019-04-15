#pragma once
#include "ICoreSystem.h"

INNO_CONCRETE InnoCoreSystem : INNO_IMPLEMENT ICoreSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCoreSystem);

	INNO_SYSTEM_EXPORT bool setup(void* appHook, void* extraHook, char* pScmdline) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT ITimeSystem* getTimeSystem() override;
	INNO_SYSTEM_EXPORT ILogSystem* getLogSystem() override;
	INNO_SYSTEM_EXPORT IMemorySystem* getMemorySystem() override;
	INNO_SYSTEM_EXPORT ITaskSystem* getTaskSystem() override;
	INNO_SYSTEM_EXPORT ITestSystem* getTestSystem() override;
	INNO_SYSTEM_EXPORT IFileSystem* getFileSystem() override;
	INNO_SYSTEM_EXPORT IGameSystem* getGameSystem() override;
	INNO_SYSTEM_EXPORT IAssetSystem* getAssetSystem() override;
	INNO_SYSTEM_EXPORT IPhysicsSystem* getPhysicsSystem() override;
	INNO_SYSTEM_EXPORT IInputSystem* getInputSystem() override;
	INNO_SYSTEM_EXPORT IVisionSystem* getVisionSystem() override;
};
