#include "ModuleManager.h"
#include "../Core/TimeSystem.h"
#include "../Core/LogSystem.h"
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
#if defined INNO_PLATFORM_WIN
#include "../Platform/WinWindow/WinWindowSystem.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11RenderingBackend.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12RenderingBackend.h"
#endif
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
#include "../RenderingBackend/GLRenderingBackend/GLRenderingBackend.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "../Platform/MacWindow/MacWindowSystem.h"
#include "../RenderingBackend/MTRenderingBackend/MTRenderingBackend.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "../Platform/LinuxWindow/LinuxWindowSystem.h"
#endif
#if defined INNO_RENDERER_VULKAN
#include "../RenderingBackend/VKRenderingBackend/VKRenderingBackend.h"
#endif

#include "../ImGuiWrapper/ImGuiWrapper.h"

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
g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, std::string(#className) + " setup finished."); \

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
g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, std::string(#className) + " setup finished."); \

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

INNO_PRIVATE_SCOPE InnoModuleManagerNS
{
	InitConfig parseInitConfig(const std::string& arg);
	bool createSubSystemInstance(void* appHook, void* extraHook, char* pScmdline);
	bool setup(void* appHook, void* extraHook, char* pScmdline, IGameInstance* gameInstance);
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
	std::unique_ptr<IRenderingBackend> m_RenderingBackend;
	IGameInstance* m_GameInstance;
	FixedSizeString<128> m_applicationName;

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;

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
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::GAME;
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "ModuleManager: Launch in game mode.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::EDITOR;
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "ModuleManager: Launch in editor mode.");
		}
		else
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: Unsupported engine mode.");
		}
	}

	auto l_renderingBackendArgPos = arg.find("renderer");

	if (l_renderingBackendArgPos == std::string::npos)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: No rendering backend argument found, use default OpenGL rendering backend.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_renderingBackendArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
			l_result.renderingBackend = RenderingBackend::GL;
#else
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX11;
#else
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX12;
#else
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingBackend = RenderingBackend::VK;
#else
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_PLATFORM_MAC
			l_result.renderingBackend = RenderingBackend::MT;
#else
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "ModuleManager: Unsupported rendering backend, use default OpenGL rendering backend.");
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

	switch (m_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
		m_RenderingBackend = std::make_unique<GLRenderingBackend>();
		if (!m_RenderingBackend.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::DX11:
#if defined INNO_PLATFORM_WIN
		m_RenderingBackend = std::make_unique<DX11RenderingBackend>();
		if (!m_RenderingBackend.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::DX12:
#if defined INNO_PLATFORM_WIN
		m_RenderingBackend = std::make_unique<DX12RenderingBackend>();
		if (!m_RenderingBackend.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::VK:
#if defined INNO_RENDERER_VULKAN
		m_RenderingBackend = std::make_unique<VKRenderingBackend>();
		if (!m_RenderingBackend.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::MT:
#if defined INNO_PLATFORM_MAC
		m_RenderingBackend = std::make_unique<MTRenderingBackend>();
		if (!m_RenderingBackend.get())
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

	auto l_renderingBackendSystem = reinterpret_cast<MTRenderingBackend*>(m_RenderingBackend.get());
	auto l_renderingBackendSystemBridge = reinterpret_cast<MTRenderingBackendBridge*>(extraHook);

	l_renderingBackendSystem->setBridge(l_renderingBackendSystemBridge);
#endif

	return true;
}

bool InnoModuleManagerNS::setup(void* appHook, void* extraHook, char* pScmdline, IGameInstance* gameInstance)
{
	m_GameInstance = gameInstance;
	m_applicationName = m_GameInstance->getGameName().c_str();

	if (!createSubSystemInstance(appHook, extraHook, pScmdline))
	{
		return false;
	}

	subSystemSetup(TimeSystem);
	subSystemSetup(LogSystem);
	subSystemSetup(MemorySystem);
	subSystemSetup(TaskSystem);

	subSystemSetup(TestSystem);

	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_I, ButtonStatus::PRESSED }, &f_toggleshowImGui);

	if (!m_WindowSystem->setup(appHook, extraHook))
	{
		return false;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WindowSystem setup finished.");

	if (!m_RenderingBackend->setup())
	{
		return false;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingBackend setup finished.");

	if (!m_RenderingFrontend->setup(m_RenderingBackend.get()))
	{
		return false;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontend setup finished.");

	if (!ImGuiWrapper::get().setup())
	{
		return false;
	}

	subSystemSetup(AssetSystem);
	subSystemSetup(FileSystem);

	if (!m_EntityManager->Setup())
	{
		return false;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "EntityManager setup finished.");

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
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "SceneHierarchyManager setup finished.");

	subSystemSetup(PhysicsSystem);
	subSystemSetup(EventSystem);

	if (!m_GameInstance->setup())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine setup finished.");
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

	m_WindowSystem->initialize();
	m_RenderingBackend->initialize();
	m_RenderingFrontend->initialize();
	ImGuiWrapper::get().initialize();

	if (!m_GameInstance->initialize())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been initialized.");
	return true;
}

bool InnoModuleManagerNS::update()
{
	while (1)
	{
		if (!m_GameInstance->update())
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

					m_RenderingBackend->update();

					if (m_showImGui)
					{
						ImGuiWrapper::get().update();
					}

					m_RenderingBackend->render();

					if (m_showImGui)
					{
						ImGuiWrapper::get().render();
					}

					m_RenderingBackend->present();

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
			m_LogSystem->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
			return true;
		}
	}
}

bool InnoModuleManagerNS::terminate()
{
	if (!m_GameInstance->terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "Game can't be terminated!");
		return false;
	}

	if (!ImGuiWrapper::get().terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GuiSystem can't be terminated!");
		return false;
	}

	if (!m_RenderingBackend->terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackend can't be terminated!");
		return false;
	}
	if (!m_RenderingFrontend->terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontend can't be terminated!");
		return false;
	}

	if (!m_WindowSystem->terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "WindowSystem can't be terminated!");
		return false;
	}

	subSystemTerm(EventSystem);
	subSystemTerm(PhysicsSystem);
	subSystemTerm(AssetSystem);

	if (!m_SceneHierarchyManager->Terminate())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "SceneHierarchyManager can't be terminated!");
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
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "EntityManager can't be terminated!");
		return false;
	}

	subSystemTerm(FileSystem);
	subSystemTerm(TestSystem);

	subSystemTerm(TaskSystem);
	subSystemTerm(MemorySystem);
	subSystemTerm(LogSystem);
	subSystemTerm(TimeSystem);

	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been terminated.");

	return true;
}

bool InnoModuleManager::setup(void* appHook, void* extraHook, char* pScmdline, IGameInstance* gameInstance)
{
	g_pModuleManager = this;

	return InnoModuleManagerNS::setup(appHook, extraHook, pScmdline, gameInstance);
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

IWindowSystem * InnoModuleManager::getWindowSystem()
{
	return m_WindowSystem.get();
}

IRenderingFrontend * InnoModuleManager::getRenderingFrontend()
{
	return m_RenderingFrontend.get();
}

IRenderingBackend * InnoModuleManager::getRenderingBackend()
{
	return m_RenderingBackend.get();
}

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