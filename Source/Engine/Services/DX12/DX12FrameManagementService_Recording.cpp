#include "DX12FrameManagementService.h"
#include "../../Component/TextureComponent.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

// --- Command recording ---

bool DX12FrameManagementService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPass)
	{
		Log(Warning, "DX12FrameManagementService::CommandListBegin: null ", (!commandList ? "commandList" : "renderPass"));
		return false;
	}

	return Open(commandList, commandList->m_Type, renderPass->m_PipelineStateObject);
}

bool DX12FrameManagementService::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindRenderPassComponent");
		return false;
	}

	auto l_dx12CommandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_dx12CommandList)
	{
		Log(Error, "Invalid DX12 command list in BindRenderPassComponent");
		return false;
	}

	ChangeRenderTargetStates(renderPass, commandList, Accessibility::ReadOnly, Accessibility::WriteOnly);
	SetDescriptorHeaps(renderPass, commandList);
	SetRenderTargets(renderPass, commandList);

	if (renderPass->m_PipelineStateObject)
	{
		auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
		PreparePipeline(renderPass, commandList, l_PSO);
	}

	if (renderPass->m_CustomCommandsFunc)
		renderPass->m_CustomCommandsFunc(commandList);

	return true;
}

bool DX12FrameManagementService::ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in ClearRenderTargets");
		return false;
	}

	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (renderPass->m_RenderPassDesc.m_RenderTargetCount == 0)
		return true;

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "Invalid DX12 command list in ClearRenderTargets");
		return false;
	}

	auto f_clearRenderTargetAsUAV = [&](DX12DeviceMemory* l_RT)
		{
			if (renderPass->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
			{
				l_commandList->ClearUnorderedAccessViewUint(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle },
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(),
					(UINT*)renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0, nullptr);
			}
			else
			{
				l_commandList->ClearUnorderedAccessViewFloat(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle },
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(),
					renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0, nullptr);
			}
		};

	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);
	if (!l_outputMergerTarget)
	{
		Log(Error, "Output merger target is null in ClearRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_currentFrame = GetCurrentFrame();
	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		if (l_currentFrame >= l_outputMergerTarget->m_RTVs.size())
		{
			Log(Error, "Invalid frame index ", l_currentFrame, " for RTVs in render pass ", renderPass->m_InstanceName);
			return false;
		}

		auto& l_RTV = l_outputMergerTarget->m_RTVs[l_currentFrame];
		if (l_RTV.m_Handles.empty())
		{
			Log(Error, "RTV handles are empty for render pass ", renderPass->m_InstanceName);
			return false;
		}

		if (index != -1 && index < renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			if (index >= l_RTV.m_Handles.size())
			{
				Log(Error, "Invalid RTV handle index ", index, " for render pass ", renderPass->m_InstanceName);
				return false;
			}
			if (l_RTV.m_Handles[index].ptr == 0)
			{
				Log(Error, "Null RTV handle at index ", index, " for render pass ", renderPass->m_InstanceName);
				return false;
			}
			l_commandList->ClearRenderTargetView(l_RTV.m_Handles[index], renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
		}
		else
		{
			for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				if (i >= l_RTV.m_Handles.size())
				{
					Log(Error, "Invalid RTV handle index ", i, " for render pass ", renderPass->m_InstanceName);
					return false;
				}
				if (l_RTV.m_Handles[i].ptr == 0)
				{
					Log(Error, "Null RTV handle at index ", i, " for render pass ", renderPass->m_InstanceName);
					return false;
				}
				l_commandList->ClearRenderTargetView(l_RTV.m_Handles[i], renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
		}
	}
	else
	{
		if (index != -1 && index < renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(l_outputMergerTarget->m_ColorOutputs[index]->m_GPUResources[l_currentFrame]));
		}
		else
		{
			for (auto i : l_outputMergerTarget->m_ColorOutputs)
			{
				f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(i->m_GPUResources[l_currentFrame]));
			}
		}
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			l_flag |= D3D12_CLEAR_FLAG_STENCIL;

		auto l_DSV = l_outputMergerTarget->m_DSVs[l_currentFrame];
		l_commandList->ClearDepthStencilView(l_DSV.m_Handle, l_flag, 1.0f, 0x00, 0, nullptr);
	}

	return true;
}

bool DX12FrameManagementService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in CommandListEnd");
		return false;
	}

	// TASK-155 F2 cross-queue exit: when the producer pass declares its render targets
	// will next be read by a different queue type, drop them to COMMON at the end of the
	// producer CL (record-order = execute-order on a single queue, so the engine-side
	// m_CurrentState mutation matches what the GPU will see). Consumers on the receiving
	// queue then implicitly promote from COMMON without recording a barrier of their own.
	if (renderPass->m_RenderPassDesc.m_PostCLState == CrossQueueExit::ToCommon
		&& renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		ChangeRenderTargetStates(renderPass, commandList, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	}

	if (!Close(commandList, renderPass->m_RenderPassDesc.m_GPUEngineType))
	{
		Log(Error, "Failed to close command list for render pass ", renderPass->m_InstanceName);
		return false;
	}

	return true;
}
