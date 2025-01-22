#include "../IRenderingServer.h"

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Common/ThreadSafeQueue.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

#include "../../Engine.h"
using namespace Inno;

bool IRenderingGraphicsDevice::Setup(ISystemConfig* systemConfig)
{
	auto l_SystemConfig = static_cast<IRenderingGraphicsDeviceSystemConfig*>(systemConfig);
	m_RenderingServer = l_SystemConfig->m_RenderingServer;
	m_RenderingComponentPool = l_SystemConfig->m_RenderingComponentPool;

    bool l_result = CreateHardwareResources();

    m_SwapChainRenderPassComp = m_RenderingComponentPool->AddRenderPassComponent("SwapChain/");
    m_SwapChainSPC = m_RenderingComponentPool->AddShaderProgramComponent("SwapChain/");
    m_SwapChainSamplerComp = m_RenderingComponentPool->AddSamplerComponent("SwapChain/");

    m_ObjectStatus = ObjectStatus::Created;

    Log(Success, "DX12RenderingServer Setup finished.");
}

bool IRenderingGraphicsDevice::Initialize()
{
    if (m_ObjectStatus != ObjectStatus::Created)
    {
        Log(Error, "DX12RenderingServer is not in Created state.");
        return false;
    }

    m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
    m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

    m_RenderingServer->InitializeShaderProgramComponent(m_SwapChainSPC);

    m_RenderingServer->InitializeSamplerComponent(m_SwapChainSamplerComp);

    if (!GetSwapChainImages())
        return false;

    auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

    l_RenderPassDesc.m_RenderTargetCount = m_swapChainImageCount;
    l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&IRenderingGraphicsDevice::AssignSwapChainImages, this);

    m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
    m_SwapChainRenderPassComp->m_RenderPassDesc.m_UseMultiFrames = true;
    m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
    m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(4);
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_IsRootConstant = true;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	// m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResource = m_SwapChainGPUBufferComp;

    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;

    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
    m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResource = m_SwapChainSamplerComp;

    m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainSPC;

    m_RenderingServer->InitializeRenderPassComponent(m_SwapChainRenderPassComp);

    return true;
}

bool IRenderingGraphicsDevice::Update()
{
    return true;
}

bool IRenderingGraphicsDevice::OnFrameEnd()
{
    return true;
}

bool IRenderingGraphicsDevice::Terminate()
{
    m_RenderingComponentPool->Delete(m_SwapChainSamplerComp);
    m_RenderingComponentPool->Delete(m_SwapChainSPC);
    m_RenderingComponentPool->Delete(m_SwapChainRenderPassComp);

    ReleaseHardwareResources();

    m_ObjectStatus = ObjectStatus::Terminated;
    Log(Success, "DX12RenderingServer has been terminated.");
}

uint32_t IRenderingGraphicsDevice::GetSwapChainImageCount()
{
    return m_swapChainImageCount;
}

RenderPassComponent* IRenderingGraphicsDevice::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

bool IRenderingGraphicsDevice::FinalizeSwapChain()
{
	m_RenderingServer->CommandListBegin(m_SwapChainRenderPassComp, m_SwapChainRenderPassComp->m_CurrentFrame);

	m_RenderingServer->BindRenderPassComponent(m_SwapChainRenderPassComp);

	m_RenderingServer->ClearRenderTargets(m_SwapChainRenderPassComp);

	//m_RenderingServer->Bind(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_GetUserPipelineOutputFunc(), 0);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Square);

	m_RenderingServer->DrawIndexedInstanced(m_SwapChainRenderPassComp, l_mesh, 1);

	//m_RenderingServer->Unbind(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_GetUserPipelineOutputFunc(), 0);

	m_RenderingServer->CommandListEnd(m_SwapChainRenderPassComp);

	m_RenderingServer->ExecuteCommandList(m_SwapChainRenderPassComp, GPUEngineType::Graphics);

	return true;
}

bool IRenderingGraphicsDevice::Present()
{
	auto l_currentFrame = m_SwapChainRenderPassComp->m_CurrentFrame;
	auto l_commandList = m_SwapChainRenderPassComp->m_CommandLists[l_currentFrame];
	m_RenderingServer->TryToTransitState(m_SwapChainRenderPassComp->m_RenderTargets[l_currentFrame].m_Texture, l_commandList, Accessibility::ReadOnly);

	PresentImpl();

	m_RenderingServer->WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
	m_RenderingServer->WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Compute);

	PostPresent();

	// if (m_needResize)
	// {
	// 	ResizeImpl();

	// 	m_SwapChainRenderPassComp->m_CurrentFrame = 0;
	// 	m_needResize = false;
	// }

	return true;
}

bool IRenderingServer::Setup(ISystemConfig *systemConfig)
{
	m_Device = CreateRenderingGraphicsDevice();
	m_RenderingComponentPool = CreateRenderingComponentPool();
	return m_Device->Setup(systemConfig);
}

bool IRenderingServer::Initialize()
{
	return m_Device->Initialize();
}

bool IRenderingServer::Terminate()
{
	auto l_result = m_Device->Terminate();
	delete m_Device;
	delete m_RenderingComponentPool;
}

ObjectStatus IRenderingServer::GetStatus()
{
	return m_Device->GetStatus();
}

