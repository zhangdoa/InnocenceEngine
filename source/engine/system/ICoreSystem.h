#pragma once
#include "../common/InnoClassTemplate.h"
#include "../exports/InnoSystem_Export.h"
#include "ITimeSystem.h"
#include "ILogSystem.h"
#include "IMemorySystem.h"
#include "ITaskSystem.h"
#include "IGameSystem.h"
#include "IAssetSystem.h"
#include "IPhysicsSystem.h"
#include "IVisionSystem.h"

INNO_INTERFACE ICoreSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ICoreSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual ITimeSystem* getTimeSystem() = 0;
	INNO_SYSTEM_EXPORT virtual ILogSystem* getLogSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IMemorySystem* getMemorySystem() = 0;
	INNO_SYSTEM_EXPORT virtual ITaskSystem* getTaskSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IGameSystem* getGameSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IAssetSystem* getAssetSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IPhysicsSystem* getPhysicsSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IVisionSystem* getVisionSystem() = 0;
};