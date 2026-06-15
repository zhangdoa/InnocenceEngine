#include "Engine_Internal.h"
#include "Common/Timer.h"
#include "Common/LogService.h"
#include "Common/Memory.h"
#include "Common/TaskScheduler.h"
#include "Common/IOService.h"
#include "Services/ConfigurationService.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
#include "Services/LightSimulationService.h"
#include "Services/CameraService.h"
#include "Services/SceneService.h"
#include "Services/AssetService.h"
#include "Services/PhysicsSimulationService.h"
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

#if defined INNO_PLATFORM_WIN
#include "Platform/WinWindow/WinWindowService.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "Platform/MacWindow/MacWindowService.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "Platform/LinuxWindow/LinuxWindowService.h"
#endif

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

#include "Platform/HeadlessWindow/HeadlessWindowService.h"

using namespace Inno;

bool Engine::CreateServices(void* appHook, void* extraHook, char* pScmdline)
{
	auto* l_cfg = g_Engine->Get<ConfigurationService>();

	Get<Timer>();
	Get<LogService>();
	Get<Memory>();
	Get<TaskScheduler>();
	Get<IOService>()->SetupWorkingDirectory();

	// Config load resolves paths against the data directory, so it must run
	// after SetupWorkingDirectory.
	if (pScmdline)
		l_cfg->LoadFromCommandLine(pScmdline);
	if (l_cfg->GetTotalFrames() > 0)
		Get<LogService>()->SetFatalOnError(true);

	Get<HIDService>();

	if (l_cfg->IsHeadless() || l_cfg->IsOffscreen()) {
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

	if (!l_cfg->IsHeadless()) {
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
		if (!l_cfg->IsOffscreen())
			Get<GUIService>();
	}

#if defined INNO_RENDERER_DIRECTX
	if (!l_cfg->IsHeadless())
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

#if defined INNO_PLATFORM_MAC
	if (!l_cfg->IsHeadless()) {
		auto l_windowSystem = reinterpret_cast<MacWindowService*>(m_pImpl->m_WindowSystem.get());
		auto l_windowSystemBridge = reinterpret_cast<MacWindowServiceBridge*>(appHook);
		l_windowSystem->setBridge(l_windowSystemBridge);
	}
#endif

	Get<EntityRegistry>();
	Get<AssetService>();
	Get<SceneService>();
	Get<PhysicsSimulationService>();
	Get<LightSimulationService>();
	Get<CameraService>();

	if (l_cfg->GetEngineMode() == EngineMode::Sidecar)
	{
		Get<EditorService>();
	}

	return true;
}
