#include "../IRenderingServer.h"

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/TaskScheduler.h"
#include "../../Common/ThreadSafeQueue.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

#include "../../Engine.h"
using namespace Inno;

// @TODO: These should be moved to a separate file.
// Static member definitions
Accessibility Accessibility::Immutable = Accessibility(false, false);
Accessibility Accessibility::ReadOnly = Accessibility(true, false);
Accessibility Accessibility::WriteOnly = Accessibility(false, true);
Accessibility Accessibility::ReadWrite = Accessibility(true, true);

bool IRenderingServer::Setup(ISystemConfig* systemConfig)
{
	bool l_result = InitializePool();
	if (!l_result)
	{
		Log(Error, "Failed to initialize pool.");
		return false;
	}

	l_result &= CreateHardwareResources();
	if (!l_result)
	{
		Log(Error, "Failed to create hardware resources.");
		return false;
	}

	m_SwapChainRenderPassComp = AddRenderPassComponent("SwapChain/");
	m_SwapChainSPC = AddShaderProgramComponent("SwapChain/");
	m_SwapChainSamplerComp = AddSamplerComponent("SwapChain/");

	m_ObjectStatus = ObjectStatus::Created;

	Log(Success, "RenderingServer Setup finished.");
	return true;
}

bool IRenderingServer::Initialize()
{
	if (m_ObjectStatus != ObjectStatus::Created)
	{
		Log(Error, "RenderingServer is not in Created state.");
		return false;
	}

	m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

	InitializeShaderProgramComponent(m_SwapChainSPC);
	InitializeSamplerComponent(m_SwapChainSamplerComp);

	if (!GetSwapChainImages())
	{
		Log(Error, "Failed to get swap chain images.");
		return false;
	}

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = m_swapChainImageCount;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&IRenderingServer::AssignSwapChainImages, this);

	m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_UseMultiFrames = true;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	// t0 - 2D texture (single image from the user pipeline)
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;

	// s0 - sampler
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;

	m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainSPC;

	InitializeRenderPassComponent(m_SwapChainRenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "RenderingServer has been initialized.");

	return true;
}

bool IRenderingServer::Update()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshComponent* l_Mesh;
		m_uninitializedMeshes.tryPop(l_Mesh);

		if (!l_Mesh)
			continue;

		InitializeImpl(l_Mesh);
		if (l_Mesh->m_ObjectStatus == ObjectStatus::Activated)
			m_initializedMeshes.emplace(l_Mesh);
	}

	while (m_uninitializedTextures.size() > 0)
	{
		TextureComponent* l_Texture;
		m_uninitializedTextures.tryPop(l_Texture);

		if (!l_Texture)
			continue;

		InitializeImpl(l_Texture);
		if (l_Texture->m_ObjectStatus == ObjectStatus::Activated)
			m_initializedTextures.emplace(l_Texture);
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialComponent* l_Material;
		m_uninitializedMaterials.tryPop(l_Material);

		if (!l_Material)
			continue;

		InitializeImpl(l_Material);
		if (l_Material->m_ObjectStatus == ObjectStatus::Activated)
		{
			for (size_t i = 0; i < MaxTextureSlotCount; i++)
			{
				auto l_texture = l_Material->m_TextureSlots[i].m_Texture;
				if (l_texture && m_initializedTextures.find(l_texture) == m_initializedTextures.end())
					m_initializedTextures.emplace(l_texture);
			}
			m_initializedMaterials.emplace(l_Material);
		}
	}

	while (m_uninitializedGPUBuffers.size() > 0)
	{
		GPUBufferComponent* l_GPUBuffer;
		m_uninitializedGPUBuffers.tryPop(l_GPUBuffer);

		if (!l_GPUBuffer)
			continue;

		InitializeImpl(l_GPUBuffer);
		if (l_GPUBuffer->m_ObjectStatus == ObjectStatus::Activated)
			m_initializedGPUBuffers.emplace(l_GPUBuffer);
	}

	while (m_uninitializedRenderPasses.size() > 0)
	{
		RenderPassComponent* l_RenderPass;
		m_uninitializedRenderPasses.tryPop(l_RenderPass);

		if (!l_RenderPass)
			continue;

		InitializeImpl(l_RenderPass);
		if (l_RenderPass->m_ObjectStatus == ObjectStatus::Activated)
			m_initializedRenderPasses.push_back(l_RenderPass);
	}

	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalCommandLists[l_currentFrame];
	Open(l_commandList, GPUEngineType::Graphics);

	for (auto i : m_initializedMeshes)
	{
		if (i->m_NeedUploadToGPU)
		{
			UploadToGPU(l_commandList, i);
			i->m_NeedUploadToGPU = false;
		}
	}

	for (auto i : m_initializedTextures)
	{
		if (i->m_NeedUploadToGPU)
		{
			UploadToGPU(l_commandList, i);
			i->m_NeedUploadToGPU = false;
		}
	}

	for (auto i : m_initializedGPUBuffers)
	{
		if (i->m_NeedUploadToGPU)
		{
			UploadToGPU(l_commandList, i);
			i->m_NeedUploadToGPU = false;
		}
	}

	Close(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool IRenderingServer::Terminate()
{
	auto l_result = true;
	l_result &= DeleteSamplerComponent(m_SwapChainSamplerComp);
	l_result &= DeleteShaderProgramComponent(m_SwapChainSPC);
	l_result &= DeleteRenderPassComponent(m_SwapChainRenderPassComp);

	l_result &= ReleaseHardwareResources();
	l_result &= TerminatePool();

	m_ObjectStatus = ObjectStatus::Terminated;

	if (l_result)
		Log(Success, "RenderingServer has been terminated.");
	else
		Log(Error, "Failed to terminate RenderingServer.");

	return l_result;
}

void IRenderingServer::InitializeMeshComponent(MeshComponent* rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
		return;

	m_uninitializedMeshes.push(rhs);
}

void IRenderingServer::InitializeTextureComponent(TextureComponent* rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
		return;

	m_uninitializedTextures.push(rhs);
}

void IRenderingServer::InitializeMaterialComponent(MaterialComponent* rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
		return;

	m_uninitializedMaterials.push(rhs);
}

void IRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent* rhs)
{
	InitializeImpl(rhs);
}

