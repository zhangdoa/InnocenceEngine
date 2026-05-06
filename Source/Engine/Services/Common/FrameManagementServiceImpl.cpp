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

	// m_GlobalSemaphore is created by the DX12 backend during CreateHardwareResources
	// (CreateSyncPrimitives sets up fence events on it), so we don't create a new one here.

	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "FrameManagementService Setup finished.");
	return true;
}

bool FrameManagementService::Initialize()
{
	if (m_ObjectStatus != ObjectStatus::Created)
	{
		Log(Error, "FrameManagementService is not in Created state.");
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
	if (g_Engine->getInitConfig().isOffscreen)
	{
		Log(Verbose, "InitializeSwapChainRenderPassComponent: Skipping in offscreen mode");
		return true;
	}

	if (!GetSwapChainImages())
	{
		Log(Error, "Failed to get swap chain images.");
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
	auto l_currentFrame = m_CurrentFrame;

	// Pre-frame hook — clients use this for debug/capture/instrumentation
	// triggers that need to straddle the GPU frame boundary. FrameManagement
	// owns frame pacing; it does NOT own RenderDoc or auto-test logic.
	if (m_PreFrameCallback)
		m_PreFrameCallback(m_FrameCountSinceLaunch);

	// BeginFrame waits for the per-queue fences of this frame slot before resetting
	// allocators, so HasGPUError below sees a GPU that has caught up to prior work.
	BeginFrame();

	if (m_HardwareService->HasGPUError())
	{
		if (!m_DeviceErrorReported)
		{
			m_DeviceErrorReported = true;
			m_HardwareService->DumpGPUDiagnostics();
			Log(Warning, "GPU device removed detected after frame wait — frame=", m_FrameCountSinceLaunch,
				" swapIndex=", l_currentFrame, " — skipping GPU work from this point forward.");
		}
		// Still run CPU-side callbacks so the logic client can count frames and trigger auto-termination
		g_Engine->Get<SceneService>()->ClearLoadingFlag();
		m_UploadHeapPreparationCallback();
		m_FrameCountSinceLaunch++;
		return false;
	}

	if (g_Engine->getInitConfig().engineMode == EngineMode::Sidecar)
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

		// Resolve this frame's GPU timer queries into the per-frame readback
		// buffer (TASK-140). Runs AFTER the per-pass command lists have been
		// submitted so the resolve sees the queries in-flight; the readback
		// of an older frame from this same call is safe because BeginFrame's
		// WaitOnCPU has already drained that older slot.
		m_HardwareService->ResolveGpuTimers();

		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Compute);

		ExecuteSwapChainCommands();

		if (g_Engine->getInitConfig().isOffscreen)
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

	// Post-frame hook — clients use this to finalize capture / readback /
	// any work that needs to happen AFTER Present but within the same
	// logical frame. Callback receives the frame index just completed.
	if (m_PostFrameCallback)
		m_PostFrameCallback(m_FrameCountSinceLaunch);

	EndFrame();

	m_FrameCountSinceLaunch++;

	// TASK-213 CL A: evaluate the readiness predicate every frame so the
	// K=3 stability window advances and the first-true log marker fires.
	// Result is intentionally discarded this CL — CL B/C/D consume it.
	(void)IsSteadyState();

	return true;
}

bool FrameManagementService::Terminate()
{
	auto l_result = true;
	l_result &= g_Engine->Get<SamplerResourceService>()->Delete(m_SwapChainSamplerComp);
	l_result &= g_Engine->Get<ShaderProgramResourceService>()->Delete(m_SwapChainShaderProgramComp);
	l_result &= g_Engine->Get<RenderPassResourceService>()->Delete(m_SwapChainRenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	if (l_result)
		Log(Success, "FrameManagementService has been terminated.");
	else
		Log(Error, "Failed to terminate FrameManagementService.");

	return l_result;
}
