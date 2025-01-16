#include "Engine.h"
#include "Common/Timer.h"
#include "Common/Logger.h"
#include "Common/Memory.h"
#include "Common/TaskScheduler.h"
#include "Common/IOService.h"
#include "Common/Task.h"
#include "Services/EntityManager.h"
#include "Services/ComponentManager.h"
#include "Services/TransformSystem.h"
#include "Services/LightSystem.h"
#include "Services/CameraSystem.h"
#include "Services/SceneSystem.h"
#include "Services/AssetSystem.h"
#include "Services/PhysicsSystem.h"
#include "Services/EventSystem.h"
#include "Services/RenderingFrontend.h"
#include "Services/GUISystem.h"

#if defined INNO_PLATFORM_WIN
#include "Platform/WinWindow/WinWindowSystem.h"
#endif

#if defined INNO_PLATFORM_MAC
#include "Platform/MacWindow/MacWindowSystem.h"
#endif

#if defined INNO_PLATFORM_LINUX
#include "Platform/LinuxWindow/LinuxWindowSystem.h"
#endif

#if defined INNO_RENDERER_DIRECTX
#include "RenderingServer/DX11/DX11RenderingServer.h"
#include "RenderingServer/DX12/DX12RenderingServer.h"
#endif

#if defined INNO_RENDERER_OPENGL
#include "RenderingServer/GL/GLRenderingServer.h"
#endif

#if defined INNO_RENDERER_VULKAN
#include "RenderingServer/VK/VKRenderingServer.h"
#endif

#if defined INNO_RENDERER_METAL
#include "RenderingServer/MT/MTRenderingServer.h"
#endif

namespace Inno 
{
    Engine* g_Engine = nullptr;
}

using namespace Inno;