void IRenderingServer::InitializeSamplerComponent(SamplerComponent* rhs)
{
	InitializeImpl(rhs);
}

void IRenderingServer::InitializeGPUBufferComponent(GPUBufferComponent* rhs)
{
	if (m_initializedGPUBuffers.find(rhs) != m_initializedGPUBuffers.end())
		return;

	m_uninitializedGPUBuffers.push(rhs);
}

void IRenderingServer::InitializeRenderPassComponent(RenderPassComponent* rhs)
{
	if (std::find(m_initializedRenderPasses.begin(), m_initializedRenderPasses.end(), rhs) != m_initializedRenderPasses.end())
		return;

	m_uninitializedRenderPasses.push(rhs);
}

bool IRenderingServer::ReserveRenderTargets(RenderPassComponent* rhs)
{
	if (rhs->m_RenderPassDesc.m_RenderTargetsReservationFunc)
	{
		Log(Verbose, "Calling customized render targets reservation function for: ", rhs->m_InstanceName.c_str());
		rhs->m_RenderPassDesc.m_RenderTargetsReservationFunc();
	}
	else
	{
		rhs->m_RenderTargets.resize(rhs->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			rhs->m_RenderTargets[i].m_IsOwned = true;
			rhs->m_RenderTargets[i].m_Texture = AddTextureComponent((std::string(rhs->m_InstanceName.c_str()) + "_RT_" + std::to_string(i) + "/").c_str());
			Log(Verbose, "Render target: ", rhs->m_RenderTargets[i].m_Texture->m_InstanceName.c_str(), " has been allocated at: ", rhs->m_RenderTargets[i].m_Texture);
		}
	}

	if (rhs->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", rhs->m_InstanceName.c_str());
		rhs->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc();
	}
	else
	{
		if (rhs->m_RenderPassDesc.m_UseDepthBuffer)
		{
			rhs->m_DepthStencilRenderTarget.m_IsOwned = true;
			rhs->m_DepthStencilRenderTarget.m_Texture = AddTextureComponent((std::string(rhs->m_InstanceName.c_str()) + "_DS/").c_str());
			Log(Verbose, "", rhs->m_InstanceName.c_str(), " depth stencil target has been allocated.");
		}
	}

	return true;
}

bool IRenderingServer::CreateRenderTargets(RenderPassComponent* rhs)
{
	if (rhs->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized render targets creation function for: ", rhs->m_InstanceName.c_str());
		rhs->m_RenderPassDesc.m_RenderTargetsCreationFunc();
	}
	else
	{
		for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_TextureComp = rhs->m_RenderTargets[i].m_Texture;
			l_TextureComp->m_TextureDesc = rhs->m_RenderPassDesc.m_RenderTargetDesc;
			l_TextureComp->m_TextureData = nullptr;

			InitializeImpl(l_TextureComp);

			Log(Verbose, "Render target: ", rhs->m_RenderTargets[i].m_Texture->m_InstanceName.c_str(), " has been created.");
		}
	}

	if (rhs->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", rhs->m_InstanceName.c_str());
		rhs->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else
	{
		if (rhs->m_RenderPassDesc.m_UseDepthBuffer)
		{
			rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc = rhs->m_RenderPassDesc.m_RenderTargetDesc;

			if (rhs->m_RenderPassDesc.m_UseStencilBuffer)
			{
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
			}
			else
			{
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
			}

			rhs->m_DepthStencilRenderTarget.m_Texture->m_TextureData = nullptr;

			InitializeImpl(rhs->m_DepthStencilRenderTarget.m_Texture);

			Log(Verbose, "", rhs->m_InstanceName.c_str(), " depth stencil target has been created.");
		}
	}

	return true;
}

