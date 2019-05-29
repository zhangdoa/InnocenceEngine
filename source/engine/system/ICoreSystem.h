#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"
#include "../exports/InnoSystem_Export.h"
#include "Core/ITimeSystem.h"
#include "Core/ILogSystem.h"
#include "Core/IMemorySystem.h"
#include "Core/ITaskSystem.h"
#include "ITestSystem.h"
#include "IFileSystem.h"
#include "IGameSystem.h"
#include "IAssetSystem.h"
#include "IPhysicsSystem.h"
#include "IInputSystem.h"
#include "IWindowSystem.h"
#include "IRenderingFrontendSystem.h"
#include "IRenderingBackendSystem.h"

enum EngineMode { GAME, EDITOR };
enum RenderingBackend { GL, DX11, DX12, VK, MT };
struct InitConfig
{
	EngineMode engineMode = EngineMode::GAME;
	RenderingBackend renderingBackend = RenderingBackend::GL;
};

INNO_INTERFACE ICoreSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ICoreSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(void* appHook, void* extraHook, char* pScmdline) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual ITimeSystem* getTimeSystem() = 0;
	INNO_SYSTEM_EXPORT virtual ILogSystem* getLogSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IMemorySystem* getMemorySystem() = 0;
	INNO_SYSTEM_EXPORT virtual ITaskSystem* getTaskSystem() = 0;
	INNO_SYSTEM_EXPORT virtual ITestSystem* getTestSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IFileSystem* getFileSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IGameSystem* getGameSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IAssetSystem* getAssetSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IPhysicsSystem* getPhysicsSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IInputSystem* getInputSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IWindowSystem* getWindowSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IRenderingFrontendSystem* getRenderingFrontendSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IRenderingBackendSystem* getRenderingBackendSystem() = 0;

	INNO_SYSTEM_EXPORT virtual InitConfig getInitConfig() = 0;

	INNO_SYSTEM_EXPORT virtual float getTickTime() = 0;
};
