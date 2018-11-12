#pragma once
#include "ICoreSystem.h"

class InnoCoreSystem : INNO_IMPLEMENT ICoreSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCoreSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT ITimeSystem* getTimeSystem() override;
	INNO_SYSTEM_EXPORT ILogSystem* getLogSystem() override;
	INNO_SYSTEM_EXPORT IMemorySystem* getMemorySystem() override;
	INNO_SYSTEM_EXPORT ITaskSystem* getTaskSystem() override;
	INNO_SYSTEM_EXPORT IGameSystem* getGameSystem() override;
	INNO_SYSTEM_EXPORT IAssetSystem* getAssetSystem() override;
	INNO_SYSTEM_EXPORT IPhysicsSystem* getPhysicsSystem() override;
	INNO_SYSTEM_EXPORT IVisionSystem* getVisionSystem() override;
};