void IRenderingServer::ExecuteGlobalCommands()
{
	auto l_currentFrame = GetCurrentFrame();

	auto l_commandList = m_GlobalCommandLists[l_currentFrame];
	Execute(l_commandList, GPUEngineType::Graphics);
	
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);

	// Insert the wait command on the GPU side here so any later render passes don't have to wait for this.
	WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);
	WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Compute, GPUEngineType::Graphics);
}

bool IRenderingServer::CommandListBegin(RenderPassComponent* rhs, size_t frameIndex)
{
	if (rhs->m_RenderPassDesc.m_UseMultiFrames)
		rhs->m_CurrentFrame = GetCurrentFrame();
	else
		rhs->m_CurrentFrame = frameIndex;

	Open(rhs->m_CommandLists[rhs->m_CurrentFrame], rhs->m_RenderPassDesc.m_GPUEngineType, rhs->m_PipelineStateObject);

	return true;
}

bool IRenderingServer::SignalOnGPU(RenderPassComponent* rhs, GPUEngineType queueType)
{
	auto l_semaphore = rhs == nullptr ? nullptr : rhs->m_Semaphores[rhs->m_CurrentFrame];

	return SignalOnGPU(l_semaphore, queueType);
}

bool IRenderingServer::WaitOnGPU(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_semaphore = rhs == nullptr ? nullptr : rhs->m_Semaphores[rhs->m_CurrentFrame];

	return WaitOnGPU(l_semaphore, queueType, semaphoreType);
}

bool IRenderingServer::CommandListEnd(RenderPassComponent* rhs)
{
	auto l_commandList = rhs->m_CommandLists[rhs->m_CurrentFrame];
	ChangeRenderTargetStates(rhs, l_commandList, Accessibility::ReadOnly);

	Close(l_commandList, rhs->m_RenderPassDesc.m_GPUEngineType);

	return true;
}

bool IRenderingServer::ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	return Execute(rhs->m_CommandLists[rhs->m_CurrentFrame], GPUEngineType);
}

bool IRenderingServer::ChangeRenderTargetStates(RenderPassComponent* renderPass, ICommandList* commandList, Accessibility accessibility)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
	{
		TryToTransitState(renderPass->m_RenderTargets[renderPass->m_CurrentFrame].m_Texture, commandList, accessibility);
	}
	else
	{
		for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			TryToTransitState(renderPass->m_RenderTargets[i].m_Texture, commandList, accessibility);
		}
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		TryToTransitState(renderPass->m_DepthStencilRenderTarget.m_Texture, commandList, accessibility);
	}

	return true;
}

bool IRenderingServer::Present()
{
	PresentImpl();

	if (m_needResize)
	{
		auto l_graphicsSemaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = GetSemaphoreValue(GPUEngineType::Compute);
		auto l_copySemaphoreValue = GetSemaphoreValue(GPUEngineType::Copy);

		WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
		WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		WaitOnCPU(l_copySemaphoreValue, GPUEngineType::Copy);
		
		ExecuteResize();

		m_needResize = false;
	}

	return true;
}

bool IRenderingServer::SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;
	return true;
}

GPUResourceComponent* IRenderingServer::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

bool IRenderingServer::Resize()
{
	m_needResize = true;
	return true;
}

bool IRenderingServer::WriteMappedMemory(MeshComponent* rhs)
{	
	if (rhs->m_MappedMemory_VB == nullptr || rhs->m_MappedMemory_IB == nullptr)
	{
		if (rhs->m_ObjectStatus == ObjectStatus::Activated)
		{
			Log(Error, "Can't upload data to mesh: ", rhs->m_InstanceName, " because it's not mapped.");
		}
		return false;
	}

	std::memcpy((char*)rhs->m_MappedMemory_VB, &rhs->m_Vertices[0], rhs->m_Vertices.size() * sizeof(Vertex));
	std::memcpy((char*)rhs->m_MappedMemory_IB, &rhs->m_Indices[0], rhs->m_Indices.size() * sizeof(Index));

	rhs->m_NeedUploadToGPU = true;

	return true;
}

