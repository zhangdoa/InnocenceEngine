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
#include "../../Services/TemplateAssetService.h"
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

	m_SwapChainRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SwapChain/");
	m_SwapChainShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SwapChain/");
	m_SwapChainSamplerComp = g_Engine->Get<SamplerResourceService>()->Add("SwapChain/");

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

	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SwapChainShaderProgramComp->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

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

	auto l_captureFrame = g_Engine->getInitConfig().captureFrame;
	bool l_isCapturing = (l_captureFrame >= 0 && m_FrameCountSinceLaunch == static_cast<uint32_t>(l_captureFrame));
	if (l_isCapturing)
		m_HardwareService->BeginCapture();

	m_HardwareService->WaitOnCPU(m_GraphicsSemaphoreValues[l_currentFrame], GPUEngineType::Graphics);
	m_HardwareService->WaitOnCPU(m_ComputeSemaphoreValues[l_currentFrame], GPUEngineType::Compute);
	m_HardwareService->WaitOnCPU(m_CopySemaphoreValues[l_currentFrame], GPUEngineType::Copy);

	BeginFrame();

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

		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
		m_HardwareService->WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Compute);

		ExecuteSwapChainCommands();

		if (!g_Engine->getInitConfig().isOffscreen)
		{
			m_HardwareService->SignalOnGPU(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
		}

		g_Engine->Get<GUIService>()->ExecuteCommands();
	}

	m_GraphicsSemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics);
	m_ComputeSemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute);
	m_CopySemaphoreValues[l_currentFrame] = m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy);

	Present();

	if (l_isCapturing)
		m_HardwareService->EndCapture();

	EndFrame();

	m_FrameCountSinceLaunch++;

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

uint32_t FrameManagementService::GetCurrentFrame()
{
	return m_CurrentFrame;
}

uint32_t FrameManagementService::GetPreviousFrame()
{
	return m_CurrentFrame == 0 ? m_swapChainImageCount - 1 : m_CurrentFrame - 1;
}

uint32_t FrameManagementService::GetNextFrame()
{
	return m_CurrentFrame == m_swapChainImageCount - 1 ? 0 : m_CurrentFrame + 1;
}

uint32_t FrameManagementService::GetSwapChainImageCount()
{
	return m_swapChainImageCount;
}

uint32_t FrameManagementService::GetFrameCountSinceLaunch()
{
	return m_FrameCountSinceLaunch;
}

void FrameManagementService::SetUploadHeapPreparationCallback(std::function<bool()>&& callback)
{
	m_UploadHeapPreparationCallback = callback;
}

void FrameManagementService::SetCommandPreparationCallback(std::function<bool()>&& callback)
{
	m_CommandPreparationCallback = callback;
}

void FrameManagementService::SetCommandExecutionCallback(std::function<bool()>&& callback)
{
	m_CommandExecutionCallback = callback;
}

RenderPassComponent* FrameManagementService::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

bool FrameManagementService::SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;
	return true;
}

GPUResourceComponent* FrameManagementService::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

ISemaphore* FrameManagementService::GetGlobalSemaphore()
{
	return m_GlobalSemaphore;
}

bool FrameManagementService::Present()
{
	PresentImpl();

	if (m_needResize)
	{
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

		auto l_graphicsSemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute);
		auto l_copySemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy);

		m_HardwareService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
		m_HardwareService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		m_HardwareService->WaitOnCPU(l_copySemaphoreValue, GPUEngineType::Copy);

		ExecuteResize();

		m_needResize = false;
	}

	return true;
}

bool FrameManagementService::WaitForGPUIdle()
{
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);
	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy), GPUEngineType::Copy);

	return true;
}

bool FrameManagementService::Resize()
{
	m_needResize = true;
	return true;
}

bool FrameManagementService::PrepareGlobalCommands()
{
	auto l_currentFrame = m_CurrentFrame;

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Open(l_commandList, GPUEngineType::Graphics);

	auto l_gpuBufferService = g_Engine->Get<GPUBufferResourceService>();
	l_gpuBufferService->ForEach([&](GPUBufferComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (i->m_MappedMemories.size() == 0)
			return;

		auto l_mappedMemory = i->m_MappedMemories[l_currentFrame];
		if (l_mappedMemory->m_NeedUploadToGPU)
		{
			TryToTransitState(i, l_commandList, Accessibility::ReadOnly, Accessibility::CopyDestination);
			l_gpuBufferService->UploadToGPU(l_commandList, i);
			TryToTransitState(i, l_commandList, Accessibility::CopyDestination, Accessibility::ReadOnly);
			l_mappedMemory->m_NeedUploadToGPU = false;
		}
	});

	l_gpuBufferService->UpdateRaytracingInstances();

	PrepareRayTracing(l_commandList);

	Close(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool FrameManagementService::ExecuteGlobalCommands()
{
	auto l_currentFrame = m_CurrentFrame;

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	m_HardwareService->Execute(l_commandList, GPUEngineType::Graphics);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);

	return true;
}

bool FrameManagementService::PrepareSwapChainCommands()
{
	if (g_Engine->getInitConfig().isOffscreen)
		return true;

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_currentFrame = m_CurrentFrame;
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	auto l_swapChainRP = m_SwapChainRenderPassComp;

	CommandListBegin(l_swapChainRP, l_commandList, l_currentFrame);

	TryToTransitState(reinterpret_cast<TextureComponent*>(l_userPipelineOutput), l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);
	BindRenderPassComponent(l_swapChainRP, l_commandList);

	ClearRenderTargets(l_swapChainRP, l_commandList);

	BindGPUResource(l_swapChainRP, l_commandList, ShaderStage::Pixel, l_userPipelineOutput, 0);
	BindGPUResource(l_swapChainRP, l_commandList, ShaderStage::Pixel, m_SwapChainSamplerComp, 1);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Square);

	DrawIndexedInstanced(l_swapChainRP, l_commandList, l_mesh, 1);

	TryToTransitState(l_swapChainRP->m_OutputMergerTarget->m_ColorOutputs[0], l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);

	CommandListEnd(l_swapChainRP, l_commandList);

	return true;
}

bool FrameManagementService::ExecuteSwapChainCommands()
{
	if (g_Engine->getInitConfig().isOffscreen)
		return true;

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_currentFrame = m_CurrentFrame;
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	m_HardwareService->Execute(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool FrameManagementService::ExecuteResize()
{
	PreResize();
	ResizeImpl();
	PostResize();

	return true;
}

bool FrameManagementService::PreResize()
{
	bool l_result = true;

	g_Engine->Get<RenderPassResourceService>()->ForEach([&](RenderPassComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			l_result = false;
		}
	});

	return l_result;
}

bool FrameManagementService::PreResize(RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	g_Engine->Get<RenderPassResourceService>()->DeleteRenderTargets(renderPass);

	return true;
}

bool FrameManagementService::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	bool l_result = true;

	g_Engine->Get<RenderPassResourceService>()->ForEach([&](RenderPassComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			l_result = false;
		}
	});

	return l_result;
}

bool FrameManagementService::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	auto l_rpService = g_Engine->Get<RenderPassResourceService>();
	l_rpService->CreateOutputMergerTargets(renderPass);
	l_rpService->InitializeOutputMergerTargets(renderPass);
	l_rpService->OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = l_rpService->AddPipelineStateObject();

	l_rpService->CreatePipelineStateObject(renderPass);

	if (renderPass->m_OnResize)
		renderPass->m_OnResize();

	return true;
}
