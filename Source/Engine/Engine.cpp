#include "Engine.h"
#include "Common/Timer.h"
#include "Common/LogService.h"
#include "Common/Memory.h"
#include "Common/TaskScheduler.h"
#include "Common/IOService.h"
#include "Common/Task.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
#include "Services/LightSimulationService.h"
#include "Services/CameraService.h"
#include "Services/SceneService.h"
#include "Services/AssetService.h"
#include "Services/PhysicsSimulationService.h"
#include "Services/BVHService.h"
#include "Services/HIDService.h"
#include "Services/RenderingConfigurationService.h"
#include "Services/TemplateAssetService.h"
#include "Services/PerFrameDataService.h"
#include "Services/LightDataService.h"
#include "Services/DrawCallService.h"
#include "Services/AnimationDrawCallService.h"
#include "Services/BillboardDrawCallService.h"
#include "Services/DebugDrawCallService.h"
#include "Services/AnimationResourceService.h"
#include "Services/AnimationSimulationService.h"
#include "Services/GUIService.h"
#include "Services/GraphicsHardwareService.h"

// Platform-specific systems
#if defined INNO_PLATFORM_WIN
#include "Platform/WinWindow/WinWindowService.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "Platform/MacWindow/MacWindowService.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "Platform/LinuxWindow/LinuxWindowService.h"
#endif

// Rendering servers
#if defined INNO_RENDERER_DIRECTX
#include "Services/DX12/DX12GraphicsHardwareService.h"
#include "Services/DX12/DX12FrameManagementService.h"
#include "Services/DX12/DX12CommandListResourceService.h"
#include "Services/DX12/DX12SamplerResourceService.h"
#include "Services/DX12/DX12ShaderProgramResourceService.h"
#include "Services/DX12/DX12TextureResourceService.h"
#include "Services/DX12/DX12GPUBufferResourceService.h"
#include "Services/DX12/DX12MeshResourceService.h"
#include "Services/DX12/DX12MaterialResourceService.h"
#include "Services/DX12/DX12RenderPassResourceService.h"
#endif

// Headless window stub
#include "Platform/HeadlessWindow/HeadlessWindowService.h"

namespace Inno
{
	Engine* g_Engine = nullptr;
}

using namespace Inno;

