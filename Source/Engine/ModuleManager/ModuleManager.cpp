#include "ModuleManager.h"
#include "../Core/TimeSystem.h"
#include "../Core/LogSystem.h"
#include "../Core/InnoLogger.h"
#include "../Core/MemorySystem.h"
#include "../Core/TaskSystem.h"
#include "../Core/TestSystem.h"
#include "../FileSystem/FileSystem.h"
#include "../EntityManager/EntityManager.h"
#include "../ComponentManager/TransformComponentManager.h"
#include "../ComponentManager/VisibleComponentManager.h"
#include "../ComponentManager/DirectionalLightComponentManager.h"
#include "../ComponentManager/PointLightComponentManager.h"
#include "../ComponentManager/SpotLightComponentManager.h"
#include "../ComponentManager/SphereLightComponentManager.h"
#include "../ComponentManager/CameraComponentManager.h"
#include "../SceneHierarchyManager/SceneHierarchyManager.h"
#include "../AssetSystem/AssetSystem.h"
#include "../PhysicsSystem/PhysicsSystem.h"
#include "../Core/EventSystem.h"
#include "../RenderingFrontend/RenderingFrontend.h"
#include "../RenderingFrontend/GUISystem.h"
#if defined INNO_PLATFORM_WIN
#include "../Platform/WinWindow/WinWindowSystem.h"
#include "../RenderingServer/DX11/DX11RenderingServer.h"
#include "../RenderingServer/DX12/DX12RenderingServer.h"
#endif
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
#include "../RenderingServer/GL/GLRenderingServer.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "../Platform/MacWindow/MacWindowSystem.h"
#include "../RenderingServer/MT/MTRenderingServer.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "../Platform/LinuxWindow/LinuxWindowSystem.h"
#endif
#if defined INNO_RENDERER_VULKAN
#include "../RenderingServer/VK/VKRenderingServer.h"
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
InnoLogger::Log(LogLevel::Success, #className, " setup finished."); \

#define subSystemInit( className ) \
if (!g_pModuleManager->get##className()->initialize()) \
{ \
	return false; \
} \

#define subSystemUpdate( className ) \
if (!g_pModuleManager->get##className()->update()) \
{ \
m_objectStatus = ObjectStatus::Suspended; \
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
if (!g_pModuleManager->getComponentManager(ComponentType::className)->Setup()) \
{ \
	return false; \
} \
InnoLogger::Log(LogLevel::Success, #className, " setup finished."); \

#define ComponentManagerInit( className ) \
if (!g_pModuleManager->getComponentManager(ComponentType::className)->Initialize()) \
{ \
	return false; \
} \

#define ComponentManagerUpdate( className ) \
if (!g_pModuleManager->getComponentManager(ComponentType::className)->Simulate()) \
{ \
m_objectStatus = ObjectStatus::Suspended; \
return false; \
}

#define ComponentManagerTerm( className ) \
if (!g_pModuleManager->getComponentManager(ComponentType::className)->Terminate()) \
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
	std::unique_ptr<IDirectionalLightComponentManager> m_DirectionalLightComponentManager;
	std::unique_ptr<IPointLightComponentManager> m_PointLightComponentManager;
	std::unique_ptr<ISpotLightComponentManager> m_SpotLightComponentManager;
	std::unique_ptr<ISphereLightComponentManager> m_SphereLightComponentManager;
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

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_allowRender = false;

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
			l_result.engineMode = EngineMode::GAME;
			InnoLogger::Log(LogLevel::Verbose, "ModuleManager: Launch in game mode.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::EDITOR;
			InnoLogger::Log(LogLevel::Verbose, "ModuleManager: Launch in editor mode.");
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
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
			l_result.renderingServer = RenderingServer::GL;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingServer = RenderingServer::DX11;
#else
			InnoLogger::Log(LogLevel::Warning, "ModuleManager: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_PLATFORM_WIN
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
#if defined INNO_PLATFORM_MAC
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
	createSubSystemInstanceDefi(DirectionalLightComponentManager);
	createSubSystemInstanceDefi(PointLightComponentManager);
	createSubSystemInstanceDefi(SpotLightComponentManager);
	createSubSystemInstanceDefi(SphereLightComponentManager);
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
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
		m_RenderingServer = std::make_unique<GLRenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::DX11:
#if defined INNO_PLATFORM_WIN
		m_RenderingServer = std::make_unique<DX11RenderingServer>();
		if (!m_RenderingServer.get())
		{
			return false;
		}
#endif
		break;
	case RenderingServer::DX12:
#if defined INNO_PLATFORM_WIN
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
#if defined INNO_PLATFORM_MAC
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

	auto l_renderingServerSystem = reinterpret_cast<MTRenderingServer*>(m_RenderingServer.get());
	auto l_renderingServerSystemBridge = reinterpret_cast<MTRenderingServerBridge*>(extraHook);

	l_renderingServerSystem->setBridge(l_renderingServerSystemBridge);
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
	InnoLogger::Log(LogLevel::Success, "WindowSystem setup finished.");

	subSystemSetup(AssetSystem);
	subSystemSetup(FileSystem);

	if (!m_EntityManager->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "EntityManager setup finished.");

	ComponentManagerSetup(TransformComponent);
	ComponentManagerSetup(VisibleComponent);
	ComponentManagerSetup(DirectionalLightComponent);
	ComponentManagerSetup(PointLightComponent);
	ComponentManagerSetup(SpotLightComponent);
	ComponentManagerSetup(SphereLightComponent);
	ComponentManagerSetup(CameraComponent);

	if (!m_SceneHierarchyManager->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "SceneHierarchyManager setup finished.");

	subSystemSetup(PhysicsSystem);
	subSystemSetup(EventSystem);

	if (!m_RenderingFrontend->setup(m_RenderingServer.get()))
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "RenderingFrontend setup finished.");

	if (!m_GUISystem->setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "GUISystem setup finished.");

	if (!m_RenderingServer->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "RenderingServer setup finished.");

	if (!m_RenderingClient->Setup())
	{
		return false;
	}

	if (!m_LogicClient->setup())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "Engine setup finished.");
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
	ComponentManagerInit(DirectionalLightComponent);
	ComponentManagerInit(PointLightComponent);
	ComponentManagerInit(SpotLightComponent);
	ComponentManagerInit(SphereLightComponent);
	ComponentManagerInit(CameraComponent);

	if (!m_SceneHierarchyManager->Initialize())
	{
		return false;
	}

	subSystemInit(AssetSystem);
	subSystemInit(PhysicsSystem);
	subSystemInit(EventSystem);
	subSystemInit(WindowSystem);

	subSystemInit(RenderingFrontend);
	subSystemInit(GUISystem);

	m_RenderingServer->Initialize();

	if (!m_RenderingClient->Initialize())
	{
		return false;
	}

	if (!m_LogicClient->initialize())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "Engine has been initialized.");
	return true;
}

bool InnoModuleManagerNS::update()
{
	while (1)
	{
		if (!m_LogicClient->update())
		{
			return false;
		}

		auto l_tickStartTime = m_TimeSystem->getCurrentTimeFromEpoch();
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
		ComponentManagerUpdate(TransformComponent);
		ComponentManagerUpdate(VisibleComponent);
		ComponentManagerUpdate(DirectionalLightComponent);
		ComponentManagerUpdate(PointLightComponent);
		ComponentManagerUpdate(SpotLightComponent);
		ComponentManagerUpdate(SphereLightComponent);
		ComponentManagerUpdate(CameraComponent);

		subSystemUpdate(AssetSystem);
		subSystemUpdate(PhysicsSystem);
		subSystemUpdate(EventSystem);

		if (m_WindowSystem->getStatus() == ObjectStatus::Activated)
		{
			if (!m_FileSystem->isLoadingScene())
			{
				m_WindowSystem->update();

				if (!m_allowRender)
				{
					m_RenderingFrontend->update();

					m_allowRender = true;
				}

				if (!m_isRendering && m_allowRender)
				{
					m_allowRender = false;

					m_isRendering = true;

					m_RenderingClient->Render();

					m_GUISystem->update();

					m_RenderingServer->Present();

					m_isRendering = false;
				}

				m_TransformComponentManager->SaveCurrentFrameTransform();

				auto l_tickEndTime = m_TimeSystem->getCurrentTimeFromEpoch();

				m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;
			}
		}
		else
		{
			m_objectStatus = ObjectStatus::Suspended;
			InnoLogger::Log(LogLevel::Warning, "Engine is stand-by.");
			return true;
		}
	}
}

bool InnoModuleManagerNS::terminate()
{
	if (!m_RenderingClient->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Rendering client can't be terminated!");
		return false;
	}

	if (!m_LogicClient->terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Logic client can't be terminated!");
		return false;
	}

	if (!m_RenderingServer->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "RenderingServer can't be terminated!");
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
		InnoLogger::Log(LogLevel::Error, "SceneHierarchyManager can't be terminated!");
		return false;
	}

	ComponentManagerTerm(TransformComponent);
	ComponentManagerTerm(VisibleComponent);
	ComponentManagerTerm(DirectionalLightComponent);
	ComponentManagerTerm(PointLightComponent);
	ComponentManagerTerm(SpotLightComponent);
	ComponentManagerTerm(SphereLightComponent);
	ComponentManagerTerm(CameraComponent);

	if (!m_EntityManager->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "EntityManager can't be terminated!");
		return false;
	}

	subSystemTerm(FileSystem);
	subSystemTerm(TestSystem);

	subSystemTerm(TaskSystem);
	subSystemTerm(MemorySystem);
	subSystemTerm(LogSystem);
	subSystemTerm(TimeSystem);

	m_objectStatus = ObjectStatus::Terminated;
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
	return m_objectStatus;
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

IComponentManager * InnoModuleManager::getComponentManager(ComponentType componentType)
{
	IComponentManager* l_result = nullptr;
	switch (componentType)
	{
	case ComponentType::TransformComponent:
		l_result = m_TransformComponentManager.get();
		break;
	case ComponentType::VisibleComponent:
		l_result = m_VisibleComponentManager.get();
		break;
	case ComponentType::DirectionalLightComponent:
		l_result = m_DirectionalLightComponentManager.get();
		break;
	case ComponentType::PointLightComponent:
		l_result = m_PointLightComponentManager.get();
		break;
	case ComponentType::SpotLightComponent:
		l_result = m_SpotLightComponentManager.get();
		break;
	case ComponentType::SphereLightComponent:
		l_result = m_SphereLightComponentManager.get();
		break;
	case ComponentType::CameraComponent:
		l_result = m_CameraComponentManager.get();
		break;
	case ComponentType::PhysicsDataComponent:
		break;
	case ComponentType::MeshDataComponent:
		break;
	case ComponentType::MaterialDataComponent:
		break;
	case ComponentType::TextureDataComponent:
		break;
	default:
		break;
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