#include "Engine.h"
#include "Common/Timer.h"
#include "Common/LogService.h"
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
#include "Services/BVHService.h"
#include "Services/HIDService.h"
#include "Services/RenderingConfigurationService.h"
#include "Services/TemplateAssetService.h"
#include "Services/RenderingContextService.h"
#include "Services/AnimationService.h"
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
#include "RenderingServer/DX12/DX12RenderingServer.h"
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

		std::function<void()> f_SceneLoadingStartedCallback;
		std::function<void()> f_SceneLoadingFinishedCallback;

		Handle<ITask> m_LogicClientUpdateTask;
		Handle<ITask> m_ComponentSystemUpdateTask;
		Handle<ITask> m_CullingTask;
		Handle<ITask> m_RenderingServerPreparationTask;
		Handle<ITask> m_RenderingClientTask;
		Handle<ITask> m_RenderingServerTask;

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
		Log(Warning, "No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		Log(Warning, "No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::Host;
			Log(Success, "Launch in host mode, engine will handle OS event.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::Slave;
			Log(Success, "Launch in slave mode, engine requires client handle OS event.");
		}
		else
		{
			Log(Warning, "Unsupported engine mode.");
		}
	}

	auto l_renderingServerArgPos = arg.find("renderer");

	if (l_renderingServerArgPos == std::string::npos)
	{
		Log(Error, "No rendering backend argument found.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_renderingServerArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.renderingServer = RenderingServer::DX12;
#else
			Log(Warning, "DirectX 12 is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingServer = RenderingServer::VK;
#else
			Log(Warning, "Vulkan is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_METAL
			l_result.renderingServer = RenderingServer::MT;
#else
			Log(Warning, "Metal is not supported on current platform.");
#endif
		}
	}

	auto l_logLevelArgPos = arg.find("loglevel");
	if (l_engineModeArgPos == std::string::npos)
	{
		Get<LogService>()->SetDefaultLogLevel(LogLevel::Success);
	}
	else
	{
		std::string l_logLevelArguments = arg.substr(l_logLevelArgPos + 9);
		l_logLevelArguments = l_logLevelArguments.substr(0, 1);

		if (l_logLevelArguments == "0")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Verbose);
		}
		else if (l_logLevelArguments == "1")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Success);
		}
		else if (l_logLevelArguments == "2")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Warning);
		}
		else if (l_logLevelArguments == "3")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Error);
		}
		else
		{
			Log(Warning, "Unsupported log level.");
		}
	}

	return l_result;
}