bool IRenderingServer::WriteMappedMemory(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	if (rhs->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_size = rhs->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * rhs->m_ElementSize;

	if (rhs->m_MappedMemory == nullptr)
	{
		if (rhs->m_ObjectStatus == ObjectStatus::Activated)
		{
			Log(Error, "Can't upload data to GPU buffer: ", rhs->m_InstanceName, " because it's not mapped.");
		}
		return false;
	}

	std::memcpy((char*)rhs->m_MappedMemory + startOffset * rhs->m_ElementSize, GPUBufferValue, l_size);

	rhs->m_NeedUploadToGPU = true;

	return true;
}

bool IRenderingServer::InitializeImpl(MaterialComponent* rhs)
{
	auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();

	for (size_t i = 0; i < MaxTextureSlotCount; i++)
	{
		auto l_texture = rhs->m_TextureSlots[i].m_Texture;
		if (l_texture)
			InitializeTextureComponent(l_texture);
	}

	rhs->m_GPUResourceType = GPUResourceType::Material;
	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool IRenderingServer::InitializeImpl(RenderPassComponent *rhs)
{
	bool l_result = true;

	l_result &= ReserveRenderTargets(rhs);

	l_result &= CreateRenderTargets(rhs);

	rhs->m_PipelineStateObject = AddPipelineStateObject();

	l_result &= CreatePipelineStateObject(rhs);

	if (rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		rhs->m_CommandLists.resize(rhs->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		rhs->m_CommandLists.resize(1);
	}

	for (size_t i = 0; i < rhs->m_CommandLists.size(); i++)
	{
		rhs->m_CommandLists[i] = AddCommandList();
	}

    auto l_tempName = std::string(rhs->m_InstanceName.c_str());
    auto l_tempNameL = std::wstring(l_tempName.begin(), l_tempName.end());

    for (size_t i = 0; i < rhs->m_CommandLists.size(); i++)
    {
        CreateCommandList(rhs->m_CommandLists[i], i, l_tempNameL);
    }

	Log(Verbose, rhs->m_InstanceName, " CommandList has been created.");

	rhs->m_Semaphores.resize(rhs->m_CommandLists.size());
	for (size_t i = 0; i < rhs->m_Semaphores.size(); i++)
	{
		rhs->m_Semaphores[i] = AddSemaphore();
	}
	
	Log(Verbose, rhs->m_InstanceName, " Semaphore has been created.");

	CreateFenceEvents(rhs);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

uint32_t IRenderingServer::GetSwapChainImageCount()
{
	return m_swapChainImageCount;
}

RenderPassComponent* IRenderingServer::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

uint32_t IRenderingServer::GetPreviousFrame()
{
	return m_PreviousFrame;
}

uint32_t IRenderingServer::GetCurrentFrame()
{
	return m_CurrentFrame;
}

bool IRenderingServer::PrepareSwapChainCommands()
{
	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	CommandListBegin(m_SwapChainRenderPassComp, GetCurrentFrame());

	BindRenderPassComponent(m_SwapChainRenderPassComp);

	ClearRenderTargets(m_SwapChainRenderPassComp);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, l_userPipelineOutput, 0);
	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_SwapChainSamplerComp, 1);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Square);

	DrawIndexedInstanced(m_SwapChainRenderPassComp, l_mesh, 1);

	CommandListEnd(m_SwapChainRenderPassComp);

	return true;
}

bool IRenderingServer::ExecuteSwapChainCommands()
{
	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	// We don't know on which queue the user pipeline output is, so we need to wait for all.
	WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Compute);
	WaitOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics, GPUEngineType::Graphics);

	ExecuteCommandList(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
	SignalOnGPU(m_SwapChainRenderPassComp, GPUEngineType::Graphics);

	return true;
}

bool IRenderingServer::ExecuteResize()
{
	PreResize();
	ResizeImpl();
	PostResize();

	return true;
}

bool IRenderingServer::PreResize()
{
	for (auto i : m_initializedRenderPasses)
	{
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			return false;
		}
	}

	return true;
}

bool IRenderingServer::PreResize(RenderPassComponent* rhs)
{
	if (!rhs->m_RenderPassDesc.m_Resizable)
		return true;

	for (auto i : rhs->m_RenderTargets)
	{
		if (i.m_IsOwned)
			DeleteTextureComponent(i.m_Texture);
	}

	rhs->m_RenderTargets.clear();

	if (rhs->m_DepthStencilRenderTarget.m_IsOwned && rhs->m_DepthStencilRenderTarget.m_Texture)
	{
		DeleteTextureComponent(rhs->m_DepthStencilRenderTarget.m_Texture);
	}

	Delete(rhs->m_PipelineStateObject);

	return true;
}

bool IRenderingServer::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	for (auto i : m_initializedRenderPasses)
	{
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			return false;
		}
	}

	return true;
}