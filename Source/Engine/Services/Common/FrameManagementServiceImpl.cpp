#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../CommandListResourceService.h"
#include "../SamplerResourceService.h"
#include "../ShaderProgramResourceService.h"
#include "../TextureResourceService.h"
#include "../GPUBufferResourceService.h"
#include "../MeshResourceService.h"
#include "../MaterialResourceService.h"
#include "../RenderPassResourceService.h"
#include "../SceneService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/SceneService.h"
#include "../../Services/GUIService.h"
#include "../ConfigurationService.h"

#include "../../Engine.h"

using namespace Inno;

bool FrameManagementService::Setup(IServiceConfig* systemConfig)
{
	m_swapChainImageCount = 3;

	if (!CreateSwapChainResources())
	{
		Log(Error, "FrameManagementService: CreateSwapChainResources() failed.");
		return false;
	}

	m_GlobalGraphicsCommandLists.resize(m_swapChainImageCount);
	for (size_t i = 0; i < m_GlobalGraphicsCommandLists.size(); i++)
	{
		auto l_commandList = g_Engine->Get<CommandListResourceService>()->Add(("GlobalGraphicsCommandList_" + std::to_string(i)).c_str());
		g_Engine->Get<CommandListResourceService>()->Initialize(l_commandList);
		m_GlobalGraphicsCommandLists[i] = l_commandList;
	}

	Log(Success, "Global Graphics CommandLists have been created.");

	m_SwapChainRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SwapChain");
	m_SwapChainShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SwapChain");
	m_SwapChainSamplerComp = g_Engine->Get<SamplerResourceService>()->Add("SwapChain");

	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "FrameManagementService Setup finished.");
	return true;
}

bool FrameManagementService::Initialize()
{
	if (m_ObjectStatus != ObjectStatus::Created)
	{
		Log(Error, " is not in Created state.");
		return false;
	}

	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert";
	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_PSPath = "swapChain.frag";

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_SwapChainShaderProgramComp);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SwapChainSamplerComp);

	InitializeSwapChainRenderPassComponent();

	m_GraphicsSemaphoreValues.resize(m_swapChainImageCount, 0);
	m_ComputeSemaphoreValues.resize(m_swapChainImageCount, 0);
	m_CopySemaphoreValues.resize(m_swapChainImageCount, 0);

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "FrameManagementService has been initialized.");

	return true;
}

bool FrameManagementService::InitializeSwapChainRenderPassComponent()
{
	if (g_Engine->Get<ConfigurationService>()->IsOffscreen())
	{
		Log(Verbose, " offscreen mode; skipping.");
		return true;
	}

	if (!GetSwapChainImages())
	{
		Log(Error, " GetSwapChainImages failed; aborting.");
		return false;
	}

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&FrameManagementService::AssignSwapChainImages, this);
	l_RenderPassDesc.m_RenderTargetsRemovalFunc = std::bind(&FrameManagementService::ReleaseSwapChainImages, this);

	auto l_swapChainRP = m_SwapChainRenderPassComp;
	l_swapChainRP->m_RenderPassDesc = l_RenderPassDesc;
	l_swapChainRP->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	l_swapChainRP->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_swapChainRP->m_ResourceBindingLayoutDescs.resize(2);

	l_swapChainRP->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	l_swapChainRP->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	l_swapChainRP->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	l_swapChainRP->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;

	l_swapChainRP->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
	l_swapChainRP->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	l_swapChainRP->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;

	l_swapChainRP->m_ShaderProgram = m_SwapChainShaderProgramComp;

	g_Engine->Get<RenderPassResourceService>()->Initialize(l_swapChainRP);

	return true;
}