bool Engine::CreateServices(void* appHook, void* extraHook, char* pScmdline)
{
	Get<Timer>();
	Get<LogService>();
	Get<Memory>();
	Get<TaskScheduler>();
	Get<IOService>();

	Get<HIDService>();

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
		Log(Error, "Failed to create Window System.");
		return false;
	}

	Get<RenderingConfigurationService>();
	Get<TemplateAssetService>();
	Get<RenderingContextService>();
	Get<AnimationService>();
	Get<GUISystem>();

	switch (m_pImpl->m_initConfig.renderingServer)
	{
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
		Log(Error, "Failed to create Rendering Server.");
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
		Log(Error, "Failed to create Rendering Client.");
		return false;
	}

	m_pImpl->m_LogicClient = std::make_unique<INNO_LOGIC_CLIENT>();
	if (!m_pImpl->m_LogicClient.get())
	{
		Log(Error, "Failed to create Logic Client.");
		return false;
	}

	m_pImpl->m_applicationName = m_pImpl->m_LogicClient->GetApplicationName();

	if (!CreateServices(appHook, extraHook, pScmdline))
		return false;

	SystemSetup(HIDService);

	IWindowSystemConfig l_windowSystemConfig;
	l_windowSystemConfig.m_AppHook = appHook;
	l_windowSystemConfig.m_ExtraHook = extraHook;

	if (!m_pImpl->m_WindowSystem->Setup(&l_windowSystemConfig))
	{
		Log(Error, "Window System can't be setup!");
		return false;
	}

	SystemSetup(EntityManager);

	SystemSetup(AssetSystem);
	SystemSetup(SceneSystem);
	SystemSetup(PhysicsSystem);

	SystemSetup(TransformSystem);
	SystemSetup(LightSystem);
	SystemSetup(CameraSystem);

	SystemSetup(TemplateAssetService);

	if (!m_pImpl->m_RenderingServer->Setup(nullptr))
	{
		Log(Error, "Rendering Server can't be setup!");
		return false;
	}

	SystemSetup(RenderingContextService);
	SystemSetup(AnimationService);

	ITask::Desc taskDesc("Default Rendering Client Setup Task", ITask::Type::Once, 2);
	auto l_DefaultRenderingClientSetupTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
		if (!m_pImpl->m_RenderingClient->Setup())
		{
			Log(Error, "Rendering Client can't be setup!");
			return false;
		}
		
		SystemSetup(GUISystem);

		return true;
		});

	l_DefaultRenderingClientSetupTask->Activate();
	l_DefaultRenderingClientSetupTask->Wait();

	if (!m_pImpl->m_LogicClient->Setup())
	{
		Log(Error, "Logic Client can't be setup!");
		return false;
	}

	m_pImpl->m_LogicClientUpdateTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Logic Client Update Task"), [&]()
		{
			if (Get<SceneSystem>()->isLoadingScene())
				return;

			m_pImpl->m_LogicClient->Update();
		});

	m_pImpl->m_ComponentSystemUpdateTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Component System Update Task"), [&]()
		{
			if (Get<SceneSystem>()->isLoadingScene())
				return true;

			m_pImpl->m_LogicClientUpdateTask->Wait(); // Not wait, but check if it's done and consume the result
			
			Get<TransformSystem>()->Update();
			Get<CameraSystem>()->Update();
			Get<LightSystem>()->Update();

			SystemUpdate(EntityManager);
			return true;
		});

	m_pImpl->m_CullingTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Culling Task"), [&]()
		{
			if (Get<SceneSystem>()->isLoadingScene())
				return true;

			m_pImpl->m_ComponentSystemUpdateTask->Wait();

			Get<PhysicsSystem>()->Update();
			Get<BVHService>()->Update();
			Get<PhysicsSystem>()->RunCulling();

			return true;
		});

	m_pImpl->m_RenderingServerPreparationTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Server Preparation Task", ITask::Type::Recurrent, 2), [&]()
		{
			if (!Get<SceneSystem>()->isLoadingScene())
			{
				m_pImpl->m_CullingTask->Wait();

				Get<RenderingContextService>()->Update();
				Get<AnimationService>()->Update();
				m_pImpl->m_RenderingClient->Prepare();
			}

			m_pImpl->m_RenderingServer->Update();

			return true;
		});

	m_pImpl->m_RenderingClientTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Client Task", ITask::Type::Recurrent, 2), [&]()
		{
			if (Get<SceneSystem>()->isLoadingScene())
				return;

			m_pImpl->m_RenderingServerPreparationTask->Wait();
			Get<GUISystem>()->Update();
		});

	m_pImpl->m_RenderingServerTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Server Task", ITask::Type::Recurrent, 2), [&]()
		{
			if (Get<HIDService>()->IsResizing())
				return true;

			m_pImpl->m_RenderingClientTask->Wait();

			auto l_tickStartTime = Get<Timer>()->GetCurrentTimeFromEpoch();

			m_pImpl->m_RenderingServer->TransferDataToGPU();

			if (!Get<SceneSystem>()->isLoadingScene())
			{
				m_pImpl->m_RenderingClient->ExecuteCommands();
			}

			m_pImpl->m_RenderingServer->FinalizeSwapChain();

			Get<GUISystem>()->ExecuteCommands();

			m_pImpl->m_RenderingServer->Present();

			m_pImpl->m_WindowSystem->GetWindowSurface()->swapBuffer();

			auto l_tickEndTime = Get<Timer>()->GetCurrentTimeFromEpoch();

			m_pImpl->m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

			return true;
		});

	m_pImpl->f_SceneLoadingStartedCallback = [&]()
		{
			m_pImpl->m_LogicClientUpdateTask->Deactivate();
			m_pImpl->m_ComponentSystemUpdateTask->Deactivate();
			m_pImpl->m_CullingTask->Deactivate();
			m_pImpl->m_RenderingClientTask->Deactivate();
		};

	m_pImpl->f_SceneLoadingFinishedCallback = [&]()
		{
			m_pImpl->m_LogicClientUpdateTask->Activate();
			m_pImpl->m_ComponentSystemUpdateTask->Activate();
			m_pImpl->m_CullingTask->Activate();
			m_pImpl->m_RenderingClientTask->Activate();
		};

	Get<SceneSystem>()->AddSceneLoadingStartedCallback(&m_pImpl->f_SceneLoadingStartedCallback, -1);
	Get<SceneSystem>()->AddSceneLoadingFinishedCallback(&m_pImpl->f_SceneLoadingFinishedCallback, 10);

	m_pImpl->m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "Engine setup finished.");

	return true;
}

