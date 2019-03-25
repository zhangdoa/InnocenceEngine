#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"
#include "../exports/InnoSystem_Export.h"
#include "ITimeSystem.h"
#include "ILogSystem.h"
#include "IMemorySystem.h"
#include "ITaskSystem.h"
#include "IFileSystem.h"
#include "IGameSystem.h"
#include "IAssetSystem.h"
#include "IPhysicsSystem.h"
#include "IInputSystem.h"
#include "IVisionSystem.h"

INNO_INTERFACE ICoreSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ICoreSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(void* hInstance, void* hwnd, char* pScmdline) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual ITimeSystem* getTimeSystem() = 0;
	INNO_SYSTEM_EXPORT virtual ILogSystem* getLogSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IMemorySystem* getMemorySystem() = 0;
	INNO_SYSTEM_EXPORT virtual ITaskSystem* getTaskSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IFileSystem* getFileSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IGameSystem* getGameSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IAssetSystem* getAssetSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IPhysicsSystem* getPhysicsSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IInputSystem* getInputSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IVisionSystem* getVisionSystem() = 0;
};