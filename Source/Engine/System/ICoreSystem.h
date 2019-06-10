#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Export/InnoSystem_Export.h"
#include "Core/ITimeSystem.h"
#include "Core/ILogSystem.h"
#include "Core/IMemorySystem.h"
#include "Core/ITaskSystem.h"
#include "Core/ITestSystem.h"
#include "FileSystem/IFileSystem.h"
#include "GameSystem/IGameSystem.h"
#include "GameSystem/IAssetSystem.h"
#include "PhysicsSystem/IPhysicsSystem.h"
#include "GameSystem/IInputSystem.h"
#include "Core/IWindowSystem.h"
#include "RenderingFrontend/IRenderingFrontend.h"
#include "RenderingBackend/IRenderingBackend.h"
#include "../../Game/IGameInstance.h"

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

	INNO_SYSTEM_EXPORT virtual bool setup(void* appHook, void* extraHook, char* pScmdline, IGameInstance* gameInstance) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool run() = 0;
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
	INNO_SYSTEM_EXPORT virtual IRenderingFrontend* getRenderingFrontend() = 0;
	INNO_SYSTEM_EXPORT virtual IRenderingBackend* getRenderingBackend() = 0;

	INNO_SYSTEM_EXPORT virtual InitConfig getInitConfig() = 0;

	INNO_SYSTEM_EXPORT virtual float getTickTime() = 0;
};
