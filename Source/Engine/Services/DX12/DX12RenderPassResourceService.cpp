#include "DX12RenderPassResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_BindlessMesh.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../FrameManagementService.h"
#include "../TextureResourceService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderPassResourceService::Setup(IServiceConfig* systemConfig)
{
	RenderPassResourceService::Setup(systemConfig);

	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(128);

	return true;
}

bool DX12RenderPassResourceService::Terminate()
{
	delete m_PSOPool;
	delete m_SemaphorePool;
	delete m_OutputMergerTargetPool;

	RenderPassResourceService::Terminate();

	return true;
}

IPipelineStateObject* DX12RenderPassResourceService::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* DX12RenderPassResourceService::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool DX12RenderPassResourceService::Add(IOutputMergerTarget*& rhs)
{
	rhs = m_OutputMergerTargetPool->Spawn();
	return rhs != nullptr;
}

bool DX12RenderPassResourceService::Delete(RenderPassComponent* ptr)
{
	return RenderPassResourceService::Delete(ptr);
}

bool DX12RenderPassResourceService::Delete(IPipelineStateObject* rhs)
{
	auto l_rhs = reinterpret_cast<DX12PipelineStateObject*>(rhs);
	l_rhs->m_PSO.Reset();
	m_PSOPool->Destroy(l_rhs);
	return true;
}

bool DX12RenderPassResourceService::Delete(ISemaphore* rhs)
{
	auto l_rhs = reinterpret_cast<DX12Semaphore*>(rhs);
	m_SemaphorePool->Destroy(l_rhs);
	return true;
}

bool DX12RenderPassResourceService::Delete(IOutputMergerTarget* rhs)
{
	auto l_rhs = reinterpret_cast<DX12OutputMergerTarget*>(rhs);
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	for (auto& j : l_rhs->m_ColorOutputs)
	{
		if (j)
			l_textureService->Delete(j);
	}

	l_rhs->m_ColorOutputs.clear();

	if (l_rhs->m_DepthStencilOutput)
		l_textureService->Delete(l_rhs->m_DepthStencilOutput);

	l_rhs->m_DepthStencilOutput = nullptr;

	m_OutputMergerTargetPool->Destroy(l_rhs);

	return true;
}

Vec4 DX12RenderPassResourceService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

bool DX12RenderPassResourceService::CreateFenceEvents(RenderPassComponent* renderPass)
{
	bool result = true;
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(renderPass->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for compute CommandQueue.");
			result = false;
		}

		l_semaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_CopyCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for copy CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, renderPass->m_InstanceName, " Fence events have been created.");
	}

	return result;
}

bool DX12RenderPassResourceService::OnOutputMergerTargetsCreated(RenderPassComponent* renderPass)
{
	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		auto& l_RTVs = l_outputMergerTarget->m_RTVs;
		if (l_RTVs.size() == 0)
		{
			l_RTVs.resize(l_swapChainImageCount);
			for (size_t i = 0; i < l_RTVs.size(); i++)
			{
				auto& l_RTV = l_RTVs[i];
				l_RTV.m_Desc = GetRTVDesc(renderPass->m_RenderPassDesc.m_RenderTargetDesc);
				l_RTV.m_Handles.resize(renderPass->m_RenderPassDesc.m_RenderTargetCount);
				for (size_t j = 0; j < l_RTV.m_Handles.size(); j++)
				{
					auto l_handle = m_ctx->m_RTVDescHeapAccessor.GetNewHandle();
					l_RTV.m_Handles[j] = D3D12_CPU_DESCRIPTOR_HANDLE{ l_handle.m_CPUHandle };
				}
			}
		}

		for (size_t i = 0; i < l_RTVs.size(); i++)
		{
			auto& l_RTV = l_RTVs[i];
			for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
			{
				auto l_renderTarget = static_cast<ID3D12Resource*>(l_outputMergerTarget->m_ColorOutputs[j]->GetGPUResource(i));
				m_ctx->m_device->CreateRenderTargetView(l_renderTarget, &l_RTV.m_Desc, l_RTV.m_Handles[j]);
			}
		}
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		auto& l_DSVs = l_outputMergerTarget->m_DSVs;
		if (l_DSVs.size() == 0)
		{
			l_DSVs.resize(l_swapChainImageCount);
			for (size_t i = 0; i < l_DSVs.size(); i++)
			{
				l_DSVs[i].m_Desc = GetDSVDesc(renderPass->m_RenderPassDesc.m_RenderTargetDesc, renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
				l_DSVs[i].m_Handle = D3D12_CPU_DESCRIPTOR_HANDLE{ m_ctx->m_DSVDescHeapAccessor.GetNewHandle().m_CPUHandle };
			}
		}

		auto l_renderTargetTexture = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
		for (size_t i = 0; i < l_DSVs.size(); i++)
		{
			auto l_renderTarget = static_cast<ID3D12Resource*>(l_outputMergerTarget->m_DepthStencilOutput->GetGPUResource(i));
			m_ctx->m_device->CreateDepthStencilView(l_renderTarget, &l_DSVs[i].m_Desc, l_DSVs[i].m_Handle);
		}
	}

	return true;
}
