#include "ModuleManager.h"
#include "../Core/InnoLogger.h"
#include "../SubSystem/TimeSystem.h"
#include "../SubSystem/LogSystem.h"
#include "../SubSystem/MemorySystem.h"
#include "../SubSystem/TaskSystem.h"
#include "../SubSystem/TestSystem.h"
#include "../SubSystem/FileSystem.h"
#include "../EntityManager/EntityManager.h"
#include "../ComponentManager/TransformComponentManager.h"
#include "../ComponentManager/VisibleComponentManager.h"
#include "../ComponentManager/LightComponentManager.h"
#include "../ComponentManager/CameraComponentManager.h"
#include "../SceneHierarchyManager/SceneHierarchyManager.h"
#include "../SubSystem/AssetSystem.h"
#include "../SubSystem/PhysicsSystem.h"
#include "../SubSystem/EventSystem.h"
#include "../RenderingFrontend/RenderingFrontend.h"
#include "../RenderingFrontend/GUISystem.h"

#if defined INNO_PLATFORM_WIN
#include "../Platform/WinWindow/WinWindowSystem.h"
#endif

#if defined INNO_PLATFORM_MAC
#include "../Platform/MacWindow/MacWindowSystem.h"
#endif

#if defined INNO_PLATFORM_LINUX
#include "../Platform/LinuxWindow/LinuxWindowSystem.h"
#endif

#if defined INNO_RENDERER_DIRECTX
#include "../RenderingServer/DX11/DX11RenderingServer.h"
#include "../RenderingServer/DX12/DX12RenderingServer.h"
#endif

#if defined INNO_RENDERER_OPENGL
#include "../RenderingServer/GL/GLRenderingServer.h"
#endif

#if defined INNO_RENDERER_VULKAN
#include "../RenderingServer/VK/VKRenderingServer.h"
#endif

#if defined INNO_RENDERER_METAL
#include "../RenderingServer/MT/MTRenderingServer.h"
#endif

INNO_ENGINE_API IModuleManager* g_pModuleManager;