bool Engine::Initialize()
{
	SystemInit(HIDService);
	m_pImpl->m_WindowSystem->Initialize();

	SystemInit(EntityManager);

	SystemInit(SceneSystem);
	SystemInit(PhysicsSystem);

	SystemInit(TransformSystem);
	SystemInit(LightSystem);
	SystemInit(CameraSystem);
	m_pImpl->m_RenderingServer->Initialize();

	SystemInit(TemplateAssetService);
	SystemInit(RenderingContextService);
	SystemInit(AnimationService);

	ITask::Desc taskDesc("Default Rendering Client Initialization Task", ITask::Type::Once, 2);
	auto l_DefaultRenderingClientInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
		if (!m_pImpl->m_RenderingClient->Initialize())
		{
			Log(Error, "Rendering Client can't be setup!");
			return false;
		}

		SystemInit(GUISystem);

		return true;
		});

	l_DefaultRenderingClientInitializationTask->Activate();
	l_DefaultRenderingClientInitializationTask->Wait();

	m_pImpl->m_RenderingServerPreparationTask->Activate();
	m_pImpl->m_RenderingServerTask->Activate();

	m_pImpl->m_LogicClient->Initialize();

	m_pImpl->m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "Engine has been initialized.");

	return true;
}

bool Engine::ExecuteDefaultTask()
{
	Get<Timer>()->Tick();

	m_pImpl->m_WindowSystem->Update();
	SystemUpdate(HIDService);
	SystemUpdate(SceneSystem);

	if (m_pImpl->m_WindowSystem->GetStatus() != ObjectStatus::Activated)
	{
		m_pImpl->m_ObjectStatus = ObjectStatus::Suspended;
		Log(Warning, "Engine is stand-by.");
		return false;
	}

	return true;
}

bool Engine::Terminate()
{
	m_pImpl->m_RenderingServerTask->Deactivate();
	m_pImpl->m_RenderingClientTask->Deactivate();
	m_pImpl->m_RenderingServerPreparationTask->Deactivate();
	m_pImpl->m_CullingTask->Deactivate();
	m_pImpl->m_ComponentSystemUpdateTask->Deactivate();
	m_pImpl->m_LogicClientUpdateTask->Deactivate();

	ITask::Desc taskDesc("Default Rendering Client Termination Task", ITask::Type::Once, 2);
	auto l_DefaultRenderingClientTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
		if (!m_pImpl->m_RenderingClient->Terminate())
		{
			Log(Error, "Rendering client can't be terminated!");
			return false;
		}
		return true;
		});
	l_DefaultRenderingClientTerminationTask->Activate();
	l_DefaultRenderingClientTerminationTask->Wait();

	Get<TaskScheduler>()->Freeze();
	Get<TaskScheduler>()->Reset();

	if (!m_pImpl->m_LogicClient->Terminate())
	{
		Log(Error, "Logic client can't be terminated!");
		return false;
	}

	if (!m_pImpl->m_RenderingServer->Terminate())
	{
		Log(Error, "RenderingServer can't be terminated!");
		return false;
	}

	SystemTerm(GUISystem);
	SystemTerm(RenderingContextService);

	SystemTerm(PhysicsSystem);

	SystemTerm(TransformSystem);
	SystemTerm(LightSystem);
	SystemTerm(CameraSystem);

	if (!Get<EntityManager>()->Terminate())
	{
		Log(Error, "EntityManager can't be terminated!");
		return false;
	}

	if (!m_pImpl->m_WindowSystem->Terminate())
	{
		Log(Error, "WindowSystem can't be terminated!");
		return false;
	}

	SystemTerm(HIDService);
	SystemTerm(SceneSystem);

	m_pImpl->m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "Engine has been terminated.");

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
