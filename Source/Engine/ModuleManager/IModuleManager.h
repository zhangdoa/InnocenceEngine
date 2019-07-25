#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Export/InnoEngine_Export.h"
#include "../Core/ITimeSystem.h"
#include "../Core/ILogSystem.h"
#include "../Core/IMemorySystem.h"
#include "../Core/ITaskSystem.h"
#include "../Core/ITestSystem.h"
#include "../FileSystem/IFileSystem.h"
#include "../EntityManager/IEntityManager.h"
#include "../ComponentManager/IComponentManager.h"
#include "../SceneHierarchyManager/ISceneHierarchyManager.h"
#include "../AssetSystem/IAssetSystem.h"
#include "../PhysicsSystem/IPhysicsSystem.h"
#include "../Core/IEventSystem.h"
#include "../Core/IWindowSystem.h"
#include "../RenderingFrontend/IRenderingFrontend.h"
#include "../RenderingBackend/IRenderingBackend.h"

#include "../Core/IRenderingClient.h"
#include "../Core/ILogicClient.h"

enum EngineMode { GAME, EDITOR };

enum RenderingBackend { GL, DX11, DX12, VK, MT };

struct InitConfig
{
	EngineMode engineMode = EngineMode::GAME;
	RenderingBackend renderingBackend = RenderingBackend::GL;
};

class IModuleManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IModuleManager);

	INNO_ENGINE_API virtual bool setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient) = 0;
	INNO_ENGINE_API virtual bool initialize() = 0;
	INNO_ENGINE_API virtual bool run() = 0;
	INNO_ENGINE_API virtual bool terminate() = 0;

	INNO_ENGINE_API virtual ObjectStatus getStatus() = 0;

	INNO_ENGINE_API virtual ITimeSystem* getTimeSystem() = 0;
	INNO_ENGINE_API virtual ILogSystem* getLogSystem() = 0;
	INNO_ENGINE_API virtual IMemorySystem* getMemorySystem() = 0;
	INNO_ENGINE_API virtual ITaskSystem* getTaskSystem() = 0;
	INNO_ENGINE_API virtual ITestSystem* getTestSystem() = 0;
	INNO_ENGINE_API virtual IFileSystem* getFileSystem() = 0;
	INNO_ENGINE_API virtual IEntityManager* getEntityManager() = 0;
	INNO_ENGINE_API virtual IComponentManager* getComponentManager(ComponentType componentType) = 0;
	INNO_ENGINE_API virtual ISceneHierarchyManager* getSceneHierarchyManager() = 0;
	INNO_ENGINE_API virtual IAssetSystem* getAssetSystem() = 0;
	INNO_ENGINE_API virtual IPhysicsSystem* getPhysicsSystem() = 0;
	INNO_ENGINE_API virtual IEventSystem* getEventSystem() = 0;
	INNO_ENGINE_API virtual IWindowSystem* getWindowSystem() = 0;
	INNO_ENGINE_API virtual IRenderingFrontend* getRenderingFrontend() = 0;
	INNO_ENGINE_API virtual IRenderingBackend* getRenderingBackend() = 0;

	INNO_ENGINE_API virtual InitConfig getInitConfig() = 0;

	INNO_ENGINE_API virtual float getTickTime() = 0;

	INNO_ENGINE_API virtual const FixedSizeString<128>& getApplicationName() = 0;
};
