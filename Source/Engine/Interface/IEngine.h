#pragma once
#include "../Common/Type.h"
#include "../Common/ClassTemplate.h"
#include "../Export/Engine_Export.h"
#include "ITimeSystem.h"
#include "ILogSystem.h"
#include "IMemorySystem.h"
#include "ITaskSystem.h"
#include "ITestSystem.h"
#include "IFileSystem.h"
#include "../EntityManager/IEntityManager.h"
#include "../Template/ComponentManager.h"
#include "ISceneSystem.h"
#include "IAssetSystem.h"
#include "IPhysicsSystem.h"
#include "IEventSystem.h"
#include "IWindowSystem.h"
#include "../RenderingFrontend/IRenderingFrontend.h"
#include "../RenderingFrontend/IGUISystem.h"
#include "../RenderingServer/IRenderingServer.h"

#include "IRenderingClient.h"
#include "ILogicClient.h"

namespace Inno
{
	enum EngineMode { Host, Slave };

	enum RenderingServer { GL, DX11, DX12, VK, MT };

	struct InitConfig
	{
		EngineMode engineMode = EngineMode::Host;
		RenderingServer renderingServer = RenderingServer::GL;
		LogLevel logLevel = LogLevel::Success;
	};

	class IEngine
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IEngine);

		INNO_ENGINE_API virtual bool Setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient) = 0;
		INNO_ENGINE_API virtual bool Initialize() = 0;
		INNO_ENGINE_API virtual bool Update() = 0;
		INNO_ENGINE_API virtual bool Terminate() = 0;

		INNO_ENGINE_API virtual bool Run() = 0;

		INNO_ENGINE_API virtual ObjectStatus GetStatus() = 0;

		INNO_ENGINE_API virtual ITimeSystem* getTimeSystem() = 0;
		INNO_ENGINE_API virtual ILogSystem* getLogSystem() = 0;
		INNO_ENGINE_API virtual IMemorySystem* getMemorySystem() = 0;
		INNO_ENGINE_API virtual ITaskSystem* getTaskSystem() = 0;
		INNO_ENGINE_API virtual ITestSystem* getTestSystem() = 0;
		INNO_ENGINE_API virtual IFileSystem* getFileSystem() = 0;
		INNO_ENGINE_API virtual IEntityManager* getEntityManager() = 0;
		INNO_ENGINE_API virtual ComponentManager* getComponentManager() = 0;
		INNO_ENGINE_API virtual ISceneSystem* getSceneSystem() = 0;
		INNO_ENGINE_API virtual IAssetSystem* getAssetSystem() = 0;
		INNO_ENGINE_API virtual IPhysicsSystem* getPhysicsSystem() = 0;
		INNO_ENGINE_API virtual IEventSystem* getEventSystem() = 0;
		INNO_ENGINE_API virtual IWindowSystem* getWindowSystem() = 0;
		INNO_ENGINE_API virtual IRenderingFrontend* getRenderingFrontend() = 0;
		INNO_ENGINE_API virtual IGUISystem* getGUISystem() = 0;
		INNO_ENGINE_API virtual IRenderingServer* getRenderingServer() = 0;

		INNO_ENGINE_API virtual InitConfig getInitConfig() = 0;

		INNO_ENGINE_API virtual float getTickTime() = 0;

		INNO_ENGINE_API virtual const FixedSizeString<128>& getApplicationName() = 0;
	};
}