#define SystemSetup( className ) \
if (!Get<##className>()->Setup(nullptr)) \
{ \
	return false; \
} \

#define SystemInit( className ) \
if (!Get<##className>()->Initialize()) \
{ \
	return false; \
} \

#define SystemUpdate( className ) \
if (!Get<##className>()->Update()) \
{ \
m_pImpl->m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define SystemOnFrameEnd( className ) \
if (!Get<##className>()->OnFrameEnd()) \
{ \
m_pImpl->m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define SystemTerm( className ) \
if (!Get<##className>()->Terminate()) \
{ \
	return false; \
} \

namespace Inno
{
	class EngineImpl
	{
	public:
		InitConfig m_initConfig;

		std::unique_ptr<IWindowSystem> m_WindowSystem;
		std::unique_ptr<IRenderingServer> m_RenderingServer;

		std::unique_ptr<IRenderingClient> m_RenderingClient;
		std::unique_ptr<ILogicClient> m_LogicClient;

		FixedSizeString<128> m_applicationName;

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::atomic<bool> m_isRendering = false;
		std::atomic<bool> m_allowRender = false;

		std::function<void()> f_LogicClientUpdateFunction;
		std::function<void()> f_PhysicsSystemUpdateBVHFunction;
		std::function<void()> f_PhysicsSystemCullingFunction;
		std::function<void()> f_RenderingFrontendUpdateFunction;
		std::function<void()> f_RenderingServerUpdateFunction;

		std::shared_ptr<ITask> m_LogicClientUpdateTask;
		std::shared_ptr<ITask> m_TransformComponentsSimulationTask;
		std::shared_ptr<ITask> m_PhysicsSystemUpdateBVHTask;
		std::shared_ptr<ITask> m_PhysicsSystemCullingTask;
		std::shared_ptr<ITask> m_RenderingFrontendUpdateTask;
		std::shared_ptr<ITask> m_RenderingServerUpdateTask;

		float m_tickTime = 0;
	};
}

Engine::Engine()
{
	g_Engine = this;
	m_pImpl = new EngineImpl();
}

Engine::~Engine()
{
	delete m_pImpl;

    // Clean up all singletons
	for (auto& pair : singletons_) 
	{
		delete static_cast<char*>(pair.second);
	}

	g_Engine = nullptr;
}

InitConfig Engine::ParseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		Get<Logger>()->Log(LogLevel::Warning, "Engine: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		Get<Logger>()->Log(LogLevel::Warning, "Engine: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::Host;
			Get<Logger>()->Log(LogLevel::Success, "Engine: Launch in host mode, engine will handle OS event.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::Slave;
			Get<Logger>()->Log(LogLevel::Success, "Engine: Launch in slave mode, engine requires client handle OS event.");
		}
		else
		{
			Get<Logger>()->Log(LogLevel::Warning, "Engine: Unsupported engine mode.");
		}
	}

	auto l_renderingServerArgPos = arg.find("renderer");

	if (l_renderingServerArgPos == std::string::npos)
	{
		Get<Logger>()->Log(LogLevel::Warning, "Engine: No rendering backend argument found, use default OpenGL rendering backend.");
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
			Get<Logger>()->Log(LogLevel::Warning, "Engine: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX11;
#else
			Get<Logger>()->Log(LogLevel::Warning, "Engine: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX12;
#else
			Get<Logger>()->Log(LogLevel::Warning, "Engine: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingServer = RenderingServer::VK;
#else
			Get<Logger>()->Log(LogLevel::Warning, "Engine: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_RENDERER_METAL
			l_result.renderingServer = RenderingServer::MT;
#else
			Get<Logger>()->Log(LogLevel::Warning, "Engine: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			Get<Logger>()->Log(LogLevel::Warning, "Engine: Unsupported rendering backend, use default OpenGL rendering backend.");
		}
	}

	auto l_logLevelArgPos = arg.find("loglevel");
	if (l_engineModeArgPos == std::string::npos)
	{
		Get<Logger>()->SetDefaultLogLevel(LogLevel::Success);
	}
	else
	{
		std::string l_logLevelArguments = arg.substr(l_logLevelArgPos + 9);
		l_logLevelArguments = l_logLevelArguments.substr(0, 1);

		if (l_logLevelArguments == "0")
		{
			Get<Logger>()->SetDefaultLogLevel(LogLevel::Verbose);
		}
		else if (l_logLevelArguments == "1")
		{
			Get<Logger>()->SetDefaultLogLevel(LogLevel::Success);
		}
		else if (l_logLevelArguments == "2")
		{
			Get<Logger>()->SetDefaultLogLevel(LogLevel::Warning);
		}
		else if (l_logLevelArguments == "3")
		{
			Get<Logger>()->SetDefaultLogLevel(LogLevel::Error);
		}
		else
		{
			Get<Logger>()->Log(LogLevel::Warning, "Engine: Unsupported log level.");
		}
	}

	return l_result;
}

bool Engine::CreateServices(void* appHook, void* extraHook, char* pScmdline)
{
	Get<Timer>();
	Get<Logger>();
	Get<Memory>();
	Get<TaskScheduler>();
	Get<IOService>();

	Get<EventSystem>();

	std::string l_windowArguments = pScmdline;
	m_pImpl->m_initConfig = ParseInitConfig(l_windowArguments);

#if defined INNO_PLATFORM_WIN
	m_pImpl->m_WindowSystem = std::make_unique<WinWindowSystem>();
#endif
#if defined INNO_PLATFORM_MAC
	m_pImpl->m_WindowSystem = std::make_unique<MacWindowSystem>();
#endif
#if defined INNO_PLATFORM_LINUX
	m_pImpl->m_WindowSystem = std::make_unique<LinuxWindowSystem>();
#endif
	if (!m_pImpl->m_WindowSystem.get())
	{
		return false;
	}

	Get<RenderingFrontend>();
	Get<GUISystem>();

	switch (m_pImpl->m_initConfig.renderingServer)
	{
	case RenderingServer::GL:
#if defined INNO_RENDERER_OPENGL
		m_pImpl->m_RenderingServer = std::make_unique<GLRenderingServer>();
#endif
		break;
	case RenderingServer::DX11:
#if defined INNO_RENDERER_DIRECTX
		m_pImpl->m_RenderingServer = std::make_unique<DX11RenderingServer>();
#endif
		break;
	case RenderingServer::DX12:
#if defined INNO_RENDERER_DIRECTX
		m_pImpl->m_RenderingServer = std::make_unique<DX12RenderingServer>();
#endif
		break;
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		m_pImpl->m_RenderingServer = std::make_unique<VKRenderingServer>();
#endif
		break;
	case RenderingServer::MT:
#if defined INNO_RENDERER_METAL
		m_pImpl->m_RenderingServer = std::make_unique<MTRenderingServer>();
#endif
		break;
	default:
		break;
	}
	
	if (!m_pImpl->m_RenderingServer.get())
	{
		return false;
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

	Get<EntityManager>();
	Get<ComponentManager>();

	Get<AssetSystem>();
	Get<SceneSystem>();
	Get<PhysicsSystem>();

	Get<TransformSystem>();
	Get<LightSystem>();
	Get<CameraSystem>();

	return true;
}

bool Engine::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pImpl->m_RenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();
	if (!m_pImpl->m_RenderingClient.get())
	{
		return false;
	}

	m_pImpl->m_LogicClient = std::make_unique<INNO_LOGIC_CLIENT>();
	if (!m_pImpl->m_LogicClient.get())
	{
		return false;
	}

	m_pImpl->m_applicationName = m_pImpl->m_LogicClient->GetApplicationName();

	if (!CreateServices(appHook, extraHook, pScmdline))
	{
		return false;
	}

	SystemSetup(EventSystem);
	
	IWindowSystemConfig l_windowSystemConfig;
	l_windowSystemConfig.m_AppHook = appHook;
	l_windowSystemConfig.m_ExtraHook = extraHook;

	if (!m_pImpl->m_WindowSystem->Setup(&l_windowSystemConfig))
	{
		return false;
	}
	Get<Logger>()->Log(LogLevel::Success, "Engine: WindowSystem Setup finished.");

	SystemSetup(EntityManager);

	SystemSetup(SceneSystem);
	SystemSetup(PhysicsSystem);

	SystemSetup(TransformSystem);
	SystemSetup(LightSystem);
	SystemSetup(CameraSystem);

	IRenderingFrontendConfig l_renderingFrontendConfig;
	l_renderingFrontendConfig.m_RenderingServer = m_pImpl->m_RenderingServer.get();

	if (!Get<RenderingFrontend>()->Setup(&l_renderingFrontendConfig))
	{
		return false;
	}
	Get<Logger>()->Log(LogLevel::Success, "Engine: RenderingFrontend Setup finished.");

	SystemSetup(GUISystem);

	if (!m_pImpl->m_RenderingServer->Setup())
	{
		return false;
	}
	Get<Logger>()->Log(LogLevel::Success, "Engine: RenderingServer Setup finished.");

	if (!m_pImpl->m_RenderingClient->Setup())
	{
		return false;
	}
	Get<Logger>()->Log(LogLevel::Success, "Engine: RenderingClient Setup finished.");

	if (!m_pImpl->m_LogicClient->Setup())
	{
		return false;
	}
	Get<Logger>()->Log(LogLevel::Success, "Engine: LogicClient Setup finished.");

	m_pImpl->m_LogicClientUpdateTask = g_Engine->Get<TaskScheduler>()->Submit("Logic Client Update Task", -1, [&]() { m_pImpl->m_LogicClient->Update(); });
	m_pImpl->m_TransformComponentsSimulationTask = g_Engine->Get<TaskScheduler>()->Submit("Transform Components Simulation Task", -1, [&]() { Get<TransformSystem>()->Update(); });
	m_pImpl->m_PhysicsSystemUpdateBVHTask = g_Engine->Get<TaskScheduler>()->Submit("Physics System Update BVH Task", -1, [&]() { Get<PhysicsSystem>()->updateBVH(); });
	m_pImpl->m_PhysicsSystemCullingTask = g_Engine->Get<TaskScheduler>()->Submit("Physics System Culling Task", -1, [&]() { Get<PhysicsSystem>()->updateCulling(); });
	m_pImpl->m_RenderingFrontendUpdateTask = g_Engine->Get<TaskScheduler>()->Submit("Rendering Frontend Update Task", -1, [&]() { Get<RenderingFrontend>()->Update(); });
	m_pImpl->m_RenderingServerUpdateTask = g_Engine->Get<TaskScheduler>()->Submit("Rendering Server Update Task", 2, [&]()
		{
			auto l_tickStartTime = Get<Timer>()->GetCurrentTimeFromEpoch();

			Get<RenderingFrontend>()->TransferDataToGPU();

			m_pImpl->m_RenderingClient->Render();

			Get<GUISystem>()->Render();

			m_pImpl->m_RenderingServer->Present();

			m_pImpl->m_WindowSystem->GetWindowSurface()->swapBuffer();

			auto l_tickEndTime = Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

			m_pImpl->m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

			SystemOnFrameEnd(TransformSystem);
			SystemOnFrameEnd(LightSystem);
			SystemOnFrameEnd(CameraSystem);

			return true;
		});

	m_pImpl->m_ObjectStatus = ObjectStatus::Created;
	Get<Logger>()->Log(LogLevel::Success, "Engine: Engine Setup finished.");

	return true;
}

bool Engine::Initialize()
{
	SystemInit(EventSystem);
	m_pImpl->m_WindowSystem->Initialize();

	SystemInit(EntityManager);

	SystemInit(SceneSystem);
	SystemInit(PhysicsSystem);

	SystemInit(TransformSystem);
	SystemInit(LightSystem);
	SystemInit(CameraSystem);
	m_pImpl->m_RenderingServer->Initialize();

	SystemInit(RenderingFrontend);
	SystemInit(GUISystem);

	m_pImpl->m_RenderingClient->Initialize();
	m_pImpl->m_LogicClient->Initialize();

	m_pImpl->m_ObjectStatus = ObjectStatus::Activated;
	Get<Logger>()->Log(LogLevel::Success, "Engine: Engine has been initialized.");

	return true;
}

bool Engine::ExecuteDefaultTask()
{
	Get<Timer>()->Tick();

	m_pImpl->m_WindowSystem->Update();
	SystemUpdate(EventSystem);
	SystemUpdate(EntityManager);

	SystemUpdate(SceneSystem);
	if (Get<SceneSystem>()->isLoadingScene())
		return true;

	m_pImpl->m_LogicClientUpdateTask->Activate();
	m_pImpl->m_TransformComponentsSimulationTask->Activate();
	m_pImpl->m_PhysicsSystemUpdateBVHTask->Activate();
	m_pImpl->m_PhysicsSystemCullingTask->Activate();
	m_pImpl->m_RenderingFrontendUpdateTask->Activate();
	m_pImpl->m_RenderingServerUpdateTask->Activate();

	SystemUpdate(CameraSystem);
	SystemUpdate(LightSystem);

	SystemUpdate(PhysicsSystem);

	if (m_pImpl->m_WindowSystem->GetStatus() == ObjectStatus::Activated)
	{
		// auto l_RenderingFrontendUpdateTask = g_Engine->Get<TaskScheduler>()->Submit("RenderingFrontendUpdateTask", 1, l_PhysicsSystemCullingTask.m_Task, f_RenderingFrontendUpdateJob);

		// m_GUISystem->Update();

		// auto l_RenderingServerUpdateTask = g_Engine->Get<TaskScheduler>()->Submit("RenderingServerUpdateTask", 2, l_RenderingFrontendUpdateTask.m_Task, f_RenderingServerUpdateJob);
	}
	else
	{
		m_pImpl->m_ObjectStatus = ObjectStatus::Suspended;
		Get<Logger>()->Log(LogLevel::Warning, "Engine: Engine is stand-by.");
		return false;
	}

	Get<TaskScheduler>()->WaitSync();

	return true;
}

bool Engine::Terminate()
{
	Get<TaskScheduler>()->WaitSync();

	if (!m_pImpl->m_RenderingClient->Terminate())
	{
		Get<Logger>()->Log(LogLevel::Error, "Engine: Rendering client can't be terminated!");
		return false;
	}

	if (!m_pImpl->m_LogicClient->Terminate())
	{
		Get<Logger>()->Log(LogLevel::Error, "Engine: Logic client can't be terminated!");
		return false;
	}

	if (!m_pImpl->m_RenderingServer->Terminate())
	{
		Get<Logger>()->Log(LogLevel::Error, "Engine: RenderingServer can't be terminated!");
		return false;
	}

	SystemTerm(GUISystem);
	SystemTerm(RenderingFrontend);

	SystemTerm(PhysicsSystem);

	SystemTerm(TransformSystem);
	SystemTerm(LightSystem);
	SystemTerm(CameraSystem);

	if (!Get<EntityManager>()->Terminate())
	{
		Get<Logger>()->Log(LogLevel::Error, "Engine: EntityManager can't be terminated!");
		return false;
	}

	if (!m_pImpl->m_WindowSystem->Terminate())
	{
		Get<Logger>()->Log(LogLevel::Error, "Engine: WindowSystem can't be terminated!");
		return false;
	}

	SystemTerm(EventSystem);
	SystemTerm(SceneSystem);

	m_pImpl->m_ObjectStatus = ObjectStatus::Terminated;
	Get<Logger>()->Log(LogLevel::Success, "Engine has been terminated.");

	return true;
}

bool Engine::Run()
{
	while (1)
	{
		if (!ExecuteDefaultTask())
		{
			return false;
		}
	}
	return true;
}

ObjectStatus Engine::GetStatus()
{
	return m_pImpl->m_ObjectStatus;
}

InitConfig Engine::getInitConfig()
{
	return m_pImpl->m_initConfig;
}

IRenderingServer* Engine::getRenderingServer()
{
	return m_pImpl->m_RenderingServer.get();
}

IWindowSystem* Engine::getWindowSystem()
{
	return m_pImpl->m_WindowSystem.get();
}

float Engine::getTickTime()
{
	return m_pImpl->m_tickTime;
}

const FixedSizeString<128>& Engine::GetApplicationName()
{
	return m_pImpl->m_applicationName;
}
