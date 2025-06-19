#include "Engine.h"
#include "Common/Timer.h"
#include "Common/LogService.h"
#include "Common/Memory.h"
#include "Common/TaskScheduler.h"
#include "Common/IOService.h"
#include "Common/Task.h"
#include "Services/EntityManager.h"
#include "Services/ComponentManager.h"
#include "Services/LightSystem.h"
#include "Services/CameraSystem.h"
#include "Services/SceneService.h"
#include "Services/AssetService.h"
#include "Services/PhysicsSimulationService.h"
#include "Services/BVHService.h"
#include "Services/HIDService.h"
#include "Services/RenderingConfigurationService.h"
#include "Services/TemplateAssetService.h"
#include "Services/RenderingContextService.h"
#include "Services/AnimationService.h"
#include "Services/GUISystem.h"

// Platform-specific systems
#if defined INNO_PLATFORM_WIN
#include "Platform/WinWindow/WinWindowSystem.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "Platform/MacWindow/MacWindowSystem.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "Platform/LinuxWindow/LinuxWindowSystem.h"
#endif

// Rendering servers
#if defined INNO_RENDERER_DIRECTX
#include "RenderingServer/DX12/DX12RenderingServer.h"
#endif
#if defined INNO_RENDERER_VULKAN
#include "RenderingServer/VK/VKRenderingServer.h"
#endif
#if defined INNO_RENDERER_METAL
#include "RenderingServer/MT/MTRenderingServer.h"
#endif

// Headless stubs
#include "Platform/HeadlessWindow/HeadlessWindowSystem.h"
#include "RenderingServer/Headless/HeadlessRenderingServer.h"

namespace Inno
{
	Engine* g_Engine = nullptr;
}

using namespace Inno;

IWindowSystem* Engine::CreateWindowSystem(bool isHeadless)
{
	if (isHeadless) {
		return new HeadlessWindowSystem();
	}
	
#if defined INNO_PLATFORM_WIN
	return new WinWindowSystem();
#elif defined INNO_PLATFORM_MAC
	return new MacWindowSystem();
#elif defined INNO_PLATFORM_LINUX
	return new LinuxWindowSystem();
#else
	Log(Error, "No WindowSystem implementation available for this platform.");
	return nullptr;
#endif
}

IRenderingServer* Engine::CreateRenderingServer(bool isHeadless, RenderingServer renderingServerType)
{
	if (isHeadless) {
		return new HeadlessRenderingServer();
	}
	
	switch (renderingServerType) {
	case RenderingServer::DX12:
#if defined INNO_RENDERER_DIRECTX
		return new DX12RenderingServer();
#else
		Log(Error, "DirectX 12 renderer not available on this platform.");
		return nullptr;
#endif
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		return new VKRenderingServer();
#else
		Log(Error, "Vulkan renderer not available on this platform.");
		return nullptr;
#endif
	case RenderingServer::MT:
#if defined INNO_RENDERER_METAL
		return new MTRenderingServer();
#else
		Log(Error, "Metal renderer not available on this platform.");
		return nullptr;
#endif
	default:
		Log(Error, "Unknown rendering server type.");
		return nullptr;
	}
}

void Engine::ResolveDependencies(const std::vector<std::type_index>& dependencies)
{
	// For now, simple dependency resolution
	// Dependencies are assumed to be resolved by the order of Get<T>() calls
	// More sophisticated topological sorting can be added later if needed
}

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

		Handle<ITask> m_RenderingExecutionTask;

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

	auto l_headlessArgPos = arg.find("headless");
	if (l_headlessArgPos != std::string::npos)
	{
		l_result.isHeadless = true;
		Log(Success, "Launch in headless mode, no windowing or rendering systems.");
	}

	auto l_offscreenArgPos = arg.find("offscreen");
	if (l_offscreenArgPos != std::string::npos)
	{
		l_result.isOffscreen = true;
		Log(Success, "Launch in offscreen mode, no windowing but real rendering server for testing.");
	}

	return l_result;
}

