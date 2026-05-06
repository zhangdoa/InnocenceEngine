#include "DX12FrameManagementService.h"
#include "../../Component/GPUBufferComponent.h"
#include "../../Component/TextureComponent.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12FrameManagementService::TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));

	auto l_oldState = static_cast<D3D12_RESOURCE_STATES>(texture->GetCurrentState(frameIndex));
	auto l_newState = static_cast<D3D12_RESOURCE_STATES>(targetAccessibility.CanWrite() ? texture->m_WriteState : texture->m_ReadState);
	if (targetAccessibility.IsCopySource())
		l_newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (targetAccessibility.IsCopyDestination())
		l_newState = D3D12_RESOURCE_STATE_COPY_DEST;
	// CrossQueueTransition: transition to COMMON so the resource can be read by a different queue type
	// without a barrier on the receiving queue (D3D12 implicit promotion from COMMON applies).
	if (targetAccessibility.IsCrossQueue())
		l_newState = D3D12_RESOURCE_STATE_COMMON;

	// Compute command lists cannot use PIXEL_SHADER_RESOURCE state in barriers.
	// Strip it from the target state; the source state must be compute-compatible
	// (caller is responsible for doing cross-queue transitions on Graphics).
	if (commandList->m_Type == GPUEngineType::Compute)
		l_newState &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	if (l_oldState != l_newState)
	{
		auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			l_oldState,
			l_newState);

		l_commandList->ResourceBarrier(1, &l_transition);

		texture->SetCurrentState(frameIndex, static_cast<uint32_t>(l_newState));
	}

	return true;
}

bool DX12FrameManagementService::TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[frameIndex]);

	auto l_oldState = static_cast<D3D12_RESOURCE_STATES>(gpuBuffer->GetCurrentState(frameIndex));
	auto l_newState = static_cast<D3D12_RESOURCE_STATES>(targetAccessibility.CanWrite() ? gpuBuffer->m_WriteState : gpuBuffer->m_ReadState);
	if (targetAccessibility.IsCopySource())
		l_newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (targetAccessibility.IsCopyDestination())
		l_newState = D3D12_RESOURCE_STATE_COPY_DEST;

	if (l_oldState != l_newState)
	{
		auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			l_deviceMemory->m_DefaultHeapBuffer.Get(),
			l_oldState,
			l_newState);

		l_commandList->ResourceBarrier(1, &l_transition);

		gpuBuffer->SetCurrentState(frameIndex, l_newState);
	}

	return true;
}

bool DX12FrameManagementService::SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "Command list is null in SetDescriptorHeaps for render pass ", renderPass->m_InstanceName);
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get(), m_ctx->m_SamplerDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12FrameManagementService::SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (!commandList || !commandList->m_CommandList)
	{
		Log(Error, "Command list is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_currentFrame = GetCurrentFrame();
	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);

	if (!l_outputMergerTarget)
	{
		Log(Error, "Output merger target is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* l_DSV = NULL;
	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		l_DSV = &l_outputMergerTarget->m_DSVs[l_currentFrame].m_Handle;

	D3D12_CPU_DESCRIPTOR_HANDLE* l_RTVs = NULL;
	uint32_t l_RTCount = 0;
	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		if (renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			l_RTVs = &l_outputMergerTarget->m_RTVs[l_currentFrame].m_Handles[0];
			l_RTCount = (uint32_t)renderPass->m_RenderPassDesc.m_RenderTargetCount;
		}
	}

	l_commandList->OMSetRenderTargets(l_RTCount, l_RTVs, FALSE, l_DSV);
	return true;
}

bool DX12FrameManagementService::PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

	if (!l_commandList)
	{
		Log(Error, "Command list is null in PreparePipeline for render pass ", renderPass->m_InstanceName);
		return false;
	}

	if (PSO->m_PSO)
		l_commandList->SetPipelineState(PSO->m_PSO.Get());

	if (PSO->m_RaytracingPSO)
		l_commandList->SetPipelineState1(PSO->m_RaytracingPSO.Get());

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (PSO->m_RootSignature)
			l_commandList->SetGraphicsRootSignature(PSO->m_RootSignature.Get());

		if (PSO->m_PSO)
		{
			l_commandList->RSSetViewports(1, &PSO->m_Viewport);
			l_commandList->RSSetScissorRects(1, &PSO->m_Scissor);
			if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
			{
				l_commandList->OMSetStencilRef(renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference);
			}
		}
	}
	else
	{
		if (PSO->m_RootSignature)
			l_commandList->SetComputeRootSignature(PSO->m_RootSignature.Get());
	}

	return true;
}

bool DX12FrameManagementService::ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;

	for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
	{
		auto l_renderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[i]);
		TryToTransitState(l_renderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_depthStencilRenderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
		TryToTransitState(l_depthStencilRenderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	return true;
}