bool IRenderingServer::InitializeMaterialComponent(MaterialComponent *rhs)
{
	auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();

	for (size_t i = 0; i < MaxTextureSlotCount; i++)
	{
		auto l_texture = rhs->m_TextureSlots[i].m_Texture;
		if (l_texture)
		{
			InitializeTextureComponent(l_texture);
			rhs->m_TextureSlots[i].m_Texture = l_texture;
			rhs->m_TextureSlots[i].m_Activated = true;
		}
		else
		{
			// @TODO: Maybe not necessary to use the default texture.
			rhs->m_TextureSlots[i].m_Texture = l_defaultMaterial->m_TextureSlots[i].m_Texture;
		}
	}

	rhs->m_GPUResourceType = GPUResourceType::Material;
	rhs->m_ObjectStatus = ObjectStatus::Activated;

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

			InitializeTextureComponent(l_TextureComp);

			Log(Verbose, "Render target: ", rhs->m_RenderTargets[i].m_Texture->m_InstanceName.c_str(), " has been initialized.");
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

			InitializeTextureComponent(rhs->m_DepthStencilRenderTarget.m_Texture);
			Log(Verbose, "", rhs->m_InstanceName.c_str(), " depth stencil target has been initialized.");
		}
	}

	return true;
}

bool IRenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_size = rhs->m_TotalSize;
	if (range != SIZE_MAX)
		l_size = range * rhs->m_ElementSize;

	std::memcpy((char *)rhs->m_MappedMemory + startOffset * rhs->m_ElementSize, GPUBufferValue, l_size);
	
	m_RenderingComponentPool->InitializeGPUBufferComponent(rhs);

	return true;
}

bool IRenderingServer::Resize()
{
	m_needResize = true;
	return true;
}

bool IRenderingServer::ResizeImpl()
{
	m_RenderingComponentPool->PreResize();
	m_Device->Resize();
	m_RenderingComponentPool->PostResize();

    return true;
}

bool IRenderingComponentPool::PreResize()
{
	for (auto i : m_initializedRenderPasses)
    {
        if (!PreResize(i))
        {
            Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
            return false;
        }
    }
}

bool IRenderingComponentPool::PreResize(RenderPassComponent* rhs)
{
	if (!rhs->m_RenderPassDesc.m_Resizable)
		return true;

	for (auto i : rhs->m_RenderTargets)
	{
		if (i.m_IsOwned)
			Delete(i.m_Texture);
	}

	rhs->m_RenderTargets.clear();

	if (rhs->m_DepthStencilRenderTarget.m_IsOwned && rhs->m_DepthStencilRenderTarget.m_Texture)
	{
		Delete(rhs->m_DepthStencilRenderTarget.m_Texture);
	}

	Delete(rhs->m_PipelineStateObject);

    return true;
}

bool IRenderingComponentPool::PostResize()
{
    for (auto i : m_initializedRenderPasses)
    {
        // if (!m_Device->PostResize(l_screenResolution, i))
        // {
        //     Log(Error, "Can't resize ", i->m_InstanceName);
        //     return false;
        // }
    }

	return true;
}

void IRenderingComponentPool::TransferDataToGPU()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshComponent* l_Mesh;
		m_uninitializedMeshes.tryPop(l_Mesh);

		if (l_Mesh)
		{
			InitializeMeshComponent(l_Mesh);
			if (l_Mesh->m_ObjectStatus == ObjectStatus::Activated)
				m_initializedMeshes.emplace(l_Mesh);
		}
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialComponent* l_Material;
		m_uninitializedMaterials.tryPop(l_Material);

		if (l_Material)
		{
			InitializeMaterialComponent(l_Material);
			if (l_Material->m_ObjectStatus == ObjectStatus::Activated)
				m_initializedMaterials.emplace(l_Material);
		}
	}
	
	if (m_uninitializedBuffers.empty())
		return;

	// auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	// l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	// for (auto rhs : m_uninitializedBuffers)
	// {
	// 	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);
	// 	if (!l_rhs->m_DefaultHeapBuffer)
	// 		continue;
		
	// 	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	// 	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffer.Get());
	// 	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	// }

	// DX12Helper::ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetDevice(), m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	
	m_uninitializedBuffers.clear();
}

void IRenderingComponentPool::InitializeMeshComponent(MeshComponent* rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
		return;

	m_uninitializedMeshes.push(rhs);
}

void IRenderingComponentPool::InitializeMaterialComponent(MaterialComponent* rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
		return;

	m_uninitializedMaterials.push(rhs);
}

void IRenderingComponentPool::InitializeGPUBufferComponent(GPUBufferComponent* rhs)
{
	if (std::find(m_uninitializedBuffers.begin(), m_uninitializedBuffers.end(), rhs) != m_uninitializedBuffers.end())
		return;

	m_uninitializedBuffers.emplace_back(rhs);
}

void IRenderingComponentPool::InitializeRenderPassComponent(RenderPassComponent* rhs)
{
	if (std::find(m_initializedRenderPasses.begin(), m_initializedRenderPasses.end(), rhs) != m_initializedRenderPasses.end())
		return;

	InitializeRenderPassComponent(rhs);
	if (rhs->m_ObjectStatus == ObjectStatus::Activated)
		m_initializedRenderPasses.push_back(rhs);
}

bool IRenderingComponentPool::ReserveRenderTargets(RenderPassComponent* rhs)
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