bool Engine::CreateServices(void* appHook, void* extraHook, char* pScmdline)
{
	// Parse configuration first
	std::string l_windowArguments = pScmdline;
	m_pImpl->m_initConfig = ParseInitConfig(l_windowArguments);

	// Essential Services (always created, low-level)
	Get<Timer>();
	Get<LogService>();
	Get<Memory>();
	Get<TaskScheduler>();
	Get<IOService>();
	Get<HIDService>();

	// Create WindowSystem based on headless/offscreen mode
	if (m_pImpl->m_initConfig.isHeadless || m_pImpl->m_initConfig.isOffscreen) {
		m_pImpl->m_WindowSystem = std::make_unique<HeadlessWindowSystem>();
	} else {
#if defined INNO_PLATFORM_WIN
		m_pImpl->m_WindowSystem = std::make_unique<WinWindowSystem>();
#elif defined INNO_PLATFORM_MAC
		m_pImpl->m_WindowSystem = std::make_unique<MacWindowSystem>();
#elif defined INNO_PLATFORM_LINUX
		m_pImpl->m_WindowSystem = std::make_unique<LinuxWindowSystem>();
#endif
	}

	if (!m_pImpl->m_WindowSystem.get()) {
		Log(Error, "Failed to create Window System.");
		return false;
	}

	// Create other rendering-related services if not headless (but do create for offscreen)
	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<RenderingConfigurationService>();
		Get<TemplateAssetService>();
		Get<RenderingContextService>();
		Get<AnimationService>();
		Get<GUISystem>();
	}

	// Create RenderingServer based on headless mode (offscreen uses real rendering server)
	if (m_pImpl->m_initConfig.isHeadless) {
		m_pImpl->m_RenderingServer = std::make_unique<HeadlessRenderingServer>();
	} else {
		// For both windowed and offscreen modes, create real rendering server
		switch (m_pImpl->m_initConfig.renderingServer) {
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
		}
	}

	if (!m_pImpl->m_RenderingServer.get()) {
		Log(Error, "Failed to create Rendering Server.");
		return false;
	}

	// Platform-specific bridge setup for Mac
#if defined INNO_PLATFORM_MAC
	if (!m_pImpl->m_initConfig.isHeadless) {
		auto l_windowSystem = reinterpret_cast<MacWindowSystem*>(m_pImpl->m_WindowSystem.get());
		auto l_windowSystemBridge = reinterpret_cast<MacWindowSystemBridge*>(appHook);
		l_windowSystem->setBridge(l_windowSystemBridge);

		auto l_renderingServer = reinterpret_cast<MTRenderingServer*>(m_pImpl->m_RenderingServer.get());
		auto l_renderingServerBridge = reinterpret_cast<MTRenderingServerBridge*>(extraHook);
		l_renderingServer->setBridge(l_renderingServerBridge);
	}
#endif

	// Additional Systems (ISystem-based, with dependency resolution)
	Get<EntityManager>();
	Get<ComponentManager>();
	Get<AssetService>();
	Get<SceneService>();
	Get<PhysicsSimulationService>();
	Get<LightSystem>();
	Get<CameraSystem>();

	return true;
}

bool Engine::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	// Create all services (Essential + Additional Systems)
	if (!CreateServices(appHook, extraHook, pScmdline))
		return false;

	// Skip LogicClient and RenderingClient only in true headless mode
	if (!m_pImpl->m_initConfig.isHeadless)
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

		if (m_pImpl->m_initConfig.isOffscreen)
		{
			m_pImpl->m_applicationName = "OffscreenEngine";
			Log(Success, "Offscreen mode: Created LogicClient and RenderingClient for testing.");
		}
		else
		{
			m_pImpl->m_applicationName = m_pImpl->m_LogicClient->GetApplicationName();
		}
	}
	else
	{
		m_pImpl->m_applicationName = "HeadlessEngine";
		Log(Success, "Headless mode: Skipping LogicClient and RenderingClient.");
	}

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

	SystemSetup(AssetService);
	SystemSetup(SceneService);
	SystemSetup(PhysicsSimulationService);

	SystemSetup(LightSystem);
	SystemSetup(CameraSystem);

	SystemSetup(TemplateAssetService);

	if (!m_pImpl->m_RenderingServer->Setup(nullptr))
	{
		Log(Error, "Rendering Server can't be setup!");
		return false;
	}

	m_pImpl->m_RenderingServer->SetUploadHeapPreparationCallback([&]()
		{
			SystemUpdate(SceneService);
			
			if (Get<SceneService>()->IsLoading())
				return true;

			// Simulation - only if LogicClient exists
			if (m_pImpl->m_LogicClient) {
				m_pImpl->m_LogicClient->Update();
			}

			// Update components
			Get<CameraSystem>()->Update();
			Get<LightSystem>()->Update();

			SystemUpdate(EntityManager);

			// Culling
			// Get<PhysicsSimulationService>()->Update();
			// Get<BVHService>()->Update();
			// Get<PhysicsSimulationService>()->RunCulling();

			// Only update rendering-related services if not headless
			if (!m_pImpl->m_initConfig.isHeadless) {
				Get<RenderingContextService>()->Update();
				Get<AnimationService>()->Update();
				if (m_pImpl->m_RenderingClient) {
					m_pImpl->m_RenderingClient->Update();
				}
			}

			return true;
		});

	m_pImpl->m_RenderingServer->SetCommandPreparationCallback([&]()
		{
			if (Get<SceneService>()->IsLoading())
				return true;

			if (m_pImpl->m_RenderingClient) {
				m_pImpl->m_RenderingClient->PrepareCommands();
			}
			return true;
		});

	m_pImpl->m_RenderingServer->SetCommandExecutionCallback([&]()
		{
			if (Get<SceneService>()->IsLoading())
				return true;

			if (m_pImpl->m_RenderingClient) {
				m_pImpl->m_RenderingClient->ExecuteCommands();
			}
			return true;
		});

	// Only setup rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
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
	}

	// Only setup LogicClient if it exists
	if (m_pImpl->m_LogicClient) {
		if (!m_pImpl->m_LogicClient->Setup())
		{
			Log(Error, "Logic Client can't be setup!");
			return false;
		}
	}

	m_pImpl->m_RenderingExecutionTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Execution Task", ITask::Type::Recurrent, 2), [&]()
		{
			if (Get<HIDService>()->IsResizing())
				return true;

			auto l_tickStartTime = Get<Timer>()->GetCurrentTimeFromEpoch();

			m_pImpl->m_RenderingServer->Update();

			auto l_tickEndTime = Get<Timer>()->GetCurrentTimeFromEpoch();

			m_pImpl->m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

			return true;
		});

	m_pImpl->m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "Engine setup finished.");

	return true;
}