#define createSubSystemInstanceDefi( className ) \
m_##className = std::make_unique<Inno##className>(); \
if (!m_##className.get()) \
{ \
	return false; \
} \

#define subSystemSetup( className ) \
if (!g_pModuleManager->get##className()->setup()) \
{ \
	return false; \
} \
InnoLogger::Log(LogLevel::Success, "ModuleManager: ", #className, " setup finished."); \

#define subSystemInit( className ) \
if (!g_pModuleManager->get##className()->initialize()) \
{ \
	return false; \
} \

#define subSystemUpdate( className ) \
if (!g_pModuleManager->get##className()->update()) \
{ \
m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define subSystemTerm( className ) \
if (!g_pModuleManager->get##className()->terminate()) \
{ \
	return false; \
} \

#define subSystemGetDefi( className ) \
I##className * InnoModuleManager::get##className() \
{ \
	return m_##className.get(); \
} \

#define ComponentManagerSetup( className ) \
if (!m_##className##Manager->Setup()) \
{ \
	return false; \
} \
InnoLogger::Log(LogLevel::Success, "ModuleManager: ", #className, " setup finished."); \

#define ComponentManagerInit( className ) \
if (!m_##className##Manager->Initialize()) \
{ \
	return false; \
} \

#define ComponentManagerSimulate( className ) \
if (!m_##className##Manager->Simulate()) \
{ \
m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define ComponentManagerPostFrame( className ) \
if (!m_##className##Manager->PostFrame()) \
{ \
m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define ComponentManagerTerm( className ) \
if (!m_##className##Manager->Terminate()) \
{ \
	return false; \
} \

namespace InnoModuleManagerNS
{
	InitConfig parseInitConfig(const std::string& arg);
	bool createSubSystemInstance(void* appHook, void* extraHook, char* pScmdline);
	bool setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient);
	bool initialize();
	bool update();
	bool terminate();

	InitConfig m_initConfig;

	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
	std::unique_ptr<ITaskSystem> m_TaskSystem;
	std::unique_ptr<ITestSystem> m_TestSystem;

	std::unique_ptr<IFileSystem> m_FileSystem;

	std::unique_ptr<IEntityManager> m_EntityManager;
	std::unique_ptr<ITransformComponentManager> m_TransformComponentManager;
	std::unique_ptr<IVisibleComponentManager> m_VisibleComponentManager;
	std::unique_ptr<ILightComponentManager> m_LightComponentManager;
	std::unique_ptr<ICameraComponentManager> m_CameraComponentManager;

	std::unique_ptr<ISceneHierarchyManager> m_SceneHierarchyManager;
	std::unique_ptr<IAssetSystem> m_AssetSystem;
	std::unique_ptr<IPhysicsSystem> m_PhysicsSystem;
	std::unique_ptr<IEventSystem> m_EventSystem;
	std::unique_ptr<IWindowSystem> m_WindowSystem;
	std::unique_ptr<IRenderingFrontend> m_RenderingFrontend;
	std::unique_ptr<IGUISystem> m_GUISystem;
	std::unique_ptr<IRenderingServer> m_RenderingServer;

	IRenderingClient* m_RenderingClient;
	ILogicClient* m_LogicClient;

	FixedSizeString<128> m_applicationName;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_allowRender = false;

	std::function<void()> f_LogicClientUpdateJob;
	std::function<void()> f_PhysicsSystemUpdateBVHJob;
	std::function<void()> f_PhysicsSystemCullingJob;
	std::function<void()> f_RenderingFrontendUpdateJob;
	std::function<void()> f_RenderingServerUpdateJob;

	float m_tickTime = 0;
}

using namespace InnoModuleManagerNS;

InitConfig InnoModuleManagerNS::parseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		InnoLogger::Log(LogLevel::Warning, "ModuleManager: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		InnoLogger::Log(LogLevel::Warning, "ModuleManager: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::Host;
			InnoLogger::Log(LogLevel::Success, "ModuleManager: Launch in host mode, engine will handle OS event.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::Slave;
			InnoLogger::Log(LogLevel::Success, "ModuleManager: Launch in slave mode, engine requires client handle OS event.");
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: Unsupported engine mode.");
		}
	}

	auto l_renderingServerArgPos = arg.find("renderer");

	if (l_renderingServerArgPos == std::string::npos)
	{
		InnoLogger::Log(LogLevel::Warning, "ModuleManager: No rendering backend argument found, use default OpenGL rendering backend.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_renderingServerArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if defined INNO_RENDERER_OPENGL
			l_result.renderingServer = RenderingServer::GL;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX11;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX12;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingServer = RenderingServer::VK;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_RENDERER_METAL
			l_result.renderingServer = RenderingServer::MT;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: Unsupported rendering backend, use default OpenGL rendering backend.");
		}
	}

	auto l_logLevelArgPos = arg.find("loglevel");
	if (l_engineModeArgPos == std::string::npos)
	{
		InnoLogger::SetDefaultLogLevel(LogLevel::Success);
	}
	else
	{
		std::string l_logLevelArguments = arg.substr(l_logLevelArgPos + 9);
		l_logLevelArguments = l_logLevelArguments.substr(0, 1);

		if (l_logLevelArguments == "0")
		{
			InnoLogger::SetDefaultLogLevel(LogLevel::Verbose);
		}
		else if (l_logLevelArguments == "1")
		{
			InnoLogger::SetDefaultLogLevel(LogLevel::Success);
		}
		else if (l_logLevelArguments == "2")
		{
			InnoLogger::SetDefaultLogLevel(LogLevel::Warning);
		}
		else if (l_logLevelArguments == "3")
		{
			InnoLogger::SetDefaultLogLevel(LogLevel::Error);
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: Unsupported log level.");
		}
	}

	return l_result;
}

bool InnoModuleManagerNS::createSubSystemInstance(void* appHook, void* extraHook, char* pScmdline)
{
	createSubSystemInstanceDefi(TimeSystem);
	createSubSystemInstanceDefi(LogSystem);
	createSubSystemInstanceDefi(MemorySystem);
	createSubSystemInstanceDefi(TaskSystem);

	createSubSystemInstanceDefi(TestSystem);
	createSubSystemInstanceDefi(FileSystem);

	createSubSystemInstanceDefi(EntityManager);
	createSubSystemInstanceDefi(TransformComponentManager);
	createSubSystemInstanceDefi(VisibleComponentManager);
	createSubSystemInstanceDefi(LightComponentManager);
	createSubSystemInstanceDefi(CameraComponentManager);

	createSubSystemInstanceDefi(SceneHierarchyManager);
	createSubSystemInstanceDefi(AssetSystem);
	createSubSystemInstanceDefi(PhysicsSystem);
	createSubSystemInstanceDefi(EventSystem);

	std::string l_windowArguments = pScmdline;
	m_initConfig = parseInitConfig(l_windowArguments);

#if defined INNO_PLATFORM_WIN
	m_WindowSystem = std::make_unique<WinWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif
#if defined INNO_PLATFORM_MAC
	m_WindowSystem = std::make_unique<MacWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif
#if defined INNO_PLATFORM_LINUX
	m_WindowSystem = std::make_unique<LinuxWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif

	m_RenderingFrontend = std::make_unique<InnoRenderingFrontend>();
	if (!m_RenderingFrontend.get())
	{
		return false;
	}

	m_GUISystem = std::make_unique<InnoGUISystem>();
	if (!m_GUISystem.get())
	{
		return false;
	}

	switch (m_initConfig.renderingServer)
	{
	case RenderingServer::GL:
#if defined INNO_RENDERER_OPENGL
		m_RenderingServer = std::make_unique<GLRenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::DX11:
#if defined INNO_RENDERER_DIRECTX
		m_RenderingServer = std::make_unique<DX11RenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::DX12:
#if defined INNO_RENDERER_DIRECTX
		m_RenderingServer = std::make_unique<DX12RenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		m_RenderingServer = std::make_unique<VKRenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
		m_RenderingServer = std::make_unique<VKRenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::MT:
#if defined INNO_RENDERER_METAL
		m_RenderingServer = std::make_unique<MTRenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	default:
		break;
	}

	// Objective-C++ bridge class instances passed as the 1st and 2nd parameters of setup()
#if defined INNO_PLATFORM_MAC
	auto l_windowSystem = reinterpret_cast<MacWindowSystem*>(m_WindowSystem.get());
	auto l_windowSystemBridge = reinterpret_cast<MacWindowSystemBridge*>(appHook);

	l_windowSystem->setBridge(l_windowSystemBridge);

	auto l_renderingServer = reinterpret_cast<MTRenderingServer*>(m_RenderingServer.get());
	auto l_renderingServerBridge = reinterpret_cast<MTRenderingServerBridge*>(extraHook);

	l_renderingServer->setBridge(l_renderingServerBridge);
#endif

	return true;
}

bool InnoModuleManagerNS::setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient)
{
	m_RenderingClient = renderingClient;
	m_LogicClient = logicClient;

	m_applicationName = m_LogicClient->getApplicationName().c_str();

	if (!createSubSystemInstance(appHook, extraHook, pScmdline))
	{
		return false;
	}

	subSystemSetup(TimeSystem);
	subSystemSetup(LogSystem);
	subSystemSetup(MemorySystem);
	subSystemSetup(TaskSystem);

	subSystemSetup(TestSystem);

	if (!m_WindowSystem->setup(appHook, extraHook))
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: WindowSystem setup finished.");

	subSystemSetup(AssetSystem);
	subSystemSetup(FileSystem);

	if (!m_EntityManager->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: EntityManager setup finished.");

	ComponentManagerSetup(TransformComponent);
	ComponentManagerSetup(VisibleComponent);
	ComponentManagerSetup(LightComponent);
	ComponentManagerSetup(CameraComponent);

	if (!m_SceneHierarchyManager->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: SceneHierarchyManager setup finished.");

	subSystemSetup(PhysicsSystem);
	subSystemSetup(EventSystem);

	if (!m_RenderingFrontend->setup(m_RenderingServer.get()))
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: RenderingFrontend setup finished.");

	if (!m_GUISystem->setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: GUISystem setup finished.");

	if (!m_RenderingServer->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "ModuleManager: RenderingServer setup finished.");

	if (!m_RenderingClient->Setup())
	{
		return false;
	}

	if (!m_LogicClient->setup())
	{
		return false;
	}

	f_LogicClientUpdateJob = [&]() {m_LogicClient->update(); };
	f_PhysicsSystemUpdateBVHJob = [&]() {m_PhysicsSystem->updateBVH(); };
	f_PhysicsSystemCullingJob = [&]() {m_PhysicsSystem->updateCulling(); };
	f_RenderingFrontendUpdateJob = [&]() {m_RenderingFrontend->update(); };
	f_RenderingServerUpdateJob = [&]() {
		auto l_tickStartTime = m_TimeSystem->getCurrentTimeFromEpoch();

		m_RenderingFrontend->transferDataToGPU();

		m_RenderingClient->Render();

		m_GUISystem->render();

		m_RenderingServer->Present();

		g_pModuleManager->getWindowSystem()->getWindowSurface()->swapBuffer();

		auto l_tickEndTime = m_TimeSystem->getCurrentTimeFromEpoch();

		m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;
	};

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "ModuleManager: Engine setup finished.");

	return true;
}

bool InnoModuleManagerNS::initialize()
{
	subSystemInit(TimeSystem);
	subSystemInit(LogSystem);
	subSystemInit(MemorySystem);
	subSystemInit(TaskSystem);

	subSystemInit(TestSystem);
	subSystemInit(FileSystem);

	if (!m_EntityManager->Initialize())
	{
		return false;
	}

	ComponentManagerInit(TransformComponent);
	ComponentManagerInit(VisibleComponent);
	ComponentManagerInit(LightComponent);
	ComponentManagerInit(CameraComponent);

	if (!m_SceneHierarchyManager->Initialize())
	{
		return false;
	}

	subSystemInit(AssetSystem);
	subSystemInit(PhysicsSystem);
	subSystemInit(EventSystem);
	subSystemInit(WindowSystem);

	m_RenderingServer->Initialize();

	subSystemInit(RenderingFrontend);
	subSystemInit(GUISystem);

	if (!m_RenderingClient->Initialize())
	{
		return false;
	}

	if (!m_LogicClient->initialize())
	{
		return false;
	}

	f_LogicClientUpdateJob();
	f_PhysicsSystemCullingJob();
	f_RenderingFrontendUpdateJob();

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "ModuleManager: Engine has been initialized.");
	return true;
}

bool InnoModuleManagerNS::update()
{
	while (1)
	{
		auto l_LogicClientUpdateTask = g_pModuleManager->getTaskSystem()->submit("LogicClientUpdateTask", 0, nullptr, f_LogicClientUpdateJob);

		subSystemUpdate(TimeSystem);
		subSystemUpdate(LogSystem);
		subSystemUpdate(MemorySystem);
		subSystemUpdate(TaskSystem);

		subSystemUpdate(TestSystem);
		subSystemUpdate(FileSystem);

		if (!m_EntityManager->Simulate())
		{
			return false;
		}

		ComponentManagerSimulate(TransformComponent);
		ComponentManagerSimulate(VisibleComponent);
		ComponentManagerSimulate(LightComponent);
		ComponentManagerSimulate(CameraComponent);

		subSystemUpdate(AssetSystem);

		f_PhysicsSystemUpdateBVHJob();

		subSystemUpdate(PhysicsSystem);

		auto l_PhysicsSystemCullingTask = g_pModuleManager->getTaskSystem()->submit("PhysicsSystemCullingTask", 1, l_LogicClientUpdateTask, f_PhysicsSystemCullingJob);

		subSystemUpdate(EventSystem);

		if (!m_FileSystem->isLoadingScene())
		{
			if (m_WindowSystem->getStatus() == ObjectStatus::Activated)
			{
				m_WindowSystem->update();

				auto l_RenderingFrontendUpdateTask = g_pModuleManager->getTaskSystem()->submit("RenderingFrontendUpdateTask", 1, l_PhysicsSystemCullingTask, f_RenderingFrontendUpdateJob);

				m_GUISystem->update();

				auto l_RenderingServerTask = g_pModuleManager->getTaskSystem()->submit("RenderingServerTask", 2, l_RenderingFrontendUpdateTask, f_RenderingServerUpdateJob);
				l_RenderingServerTask->Wait();

				ComponentManagerPostFrame(TransformComponent);
				ComponentManagerPostFrame(VisibleComponent);
				ComponentManagerPostFrame(LightComponent);
				ComponentManagerPostFrame(CameraComponent);
			}
			else
			{
				m_ObjectStatus = ObjectStatus::Suspended;
				InnoLogger::Log(LogLevel::Warning, "ModuleManager: Engine is stand-by.");
				return true;
			}
		}

		m_TaskSystem->waitAllTasksToFinish();
	}
}

bool InnoModuleManagerNS::terminate()
{
	m_TaskSystem->waitAllTasksToFinish();

	if (!m_RenderingClient->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: Rendering client can't be terminated!");
		return false;
	}

	if (!m_LogicClient->terminate())
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: Logic client can't be terminated!");
		return false;
	}

	if (!m_RenderingServer->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: RenderingServer can't be terminated!");
		return false;
	}

	subSystemTerm(GUISystem);
	subSystemTerm(RenderingFrontend);

	subSystemTerm(WindowSystem);

	subSystemTerm(EventSystem);
	subSystemTerm(PhysicsSystem);
	subSystemTerm(AssetSystem);

	if (!m_SceneHierarchyManager->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: SceneHierarchyManager can't be terminated!");
		return false;
	}

	ComponentManagerTerm(TransformComponent);
	ComponentManagerTerm(VisibleComponent);
	ComponentManagerTerm(LightComponent);
	ComponentManagerTerm(CameraComponent);

	if (!m_EntityManager->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: EntityManager can't be terminated!");
		return false;
	}

	subSystemTerm(FileSystem);
	subSystemTerm(TestSystem);

	subSystemTerm(TaskSystem);
	subSystemTerm(MemorySystem);
	subSystemTerm(LogSystem);
	subSystemTerm(TimeSystem);

	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "Engine has been terminated.");

	return true;
}

bool InnoModuleManager::setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient)
{
	g_pModuleManager = this;

	return InnoModuleManagerNS::setup(appHook, extraHook, pScmdline, renderingClient, logicClient);
}

bool InnoModuleManager::initialize()
{
	return InnoModuleManagerNS::initialize();
}

bool InnoModuleManager::run()
{
	return InnoModuleManagerNS::update();
}

bool InnoModuleManager::terminate()
{
	return InnoModuleManagerNS::terminate();
}

ObjectStatus InnoModuleManager::getStatus()
{
	return m_ObjectStatus;
}

subSystemGetDefi(TimeSystem);
subSystemGetDefi(LogSystem);
subSystemGetDefi(MemorySystem);
subSystemGetDefi(TaskSystem);

subSystemGetDefi(TestSystem);
subSystemGetDefi(FileSystem);

subSystemGetDefi(EntityManager);
subSystemGetDefi(SceneHierarchyManager);
subSystemGetDefi(AssetSystem);
subSystemGetDefi(PhysicsSystem);
subSystemGetDefi(EventSystem);
subSystemGetDefi(WindowSystem);
subSystemGetDefi(RenderingFrontend);
subSystemGetDefi(GUISystem);
subSystemGetDefi(RenderingServer);

IComponentManager* InnoModuleManager::getComponentManager(uint32_t componentTypeID)
{
	IComponentManager* l_result = nullptr;

	if (componentTypeID == 1)
	{
		l_result = m_TransformComponentManager.get();
	}
	else if (componentTypeID == 2)
	{
		l_result = m_VisibleComponentManager.get();
	}
	else if (componentTypeID == 3)
	{
		l_result = m_LightComponentManager.get();
	}
	else if (componentTypeID == 4)
	{
		l_result = m_CameraComponentManager.get();
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "ModuleManager: Unknown component manager, ComponentTypeID: ", componentTypeID);
	}

	return l_result;
}

InitConfig InnoModuleManager::getInitConfig()
{
	return m_initConfig;
}

float InnoModuleManager::getTickTime()
{
	return  m_tickTime;
}

const FixedSizeString<128>& InnoModuleManager::getApplicationName()
{
	return m_applicationName;
}