IWindowService* Engine::CreateWindowSystem(bool isHeadless)
{
	if (isHeadless) {
		return new HeadlessWindowService();
	}
	
#if defined INNO_PLATFORM_WIN
	return new WinWindowService();
#elif defined INNO_PLATFORM_MAC
	return new MacWindowService();
#elif defined INNO_PLATFORM_LINUX
	return new LinuxWindowService();
#else
	Log(Error, "No WindowSystem implementation available for this platform.");
	return nullptr;
#endif
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

		std::unique_ptr<IWindowService> m_WindowSystem;

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

	auto l_graphicsServiceArgPos = arg.find("renderer");

	if (l_graphicsServiceArgPos == std::string::npos)
	{
		Log(Error, "No rendering backend argument found.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_graphicsServiceArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.graphicsService = GraphicsService::DX12;
#else
			Log(Warning, "DirectX 12 is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.graphicsService = GraphicsService::VK;
#else
			Log(Warning, "Vulkan is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_METAL
			l_result.graphicsService = GraphicsService::MT;
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

	if (arg.find("audit") != std::string::npos)
	{
		l_result.isAudit = true;
		Log(Success, "Audit mode: will dump all pass outputs on frame 5.");
	}

	auto l_testArgPos = arg.find("-test");
	if (l_testArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_testArgPos + 5);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			auto l_end = l_remainder.find(' ', l_start);
			std::string l_caseName = l_remainder.substr(l_start,
				l_end == std::string::npos ? std::string::npos : l_end - l_start);
			strncpy(l_result.testCase, l_caseName.c_str(), sizeof(l_result.testCase) - 1);
			Log(Success, "Test case: ", l_result.testCase);
		}
		else
		{
			Log(Warning, "'-test' flag found but no test case name provided. Ignoring.");
		}
	}

	auto l_framesArgPos = arg.find("-frames");
	if (l_framesArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_framesArgPos + 7);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.maxFrames = std::stoi(l_remainder.substr(l_start));
			Log(Success, "Auto-terminate after ", l_result.maxFrames, " frames post-GI-scene-load.");
		}
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
	Get<IOService>()->setupWorkingDirectory();
	Get<HIDService>();

	// Create WindowSystem based on headless/offscreen mode
	if (m_pImpl->m_initConfig.isHeadless || m_pImpl->m_initConfig.isOffscreen) {
		m_pImpl->m_WindowSystem = std::make_unique<HeadlessWindowService>();
	} else {
#if defined INNO_PLATFORM_WIN
		m_pImpl->m_WindowSystem = std::make_unique<WinWindowService>();
#elif defined INNO_PLATFORM_MAC
		m_pImpl->m_WindowSystem = std::make_unique<MacWindowService>();
#elif defined INNO_PLATFORM_LINUX
		m_pImpl->m_WindowSystem = std::make_unique<LinuxWindowService>();
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
		Get<PerFrameDataService>();
		Get<LightDataService>();
		Get<DrawCallService>();
		Get<AnimationDrawCallService>();
		Get<BillboardDrawCallService>();
		Get<DebugDrawCallService>();
		Get<AnimationSimulationService>();
		Get<AnimationResourceService>();
		if (!m_pImpl->m_initConfig.isOffscreen)
			Get<GUIService>();
	}

	// Create focused graphics services (DX12-specific derived classes)
#if defined INNO_RENDERER_DIRECTX
	if (!m_pImpl->m_initConfig.isHeadless)
	{
		auto* l_hwService = new DX12GraphicsHardwareService();
		auto* l_ctx = l_hwService->GetDX12Context();

		auto* l_fmService = new DX12FrameManagementService();
		l_fmService->SetDX12Context(l_ctx);

		l_hwService->SetFrameManagementService(l_fmService);
		l_fmService->SetHardwareService(l_hwService);

		singletons_[std::type_index(typeid(GraphicsHardwareService))] = l_hwService;
		singletons_[std::type_index(typeid(FrameManagementService))] = l_fmService;

		auto* l_cmdListService = new DX12CommandListResourceService();
		l_cmdListService->SetDX12Context(l_ctx);
		auto* l_samplerService = new DX12SamplerResourceService();
		l_samplerService->SetDX12Context(l_ctx);
		auto* l_shaderService = new DX12ShaderProgramResourceService();
		l_shaderService->SetDX12Context(l_ctx);
		auto* l_textureService = new DX12TextureResourceService();
		l_textureService->SetDX12Context(l_ctx);
		auto* l_gpuBufferService = new DX12GPUBufferResourceService();
		l_gpuBufferService->SetDX12Context(l_ctx);
		auto* l_meshService = new DX12MeshResourceService();
		l_meshService->SetDX12Context(l_ctx);
		auto* l_materialService = new DX12MaterialResourceService();
		l_materialService->SetDX12Context(l_ctx);
		auto* l_renderPassService = new DX12RenderPassResourceService();
		l_renderPassService->SetDX12Context(l_ctx);

		singletons_[std::type_index(typeid(CommandListResourceService))] = l_cmdListService;
		singletons_[std::type_index(typeid(SamplerResourceService))] = l_samplerService;
		singletons_[std::type_index(typeid(ShaderProgramResourceService))] = l_shaderService;
		singletons_[std::type_index(typeid(TextureResourceService))] = l_textureService;
		singletons_[std::type_index(typeid(GPUBufferResourceService))] = l_gpuBufferService;
		singletons_[std::type_index(typeid(MeshResourceService))] = l_meshService;
		singletons_[std::type_index(typeid(MaterialResourceService))] = l_materialService;
		singletons_[std::type_index(typeid(RenderPassResourceService))] = l_renderPassService;
	}
#endif

	// Platform-specific bridge setup for Mac
#if defined INNO_PLATFORM_MAC
	if (!m_pImpl->m_initConfig.isHeadless) {
		auto l_windowSystem = reinterpret_cast<MacWindowService*>(m_pImpl->m_WindowSystem.get());
		auto l_windowSystemBridge = reinterpret_cast<MacWindowServiceBridge*>(appHook);
		l_windowSystem->setBridge(l_windowSystemBridge);
		// TODO: Metal bridge setup needs to go through the focused service pattern
	}
#endif

	// Additional Systems (IService-based, with dependency resolution)
	Get<EntityRegistry>();
	Get<AssetService>();
	Get<SceneService>();
	Get<PhysicsSimulationService>();
	Get<LightSimulationService>();
	Get<CameraService>();

	return true;
}

bool Engine::Setup(void* appHook, void* extraHook, char* pScmdline,
	std::unique_ptr<IRenderingClient> renderingClient,
	std::unique_ptr<ILogicClient> logicClient)
{
	// Create all services (Essential + Additional Systems)
	if (!CreateServices(appHook, extraHook, pScmdline))
		return false;

	m_pImpl->m_RenderingClient = std::move(renderingClient);
	m_pImpl->m_LogicClient = std::move(logicClient);

	if (m_pImpl->m_LogicClient)
	{
		if (m_pImpl->m_initConfig.isOffscreen)
		{
			m_pImpl->m_applicationName = "OffscreenEngine";
			Log(Success, "Offscreen mode: LogicClient and RenderingClient injected for testing.");
		}
		else
		{
			m_pImpl->m_applicationName = m_pImpl->m_LogicClient->GetApplicationName();
		}
	}
	else
	{
		m_pImpl->m_applicationName = "HeadlessEngine";
		Log(Success, "No clients injected: running headless.");
	}

	SystemSetup(HIDService);

	IWindowServiceConfig l_windowSystemConfig;
	l_windowSystemConfig.m_AppHook = appHook;
	l_windowSystemConfig.m_ExtraHook = extraHook;

	if (!m_pImpl->m_WindowSystem->Setup(&l_windowSystemConfig))
	{
		Log(Error, "Window System can't be setup!");
		return false;
	}

	SystemSetup(EntityRegistry);
	SystemSetup(TransformService);

	SystemSetup(AssetService);
	SystemSetup(SceneService);
	SystemSetup(PhysicsSimulationService);

	SystemSetup(LightSimulationService);
	SystemSetup(CameraService);

	SystemSetup(TemplateAssetService);

	if (!m_pImpl->m_initConfig.isHeadless)
	{
		if (!Get<CommandListResourceService>()->Setup())
		{
			Log(Error, "CommandListResourceService can't be setup!");
			return false;
		}
		if (!Get<SamplerResourceService>()->Setup())
		{
			Log(Error, "SamplerResourceService can't be setup!");
			return false;
		}
		if (!Get<ShaderProgramResourceService>()->Setup())
		{
			Log(Error, "ShaderProgramResourceService can't be setup!");
			return false;
		}
		if (!Get<TextureResourceService>()->Setup())
		{
			Log(Error, "TextureResourceService can't be setup!");
			return false;
		}
		if (!Get<GPUBufferResourceService>()->Setup())
		{
			Log(Error, "GPUBufferResourceService can't be setup!");
			return false;
		}
		if (!Get<MeshResourceService>()->Setup())
		{
			Log(Error, "MeshResourceService can't be setup!");
			return false;
		}
		if (!Get<MaterialResourceService>()->Setup())
		{
			Log(Error, "MaterialResourceService can't be setup!");
			return false;
		}
		if (!Get<RenderPassResourceService>()->Setup())
		{
			Log(Error, "RenderPassResourceService can't be setup!");
			return false;
		}

		if (!Get<GraphicsHardwareService>()->Setup())
		{
			Log(Error, "GraphicsHardwareService can't be setup!");
			return false;
		}

		if (!Get<FrameManagementService>()->Setup())
		{
			Log(Error, "FrameManagementService can't be setup!");
			return false;
		}
	}

	// Only setup rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<FrameManagementService>()->SetUploadHeapPreparationCallback([&]()
			{
				SystemUpdate(SceneService);

				if (Get<SceneService>()->IsLoading())
					return true;

				// Simulation - only if LogicClient exists
				if (m_pImpl->m_LogicClient) {
					m_pImpl->m_LogicClient->Update();
				}

				// Update components
				Get<CameraService>()->Update();
				Get<LightSimulationService>()->Update();

				SystemUpdate(EntityRegistry);

				Get<TransformService>()->Update();

				Get<PerFrameDataService>()->Update();
				Get<LightDataService>()->Update();
				Get<DrawCallService>()->Update();
				Get<AnimationDrawCallService>()->Update();
				Get<BillboardDrawCallService>()->Update();
				Get<DebugDrawCallService>()->Update();
				Get<AnimationSimulationService>()->Update();
				Get<AnimationResourceService>()->Update();
				if (m_pImpl->m_RenderingClient) {
					m_pImpl->m_RenderingClient->Update();
				}

				return true;
			});

		Get<FrameManagementService>()->SetCommandPreparationCallback([&]()
			{
				if (Get<SceneService>()->IsLoading())
					return true;

				if (m_pImpl->m_RenderingClient) {
					m_pImpl->m_RenderingClient->PrepareCommands();
				}
				return true;
			});

		Get<FrameManagementService>()->SetCommandExecutionCallback([&]()
			{
				if (Get<SceneService>()->IsLoading())
					return true;

				if (m_pImpl->m_RenderingClient) {
					m_pImpl->m_RenderingClient->ExecuteCommands();
				}
				return true;
			});
	}

	// Only setup rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		SystemSetup(PerFrameDataService);
		SystemSetup(LightDataService);
		SystemSetup(DrawCallService);
		SystemSetup(AnimationDrawCallService);
		SystemSetup(BillboardDrawCallService);
		SystemSetup(DebugDrawCallService);
		SystemSetup(AnimationSimulationService);
		SystemSetup(AnimationResourceService);

		ITask::Desc taskDesc("Default Rendering Client Setup Task", ITask::Type::Once, 2);
		auto l_DefaultRenderingClientSetupTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (m_pImpl->m_RenderingClient) {
				if (!m_pImpl->m_RenderingClient->Setup())
				{
					Log(Error, "Rendering Client can't be setup!");
					return false;
				}
			}

			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemSetup(GUIService);

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

	if (!m_pImpl->m_initConfig.isHeadless)
	{
		m_pImpl->m_RenderingExecutionTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Execution Task", ITask::Type::Recurrent, 2), [&]()
			{
				if (Get<HIDService>()->IsResizing())
					return true;

				auto l_tickStartTime = Get<Timer>()->GetCurrentTimeFromEpoch();

				Get<FrameManagementService>()->Update();

				auto l_tickEndTime = Get<Timer>()->GetCurrentTimeFromEpoch();

				m_pImpl->m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

				return true;
			});
	}

	m_pImpl->m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "Engine setup finished.");

	return true;
}