bool Engine::Initialize()
{
	SystemInit(HIDService);
	m_pImpl->m_WindowSystem->Initialize();

	SystemInit(EntityManager);

	SystemInit(SceneService);
	SystemInit(PhysicsSimulationService);

	SystemInit(LightSystem);
	SystemInit(CameraSystem);
	m_pImpl->m_RenderingServer->Initialize();

	// Only initialize rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		SystemInit(TemplateAssetService);
		SystemInit(RenderingContextService);
		SystemInit(AnimationService);

		ITask::Desc taskDesc("Default Rendering Client Initialization Task", ITask::Type::Once, 2);
		auto l_DefaultRenderingClientInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (!m_pImpl->m_RenderingClient->Initialize())
			{
				Log(Error, "Rendering Client can't be initialized!");
				return false;
			}

			SystemInit(GUISystem);

			return true;
			});

		l_DefaultRenderingClientInitializationTask->Activate();
		l_DefaultRenderingClientInitializationTask->Wait();

		// Check if m_RenderingExecutionTask exists before activating
		if (m_pImpl->m_RenderingExecutionTask)
		{
			Log(Verbose, "Activating rendering execution task...");
			m_pImpl->m_RenderingExecutionTask->Activate();
		}
		else
		{
			Log(Error, "m_RenderingExecutionTask is null!");
		}
	}

	// Only initialize LogicClient if it exists
	if (m_pImpl->m_LogicClient) {
		m_pImpl->m_LogicClient->Initialize();
	}

	m_pImpl->m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "Engine has been initialized.");

	return true;
}

bool Engine::ExecuteDefaultTask()
{
	Get<Timer>()->Tick();

	m_pImpl->m_WindowSystem->Update();
	SystemUpdate(HIDService);

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
	// Only wait for rendering task if it was created
	if (m_pImpl->m_RenderingExecutionTask) {
		m_pImpl->m_RenderingExecutionTask->Wait();
		m_pImpl->m_RenderingExecutionTask->Deactivate();
	}

	// Only terminate LogicClient if it exists
	if (m_pImpl->m_LogicClient) {
		if (!m_pImpl->m_LogicClient->Terminate())
		{
			Log(Error, "Logic client can't be terminated!");
			return false;
		}
	}

	// Only terminate rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		ITask::Desc taskDesc("Default Rendering Client Termination Task", ITask::Type::Once, 2);
		auto l_DefaultRenderingClientTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			SystemTerm(GUISystem);

			if (m_pImpl->m_RenderingClient && !m_pImpl->m_RenderingClient->Terminate())
			{
				Log(Error, "Rendering client can't be terminated!");
				return false;
			}
			return true;
			});
		l_DefaultRenderingClientTerminationTask->Activate();
		l_DefaultRenderingClientTerminationTask->Wait();

		SystemTerm(AnimationService);
		SystemTerm(RenderingContextService);
		SystemTerm(TemplateAssetService);
	}
	
	if (!m_pImpl->m_RenderingServer->Terminate())
	{
		Log(Error, "RenderingServer can't be terminated!");
		return false;
	}

	SystemTerm(CameraSystem);
	SystemTerm(LightSystem);

	SystemTerm(PhysicsSimulationService);
	SystemTerm(SceneService);

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

	Get<TaskScheduler>()->Freeze();
	Get<TaskScheduler>()->Reset();

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