bool FrameManagementService::Update()
{
	// A requested shutdown (budget reached, window closed, device lost) stops
	// new GPU work so the main loop can tear down without a racing frame.
	if (g_Engine->IsShutdownRequested())
		return false;

	auto l_currentFrame = m_CurrentFrame;

	if (m_PreFrameCallback)
		m_PreFrameCallback(m_FrameCountSinceLaunch);

	// BeginFrame waits for this slot's per-queue fences before resetting
	// allocators, so HasGPUError below observes a GPU caught up to prior work.
	BeginFrame();

	if (m_HardwareService->HasGPUError())
	{
		if (!m_DeviceErrorReported)
		{
			m_DeviceErrorReported = true;
			m_HardwareService->DumpGPUDiagnostics();
			Log(Warning, " GPU device removed after frame wait; frame=", m_FrameCountSinceLaunch, " swapIndex=", l_currentFrame, " — skipping GPU work from this point forward.");
		}
		// Device removed: drain CPU-side callbacks once, then request a clean
		// shutdown — a lost device can't recover, so honour the terminate
		// contract immediately instead of spinning a dead render loop.
		g_Engine->Get<SceneService>()->ClearLoadingFlag();
		m_UploadHeapPreparationCallback();
		m_FrameCountSinceLaunch++;
		g_Engine->RequestShutdown();
		return false;
	}

	if (g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Sidecar)
	{
		AssignSwapChainImages();
	}

	g_Engine->Get<MeshResourceService>()->InitializeComponents();
	g_Engine->Get<TextureResourceService>()->InitializeComponents();
	g_Engine->Get<MaterialResourceService>()->InitializeComponents();
	g_Engine->Get<GPUBufferResourceService>()->InitializeComponents();
	g_Engine->Get<RenderPassResourceService>()->InitializeComponents();
	g_Engine->Get<SceneService>()->ClearLoadingFlag();
	m_UploadHeapPreparationCallback();
	PrepareGlobalCommands();
	ExecuteGlobalCommands();

	if (!g_Engine->Get<SceneService>()->IsLoading())
	{
		m_CommandPreparationCallback();

		PrepareSwapChainCommands();
		g_Engine->Get<GUIService>()->Update();

		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Compute, GPUEngineType::Graphics);

		m_CommandExecutionCallback();

		// Must run after per-pass command lists are submitted (so the resolve
		// sees the queries in flight) and after BeginFrame drained the older
		// slot (so the older frame's readback this call touches is safe).
		m_HardwareService->ResolveGpuTimers();

		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Compute);

		ExecuteSwapChainCommands();

		if (g_Engine->Get<ConfigurationService>()->IsOffscreen())
		{
			m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
			m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
		}
		else
		{
			m_HardwareService->SignalOnGPU(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
		}

		g_Engine->Get<GUIService>()->ExecuteCommands();
	}

	m_GraphicsSemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics);
	m_ComputeSemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute);
	m_CopySemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy);

	Present();

	if (m_PostFrameCallback)
		m_PostFrameCallback(m_FrameCountSinceLaunch);

	EndFrame();

	m_FrameCountSinceLaunch++;

	// Drive the steady-state window every frame; the boolean is consumed by
	// IsSteadyState() callers and the first-true log marker fires from inside.
	(void)IsSteadyState();

	// Frame-lifecycle authority: terminate once the totalFrames budget is spent.
	EvaluateFrameBudget();

	return true;
}

bool FrameManagementService::Terminate()
{
	Log(Verbose, " starting.");
	if (!g_Engine->Get<SamplerResourceService>()->Delete(m_SwapChainSamplerComp))
	{
		Log(Error, " SamplerResourceService->Delete failed; aborting.");
		return false;
	}
	if (!g_Engine->Get<ShaderProgramResourceService>()->Delete(m_SwapChainShaderProgramComp))
	{
		Log(Error, " ShaderProgramResourceService->Delete failed; aborting.");
		return false;
	}
	if (!g_Engine->Get<RenderPassResourceService>()->Delete(m_SwapChainRenderPassComp))
	{
		Log(Error, " RenderPassResourceService->Delete failed; aborting.");
		return false;
	}

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, " terminated.");
	return true;
}