bool Engine::Initialize()
{
	SystemInit(HIDService);
	m_pImpl->m_WindowSystem->Initialize();

	SystemInit(EntityRegistry);
	SystemInit(TransformService);

	SystemInit(SceneService);
	SystemInit(PhysicsSimulationService);

	SystemInit(LightSimulationService);
	SystemInit(CameraService);

	// Only initialize rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<FrameManagementService>()->Initialize();
		SystemInit(TemplateAssetService);
		SystemInit(PerFrameDataService);
		SystemInit(LightDataService);
		SystemInit(DrawCallService);
		SystemInit(AnimationDrawCallService);
		SystemInit(BillboardDrawCallService);
		SystemInit(DebugDrawCallService);
		SystemInit(AnimationSimulationService);
		SystemInit(AnimationResourceService);

		ITask::Desc taskDesc("Default Rendering Client Initialization Task", ITask::Type::Once, 2);
		auto l_DefaultRenderingClientInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (m_pImpl->m_RenderingClient) {
				if (!m_pImpl->m_RenderingClient->Initialize())
				{
					Log(Error, "Rendering Client can't be initialized!");
					return false;
				}
			}

			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemInit(GUIService);

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
			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemTerm(GUIService);

			if (m_pImpl->m_RenderingClient && !m_pImpl->m_RenderingClient->Terminate())
			{
				Log(Error, "Rendering client can't be terminated!");
				return false;
			}
			return true;
			});
		l_DefaultRenderingClientTerminationTask->Activate();
		l_DefaultRenderingClientTerminationTask->Wait();

		SystemTerm(AnimationResourceService);
		SystemTerm(AnimationSimulationService);
		SystemTerm(DebugDrawCallService);
		SystemTerm(BillboardDrawCallService);
		SystemTerm(AnimationDrawCallService);
		SystemTerm(DrawCallService);
		SystemTerm(LightDataService);
		SystemTerm(PerFrameDataService);
		SystemTerm(TemplateAssetService);

		Get<FrameManagementService>()->Terminate();
		Get<RenderPassResourceService>()->Terminate();
		Get<MaterialResourceService>()->Terminate();
		Get<MeshResourceService>()->Terminate();
		Get<GPUBufferResourceService>()->Terminate();
		Get<TextureResourceService>()->Terminate();
		Get<ShaderProgramResourceService>()->Terminate();
		Get<SamplerResourceService>()->Terminate();
		Get<CommandListResourceService>()->Terminate();
	}

	SystemTerm(CameraService);
	SystemTerm(LightSimulationService);

	SystemTerm(PhysicsSimulationService);
	SystemTerm(SceneService);
	SystemTerm(TransformService);

	SystemTerm(EntityRegistry);

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

IWindowService* Engine::getWindowService()
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
