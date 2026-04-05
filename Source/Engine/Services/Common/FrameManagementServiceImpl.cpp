#include "../FrameManagementService.h"
#include "../GraphicsResourceService.h"
#include "../GraphicsHardwareService.h"

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
		auto l_commandList = m_ResourceService->AddCommandListComponent(("GlobalGraphicsCommandList_" + std::to_string(i)).c_str());
		m_ResourceService->Initialize(l_commandList);
		m_GlobalGraphicsCommandLists[i] = l_commandList;
	}

	Log(Success, "Global Graphics CommandLists have been created.");

	m_SwapChainRenderPassComp = m_ResourceService->AddRenderPassComponent("SwapChain/");
	m_SwapChainShaderProgramComp = m_ResourceService->AddShaderProgramComponent("SwapChain/");
	m_SwapChainSamplerComp = m_ResourceService->AddSamplerComponent("SwapChain/");

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

	m_ResourceService->Initialize(m_SwapChainShaderProgramComp);
	m_ResourceService->Initialize(m_SwapChainSamplerComp);

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

	m_ResourceService->Initialize(l_swapChainRP);

	return true;
}

bool FrameManagementService::Update()
{
	auto l_currentFrame = m_CurrentFrame;

	m_HardwareService->WaitOnCPU(m_GraphicsSemaphoreValues[l_currentFrame], GPUEngineType::Graphics);
	m_HardwareService->WaitOnCPU(m_ComputeSemaphoreValues[l_currentFrame], GPUEngineType::Compute);
	m_HardwareService->WaitOnCPU(m_CopySemaphoreValues[l_currentFrame], GPUEngineType::Copy);

	BeginFrame();

	m_ResourceService->InitializeComponents();

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

	EndFrame();

	m_FrameCountSinceLaunch++;

	return true;
}

bool FrameManagementService::Terminate()
{
	auto l_result = true;
	l_result &= m_ResourceService->Delete(m_SwapChainSamplerComp);
	l_result &= m_ResourceService->Delete(m_SwapChainShaderProgramComp);
	l_result &= m_ResourceService->Delete(m_SwapChainRenderPassComp);

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

	for (auto i : m_ResourceService->GetGPUBufferPointers())
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (i->m_MappedMemories.size() == 0)
			continue;

		auto l_mappedMemory = i->m_MappedMemories[l_currentFrame];
		if (l_mappedMemory->m_NeedUploadToGPU)
		{
			TryToTransitState(i, l_commandList, Accessibility::ReadOnly, Accessibility::CopyDestination);
			m_ResourceService->UploadToGPU(l_commandList, i);
			TryToTransitState(i, l_commandList, Accessibility::CopyDestination, Accessibility::ReadOnly);
			l_mappedMemory->m_NeedUploadToGPU = false;
		}
	}

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
	for (auto i : m_ResourceService->GetRenderPassPointers())
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			return false;
		}
	}

	return true;
}

bool FrameManagementService::PreResize(RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	m_ResourceService->DeleteRenderTargets(renderPass);

	return true;
}

bool FrameManagementService::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	for (auto i : m_ResourceService->GetRenderPassPointers())
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			return false;
		}
	}

	return true;
}

bool FrameManagementService::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	m_ResourceService->CreateOutputMergerTargets(renderPass);
	m_ResourceService->InitializeOutputMergerTargets(renderPass);

	m_ResourceService->OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = m_ResourceService->AddPipelineStateObject();

	m_ResourceService->CreatePipelineStateObject(renderPass);

	if (renderPass->m_OnResize)
		renderPass->m_OnResize();

	return true;
}
