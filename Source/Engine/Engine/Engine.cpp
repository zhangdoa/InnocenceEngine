#include "Engine.h"
#include "../Core/InnoLogger.h"
#include "../SubSystem/TimeSystem.h"
#include "../SubSystem/LogSystem.h"
#include "../SubSystem/MemorySystem.h"
#include "../SubSystem/TaskSystem.h"
#include "../SubSystem/TestSystem.h"
#include "../SubSystem/FileSystem.h"
#include "../EntityManager/EntityManager.h"
#include "../SubSystem/TransformSystem.h"
#include "../SubSystem/LightSystem.h"
#include "../SubSystem/CameraSystem.h"
#include "../SubSystem/SceneSystem.h"
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

using namespace Inno;
INNO_ENGINE_API IEngine* g_Engine;

#define createSystemInstanceDefi( className ) \
m_##className = std::make_unique<Inno##className>(); \
if (!m_##className.get()) \
{ \
	return false; \
} \

#define SystemSetup( className ) \
if (!m_##className->Setup()) \
{ \
	return false; \
} \
InnoLogger::Log(LogLevel::Success, "Engine: ", #className, " Setup finished."); \

#define SystemInit( className ) \
if (!m_##className->Initialize()) \
{ \
	return false; \
} \

#define SystemUpdate( className ) \
if (!m_##className->Update()) \
{ \
m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define SystemOnFrameEnd( className ) \
if (!m_##className->OnFrameEnd()) \
{ \
m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define SystemTerm( className ) \
if (!m_##className->Terminate()) \
{ \
	return false; \
} \

#define SystemGetDefi( className ) \
I##className * Engine::get##className() \
{ \
	return m_##className.get(); \
} \

namespace Inno
{
	namespace EngineNS
	{
		InitConfig parseInitConfig(const std::string& arg);
		bool createSystemInstance(void* appHook, void* extraHook, char* pScmdline);
		bool Setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool Run();

		InitConfig m_initConfig;

		std::unique_ptr<ITimeSystem> m_TimeSystem;
		std::unique_ptr<ILogSystem> m_LogSystem;
		std::unique_ptr<IMemorySystem> m_MemorySystem;
		std::unique_ptr<ITaskSystem> m_TaskSystem;
		std::unique_ptr<ITestSystem> m_TestSystem;

		std::unique_ptr<IFileSystem> m_FileSystem;

		std::unique_ptr<IEntityManager> m_EntityManager;
		std::unique_ptr<ComponentManager> m_ComponentManager;
		std::unique_ptr<InnoTransformSystem> m_TransformSystem;
		std::unique_ptr<InnoLightSystem> m_LightSystem;
		std::unique_ptr<InnoCameraSystem> m_CameraSystem;

		std::unique_ptr<ISceneSystem> m_SceneSystem;
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
}

using namespace EngineNS;

InitConfig EngineNS::parseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		InnoLogger::Log(LogLevel::Warning, "Engine: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		InnoLogger::Log(LogLevel::Warning, "Engine: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::Host;
			InnoLogger::Log(LogLevel::Success, "Engine: Launch in host mode, engine will handle OS event.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::Slave;
			InnoLogger::Log(LogLevel::Success, "Engine: Launch in slave mode, engine requires client handle OS event.");
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "Engine: Unsupported engine mode.");
		}
	}

	auto l_renderingServerArgPos = arg.find("renderer");

	if (l_renderingServerArgPos == std::string::npos)
	{
		InnoLogger::Log(LogLevel::Warning, "Engine: No rendering backend argument found, use default OpenGL rendering backend.");
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
			InnoLogger::Log(LogLevel::Warning, "Engine: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX11;
#else
			InnoLogger::Log(LogLevel::Warning, "Engine: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX12;
#else
			InnoLogger::Log(LogLevel::Warning, "Engine: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingServer = RenderingServer::VK;
#else
			InnoLogger::Log(LogLevel::Warning, "Engine: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_RENDERER_METAL
			l_result.renderingServer = RenderingServer::MT;
#else
			InnoLogger::Log(LogLevel::Warning, "Engine: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "Engine: Unsupported rendering backend, use default OpenGL rendering backend.");
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
			InnoLogger::Log(LogLevel::Warning, "Engine: Unsupported log level.");
		}
	}

	return l_result;
}

bool EngineNS::createSystemInstance(void* appHook, void* extraHook, char* pScmdline)
{
	createSystemInstanceDefi(TimeSystem);
	createSystemInstanceDefi(LogSystem);
	createSystemInstanceDefi(MemorySystem);
	createSystemInstanceDefi(TaskSystem);

	createSystemInstanceDefi(TestSystem);
	createSystemInstanceDefi(FileSystem);

	createSystemInstanceDefi(EntityManager);
	m_ComponentManager = std::make_unique<ComponentManager>();
	createSystemInstanceDefi(TransformSystem);
	createSystemInstanceDefi(LightSystem);
	createSystemInstanceDefi(CameraSystem);

	createSystemInstanceDefi(SceneSystem);
	createSystemInstanceDefi(AssetSystem);
	createSystemInstanceDefi(PhysicsSystem);
	createSystemInstanceDefi(EventSystem);

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

	// Objective-C++ bridge class instances passed as the 1st and 2nd parameters of Setup()
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

bool EngineNS::Setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient)
{
	m_RenderingClient = renderingClient;
	m_LogicClient = logicClient;

	m_applicationName = m_LogicClient->GetApplicationName();

	if (!createSystemInstance(appHook, extraHook, pScmdline))
	{
		return false;
	}

	SystemSetup(TimeSystem);
	SystemSetup(LogSystem);
	SystemSetup(MemorySystem);
	SystemSetup(TaskSystem);

	SystemSetup(TestSystem);

	IWindowSystemConfig l_windowSystemConfig;
	l_windowSystemConfig.m_AppHook = appHook;
	l_windowSystemConfig.m_ExtraHook = extraHook;

	if (!m_WindowSystem->Setup(&l_windowSystemConfig))
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: WindowSystem Setup finished.");

	SystemSetup(AssetSystem);
	SystemSetup(FileSystem);

	if (!m_EntityManager->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: EntityManager Setup finished.");

	SystemSetup(TransformSystem);
	SystemSetup(LightSystem);
	SystemSetup(CameraSystem);

	SystemSetup(SceneSystem);
	SystemSetup(PhysicsSystem);
	SystemSetup(EventSystem);

	IRenderingFrontendConfig l_renderingFrontendConfig;
	l_renderingFrontendConfig.m_RenderingServer = m_RenderingServer.get();

	if (!m_RenderingFrontend->Setup(&l_renderingFrontendConfig))
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: RenderingFrontend Setup finished.");

	if (!m_GUISystem->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: GUISystem Setup finished.");

	if (!m_RenderingServer->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: RenderingServer Setup finished.");

	if (!m_RenderingClient->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: RenderingClient Setup finished.");

	if (!m_LogicClient->Setup())
	{
		return false;
	}
	InnoLogger::Log(LogLevel::Success, "Engine: LogicClient Setup finished.");

	f_LogicClientUpdateJob = [&]() { m_LogicClient->Update(); };
	f_PhysicsSystemUpdateBVHJob = [&]() { m_PhysicsSystem->updateBVH(); };
	f_PhysicsSystemCullingJob = [&]() { m_PhysicsSystem->updateCulling(); };
	f_RenderingFrontendUpdateJob = [&]() { m_RenderingFrontend->Update(); };
	f_RenderingServerUpdateJob = [&]()
	{
		auto l_tickStartTime = m_TimeSystem->getCurrentTimeFromEpoch();

		m_RenderingFrontend->transferDataToGPU();

		m_RenderingClient->Render();

		m_GUISystem->Render();

		m_RenderingServer->Present();

		m_WindowSystem->getWindowSurface()->swapBuffer();

		auto l_tickEndTime = m_TimeSystem->getCurrentTimeFromEpoch();

		m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

		SystemOnFrameEnd(TransformSystem);
		SystemOnFrameEnd(LightSystem);
		SystemOnFrameEnd(CameraSystem);

		return true;
	};

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "Engine: Engine Setup finished.");

	return true;
}

bool EngineNS::Initialize()
{
	SystemInit(TimeSystem);
	SystemInit(LogSystem);
	SystemInit(MemorySystem);
	SystemInit(TaskSystem);

	SystemInit(TestSystem);
	SystemInit(FileSystem);

	if (!m_EntityManager->Initialize())
	{
		return false;
	}

	SystemInit(TransformSystem);
	SystemInit(LightSystem);
	SystemInit(CameraSystem);

	SystemInit(SceneSystem);
	SystemInit(AssetSystem);
	SystemInit(PhysicsSystem);
	SystemInit(EventSystem);
	SystemInit(WindowSystem);

	m_RenderingServer->Initialize();

	SystemInit(RenderingFrontend);
	SystemInit(GUISystem);

	if (!m_RenderingClient->Initialize())
	{
		return false;
	}

	if (!m_LogicClient->Initialize())
	{
		return false;
	}

	f_LogicClientUpdateJob();
	f_PhysicsSystemCullingJob();
	f_RenderingFrontendUpdateJob();

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "Engine: Engine has been initialized.");
	return true;
}

bool EngineNS::Update()
{
	auto l_LogicClientUpdateTask = g_Engine->getTaskSystem()->Submit("LogicClientUpdateTask", 0, nullptr, f_LogicClientUpdateJob);

	SystemUpdate(TimeSystem);
	SystemUpdate(LogSystem);
	SystemUpdate(MemorySystem);
	SystemUpdate(TaskSystem);

	SystemUpdate(TestSystem);
	SystemUpdate(FileSystem);
	SystemUpdate(SceneSystem);

	if (!m_EntityManager->Update())
	{
		return false;
	}

	SystemUpdate(TransformSystem);
	SystemUpdate(CameraSystem);
	SystemUpdate(LightSystem);

	SystemUpdate(AssetSystem);

	f_PhysicsSystemUpdateBVHJob();

	SystemUpdate(PhysicsSystem);

	auto l_PhysicsSystemCullingTask = g_Engine->getTaskSystem()->Submit("PhysicsSystemCullingTask", 1, l_LogicClientUpdateTask.m_Task, f_PhysicsSystemCullingJob);

	SystemUpdate(EventSystem);

	if (!m_SceneSystem->isLoadingScene())
	{
		if (m_WindowSystem->GetStatus() == ObjectStatus::Activated)
		{
			m_WindowSystem->Update();

			auto l_RenderingFrontendUpdateTask = g_Engine->getTaskSystem()->Submit("RenderingFrontendUpdateTask", 1, l_PhysicsSystemCullingTask.m_Task, f_RenderingFrontendUpdateJob);

			m_GUISystem->Update();

			auto l_RenderingServerUpdateTask = g_Engine->getTaskSystem()->Submit("RenderingServerUpdateTask", 2, l_RenderingFrontendUpdateTask.m_Task, f_RenderingServerUpdateJob);
		}
		else
		{
			m_ObjectStatus = ObjectStatus::Suspended;
			InnoLogger::Log(LogLevel::Warning, "Engine: Engine is stand-by.");
			return false;
		}
	}

	m_TaskSystem->WaitSync();

	return true;
}

bool EngineNS::Terminate()
{
	m_TaskSystem->WaitSync();

	if (!m_RenderingClient->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Engine: Rendering client can't be terminated!");
		return false;
	}

	if (!m_LogicClient->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Engine: Logic client can't be terminated!");
		return false;
	}

	if (!m_RenderingServer->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Engine: RenderingServer can't be terminated!");
		return false;
	}

	SystemTerm(GUISystem);
	SystemTerm(RenderingFrontend);

	SystemTerm(WindowSystem);

	SystemTerm(EventSystem);
	SystemTerm(PhysicsSystem);
	SystemTerm(AssetSystem);
	SystemTerm(SceneSystem);

	SystemTerm(TransformSystem);
	SystemTerm(LightSystem);
	SystemTerm(CameraSystem);

	if (!m_EntityManager->Terminate())
	{
		InnoLogger::Log(LogLevel::Error, "Engine: EntityManager can't be terminated!");
		return false;
	}

	SystemTerm(FileSystem);
	SystemTerm(TestSystem);

	SystemTerm(TaskSystem);
	SystemTerm(MemorySystem);
	SystemTerm(LogSystem);
	SystemTerm(TimeSystem);

	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "Engine has been terminated.");

	return true;
}

bool EngineNS::Run()
{
	while (1)
	{
		if (!EngineNS::Update())
		{
			return false;
		};
	}
	return true;
}

bool Engine::Setup(void* appHook, void* extraHook, char* pScmdline, IRenderingClient* renderingClient, ILogicClient* logicClient)
{
	g_Engine = this;

	return EngineNS::Setup(appHook, extraHook, pScmdline, renderingClient, logicClient);
}

bool Engine::Initialize()
{
	return EngineNS::Initialize();
}

bool Engine::Update()
{
	return EngineNS::Update();
}

bool Engine::Terminate()
{
	return EngineNS::Terminate();
}

bool Engine::Run()
{
	return EngineNS::Run();
}

ObjectStatus Engine::GetStatus()
{
	return m_ObjectStatus;
}

SystemGetDefi(TimeSystem);
SystemGetDefi(LogSystem);
SystemGetDefi(MemorySystem);
SystemGetDefi(TaskSystem);

SystemGetDefi(TestSystem);
SystemGetDefi(FileSystem);

SystemGetDefi(EntityManager);
SystemGetDefi(SceneSystem);
SystemGetDefi(AssetSystem);
SystemGetDefi(PhysicsSystem);
SystemGetDefi(EventSystem);
SystemGetDefi(WindowSystem);
SystemGetDefi(RenderingFrontend);
SystemGetDefi(GUISystem);
SystemGetDefi(RenderingServer);

ComponentManager* Engine::getComponentManager()
{
	return m_ComponentManager.get();
}

InitConfig Engine::getInitConfig()
{
	return m_initConfig;
}

float Engine::getTickTime()
{
	return  m_tickTime;
}

const FixedSizeString<128>& Engine::getApplicationName()
{
	return m_applicationName;
}
