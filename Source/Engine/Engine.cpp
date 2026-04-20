#include "Engine.h"
#include <chrono>
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
#include "Services/EditorService.h"
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
	auto l_sidecarArgPos = arg.find("sidecar");

	if (l_engineModeArgPos == std::string::npos)
	{
		if (l_sidecarArgPos == std::string::npos)
		{
			Log(Warning, "No engine mode argument found, use default game mode.");
		}
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
		else if (l_engineModeArguments == "2")
		{
			l_result.engineMode = EngineMode::Sidecar;
			Log(Success, "Launch in sidecar mode, engine will be controlled by external process.");
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
	if (l_logLevelArgPos == std::string::npos)
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

	if (l_sidecarArgPos != std::string::npos)
	{
		l_result.engineMode = EngineMode::Sidecar;
		Log(Success, "Launch in sidecar mode, engine will be controlled by external process.");
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

	auto l_framesArgPos = arg.find("-total_frames");
	if (l_framesArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_framesArgPos + 13);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.totalFrames = std::stoi(l_remainder.substr(l_start));
			Log(Success, "Auto-terminate after ", l_result.totalFrames, " frames.");
		}
	}

	auto l_reloadArgPos = arg.find("-reload_at_frame");
	if (l_reloadArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_reloadArgPos + 16);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.reloadAtFrame = std::stoi(l_remainder.substr(l_start));
			Log(Success, "Scene reload at frame ", l_result.reloadAtFrame, ".");
		}
	}

	if (arg.find("-gpu_validation") != std::string::npos)
	{
		l_result.enableGPUValidation = true;
		Log(Success, "D3D12 GPU-based validation enabled.");
	}

	auto l_serializeTestPos = arg.find("-serialize_test");
	if (l_serializeTestPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_serializeTestPos + 15);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			auto l_end = l_remainder.find(' ', l_start);
			std::string l_path = l_remainder.substr(l_start,
				l_end == std::string::npos ? std::string::npos : l_end - l_start);
			if (l_path.size() < sizeof(l_result.serializeTest))
			{
				std::memcpy(l_result.serializeTest, l_path.c_str(), l_path.size() + 1);
				Log(Success, "Serialize-determinism test on scene: ", l_result.serializeTest);
				l_result.isOffscreen = true; // render pipeline not required
				l_result.totalFrames = 1;    // exit immediately after save
			}
		}
	}

	auto l_captureArgPos = arg.find("-capture_frame");
	if (l_captureArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_captureArgPos + 14);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.captureFrame = std::stoi(l_remainder.substr(l_start));
			Log(Success, "RenderDoc capture at frame ", l_result.captureFrame, ".");
		}
	}

	auto l_parentPidArgPos = arg.find("-parent_pid");
	if (l_parentPidArgPos != std::string::npos)
	{
		auto l_remainder = arg.substr(l_parentPidArgPos + 12);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.parentPID = std::stoul(l_remainder.substr(l_start));
			Log(Success, "Parent PID set to: ", l_result.parentPID);
		}
	}

	// Bake mode (TASK-68): `-bake "path1.gltf;path2.fbx;..."` runs a one-shot
	// asset-import-then-exit pass with no window or rendering services. The
	// quoted argument is a `;`-separated list of paths relative to the
	// working directory. Implies -headless.
	auto l_bakeArgPos = arg.find("-bake");
	if (l_bakeArgPos != std::string::npos)
	{
		auto l_remainder = arg.substr(l_bakeArgPos + 5);
		// Accept either `-bake "a;b"` (quoted) or `-bake a;b` (unquoted, ends at next arg).
		auto l_start = l_remainder.find_first_not_of(" \t");
		if (l_start != std::string::npos)
		{
			size_t l_end = std::string::npos;
			if (l_remainder[l_start] == '"')
			{
				++l_start;
				l_end = l_remainder.find('"', l_start);
			}
			else
			{
				l_end = l_remainder.find_first_of(" \t", l_start);
			}
			const std::string l_list = l_remainder.substr(
				l_start, l_end == std::string::npos ? std::string::npos : l_end - l_start);
			strncpy(l_result.bakeInputs, l_list.c_str(), sizeof(l_result.bakeInputs) - 1);
			l_result.isBakeMode = true;
			l_result.isHeadless = true;
			Log(Success, "Bake mode: will import '", l_result.bakeInputs, "' then exit.");
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
	if (m_pImpl->m_initConfig.totalFrames > 0)
		Get<LogService>()->SetFatalOnError(true);
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

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		Get<EditorService>();
	}

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

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		SystemSetup(EditorService);
	}

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

				if (m_pImpl->m_LogicClient)
					m_pImpl->m_LogicClient->Update();

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
				if (m_pImpl->m_RenderingClient)
					m_pImpl->m_RenderingClient->Update();

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

		// RenderDoc / PIX capture trigger. Lives here rather than inside
		// FrameManagementService::Update because the frame manager owns frame
		// pacing, not debug-tool triggers. -capture_frame N fires a single
		// frame capture at frame N (0-indexed, FrameCountSinceLaunch).
		const int l_captureFrame = m_pImpl->m_initConfig.captureFrame;
		if (l_captureFrame >= 0)
		{
			Get<FrameManagementService>()->SetPreFrameCallback([this, l_captureFrame](uint32_t frameCount)
				{
					if (frameCount == static_cast<uint32_t>(l_captureFrame))
						Get<GraphicsHardwareService>()->BeginCapture();
				});

			Get<FrameManagementService>()->SetPostFrameCallback([this, l_captureFrame](uint32_t frameCount)
				{
					if (frameCount != static_cast<uint32_t>(l_captureFrame))
						return;

					// Drain all queued GPU work so the capture boundary encloses
					// a complete frame — RenderDoc's EndFrameCapture otherwise
					// sees a mid-flight state and records no useful frame.
					auto* l_fm = Get<FrameManagementService>();
					auto* l_hw = Get<GraphicsHardwareService>();
					l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
					l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Compute),  GPUEngineType::Compute);
					l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Copy),     GPUEngineType::Copy);
					(void)l_fm;
					l_hw->EndCapture();
				});
		}
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
		auto l_ExampleRenderingClientSetupTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
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

		l_ExampleRenderingClientSetupTask->Activate();
		l_ExampleRenderingClientSetupTask->Wait();
	}

	// Only setup LogicClient if it exists and we're not in bake mode.
	// Bake is a one-shot asset-import-then-exit flow; LogicClient loads
	// scenes / spawns players / runs physics — none of which the import
	// pipeline touches, and scene loading would call WaitForGPUIdle on a
	// FrameManagementService that has no GraphicsHardwareService wired up
	// (headless). Skip the whole subsystem.
	if (m_pImpl->m_LogicClient && !m_pImpl->m_initConfig.isBakeMode) {
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

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		SystemInit(EditorService);
	}

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
		auto l_ExampleRenderingClientInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
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

		l_ExampleRenderingClientInitializationTask->Activate();
		l_ExampleRenderingClientInitializationTask->Wait();

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

	// Only initialize LogicClient if it exists and we're not in bake mode
	// (see Setup for rationale — bake skips the game layer entirely).
	if (m_pImpl->m_LogicClient && !m_pImpl->m_initConfig.isBakeMode) {
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

	// Drain the GPU before destroying resources — the last rendered frame's
	// commands may still be in-flight since no subsequent BeginFrame waited.
	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<FrameManagementService>()->WaitForGPUIdle();
	}

	// Phase 1 of shutdown — GPU-alive finalization. Anything that needs a
	// working GPU (readback, final flush, capture save) runs here, before
	// LogicClient::Terminate starts any long-running CPU work. This makes
	// the "GPU alive during readback" invariant structural instead of
	// depending on where a line happens to sit in Terminate (see TASK-42).
	if (!m_pImpl->m_initConfig.isHeadless && m_pImpl->m_RenderingClient) {
		if (!m_pImpl->m_RenderingClient->FinalizeGPUResults())
			Log(Warning, "RenderingClient::FinalizeGPUResults reported failure; continuing shutdown.");
	}

	// Phase 2 — LogicClient CPU-heavy shutdown (CPU path tracer, physics
	// teardown, etc.). GPU may become unresponsive mid-way (TDR) during
	// this phase; nothing here may touch GPU resources. Skipped in bake
	// mode (client was never Setup/Initialize'd).
	if (m_pImpl->m_LogicClient && !m_pImpl->m_initConfig.isBakeMode) {
		if (!m_pImpl->m_LogicClient->Terminate())
		{
			Log(Error, "Logic client can't be terminated!");
			return false;
		}
	}

	// Only terminate rendering-related services if not headless
	if (!m_pImpl->m_initConfig.isHeadless) {
		ITask::Desc taskDesc("Default Rendering Client Termination Task", ITask::Type::Once, 2);
		auto l_ExampleRenderingClientTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemTerm(GUIService);

			if (m_pImpl->m_RenderingClient && !m_pImpl->m_RenderingClient->Terminate())
			{
				Log(Error, "Rendering client can't be terminated!");
				return false;
			}
			return true;
			});
		l_ExampleRenderingClientTerminationTask->Activate();
		l_ExampleRenderingClientTerminationTask->Wait();

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

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		SystemTerm(EditorService);
	}

	SystemTerm(PhysicsSimulationService);
	SystemTerm(SceneService);
	SystemTerm(AssetService);
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
	// Bake mode (TASK-68): headless, one-shot import-then-exit. The normal
	// Run loop depends on WindowSystem being Activated; in bake mode we use
	// the HeadlessWindowService and never enter frame pacing.
	//
	// Per-file parallelism (TASK-70 axis 1): each path runs on its own
	// std::thread. Inside ImportSync, ProcessAssimpScene fans out mesh and
	// material work to TaskScheduler workers (axis 2). If the outer file loop
	// also ran on scheduler workers, a worker's outer task would call
	// Thread::AddTask on itself — Thread::AddTask requires the target thread
	// to leave Busy, which can't happen while the outer task blocks on its
	// own sub-tasks. std::thread keeps the orchestrator off the worker pool
	// so sub-task submissions make forward progress. AssimpWrapper::Import is
	// thread-safe: each call constructs its own local Assimp::Importer, writes
	// to unique filenames, and goes through the per-type shared_mutex-guarded
	// AssetService registries.
	if (m_pImpl->m_initConfig.isBakeMode)
	{
		const std::string l_list(m_pImpl->m_initConfig.bakeInputs);
		auto* l_assetService = Get<AssetService>();

		struct BakeTask
		{
			std::string path;
			bool ok{false};
			int64_t elapsedMs{0};
		};
		std::vector<BakeTask> l_tasks;

		size_t l_pos = 0;
		while (l_pos < l_list.size())
		{
			size_t l_sep = l_list.find(';', l_pos);
			if (l_sep == std::string::npos) l_sep = l_list.size();
			std::string l_path = l_list.substr(l_pos, l_sep - l_pos);
			l_pos = l_sep + 1;
			if (l_path.empty()) continue;
			l_tasks.push_back({ std::move(l_path), false, 0 });
		}

		const auto l_batchStart = std::chrono::steady_clock::now();

		std::vector<std::thread> l_threads;
		l_threads.reserve(l_tasks.size());
		for (auto& l_entry : l_tasks)
		{
			l_threads.emplace_back([l_assetService, &l_entry]()
			{
				const auto l_fileStart = std::chrono::steady_clock::now();
				const bool l_result = l_assetService->ImportSync(l_entry.path.c_str());
				l_entry.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - l_fileStart).count();
				l_entry.ok = l_result;
			});
		}
		for (auto& l_thread : l_threads)
			l_thread.join();

		uint32_t l_ok = 0, l_fail = 0;
		for (const auto& l_entry : l_tasks)
		{
			if (l_entry.ok)
			{
				Log(Success, "Bake: ", l_entry.path.c_str(), " imported in ", static_cast<uint64_t>(l_entry.elapsedMs), " ms.");
				++l_ok;
			}
			else
			{
				Log(Error, "Bake: ", l_entry.path.c_str(), " FAILED after ", static_cast<uint64_t>(l_entry.elapsedMs), " ms.");
				++l_fail;
			}
		}
		const auto l_totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - l_batchStart).count();
		Log(Success, "Bake complete: ", l_ok, " ok, ", l_fail, " failed, wall-clock ",
			static_cast<uint64_t>(l_totalMs), " ms across ", static_cast<uint32_t>(l_tasks.size()), " files.");
		return l_fail == 0;
	}

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
