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
	Log(Verbose, " starting.");
	if (!RenderPassResourceService::Setup(systemConfig))
	{
		Log(Error, " base RenderPassResourceService::Setup failed; aborting.");
		return false;
	}

	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(512);
	if (!m_PSOPool)
	{
		Log(Error, " failed to create PSO pool; aborting.");
		return false;
	}
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(1024);
	if (!m_SemaphorePool)
	{
		Log(Error, " failed to create Semaphore pool; aborting.");
		return false;
	}
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(512);
	if (!m_OutputMergerTargetPool)
	{
		Log(Error, " failed to create OMT pool; aborting.");
		return false;
	}

	Log(Verbose, " finished.");
	return true;
}

bool DX12RenderPassResourceService::Terminate()
{
	Log(Verbose, " starting.");
	if (m_PSOPool)
	{
		delete m_PSOPool;
		m_PSOPool = nullptr;
	}
	if (m_SemaphorePool)
	{
		delete m_SemaphorePool;
		m_SemaphorePool = nullptr;
	}
	if (m_OutputMergerTargetPool)
	{
		delete m_OutputMergerTargetPool;
		m_OutputMergerTargetPool = nullptr;
	}

	if (!RenderPassResourceService::Terminate())
	{
		Log(Error, " base RenderPassResourceService::Terminate failed; aborting.");
		return false;
	}

	Log(Verbose, " finished.");
	return true;
}

IPipelineStateObject* DX12RenderPassResourceService::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* DX12RenderPassResourceService::AddSemaphore()
{
	auto* l_result = m_SemaphorePool->Spawn();
	if (!l_result)
		Log(Error, " AddSemaphore failed (pool returned nullptr; capacity exhausted or pool uninitialized).");
	return l_result;
}

bool DX12RenderPassResourceService::Add(IOutputMergerTarget*& rhs)
{
	rhs = m_OutputMergerTargetPool->Spawn();
	if (!rhs)
	{
		Log(Error, " Add: OutputMergerTarget pool returned nullptr; aborting.");
		return false;
	}
	return true;
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
		{
			if (!l_textureService->Delete(j))
			{
				Log(Error, " TextureService->Delete failed for color output; aborting.");
				return false;
			}
		}
	}

	l_rhs->m_ColorOutputs.clear();

	if (l_rhs->m_DepthStencilOutput)
	{
		if (!l_textureService->Delete(l_rhs->m_DepthStencilOutput))
		{
			Log(Error, " TextureService->Delete failed for depth-stencil output; aborting.");
			return false;
		}
	}
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
		if (!l_semaphore)
		{
			Log(Error, renderPass->m_InstanceName, " CreateFenceEvents: m_Semaphores[", i, "] is nullptr (AddSemaphore failed earlier; pass will be skipped).");
			result = false;
			continue;
		}
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


		for (size_t i = 0; i < l_DSVs.size(); i++)
		{
			auto l_renderTarget = static_cast<ID3D12Resource*>(l_outputMergerTarget->m_DepthStencilOutput->GetGPUResource(i));
			m_ctx->m_device->CreateDepthStencilView(l_renderTarget, &l_DSVs[i].m_Desc, l_DSVs[i].m_Handle);
		}
	}

	return true;
}
