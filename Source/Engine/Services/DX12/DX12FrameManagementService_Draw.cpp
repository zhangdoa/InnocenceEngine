#include "DX12FrameManagementService.h"
#include "../GPUBufferResourceService.h"
#include "../../Component/MeshComponent.h"
#include "../../Engine.h"
#include "../../Services/DrawCallService.h"
#include "../../Services/AssetService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12FrameManagementService::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	if (!renderPass || !commandList || !mesh)
	{
		Log(Error, "Null parameters in DrawIndexedInstanced");
		return false;
	}

	auto* l_resource = AssetService::GetMeshAsset(mesh->m_Asset);
	if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
	{
		Log(Warning, "DX12FrameManagementService::DrawIndexedInstanced: mesh asset not resident");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = l_resource->m_VertexBufferView.m_BufferLocation;
	vbv.StrideInBytes = l_resource->m_VertexBufferView.m_StrideInBytes;
	vbv.SizeInBytes = l_resource->m_VertexBufferView.m_SizeInBytes;

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = l_resource->m_IndexBufferView.m_BufferLocation;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = l_resource->m_IndexBufferView.m_SizeInBytes;

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 1, &vbv);
	l_commandList->IASetIndexBuffer(&ibv);
	l_commandList->DrawIndexedInstanced(l_resource->GetIndexCount(), (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12FrameManagementService::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DrawInstanced");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 0, nullptr);
	l_commandList->IASetIndexBuffer(nullptr);
	l_commandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12FrameManagementService::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in Dispatch");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in Dispatch for render pass ", renderPass->m_InstanceName);
		return false;
	}

	l_commandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12FrameManagementService::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DispatchRays");
		return false;
	}

	if (!g_Engine->Get<GPUBufferResourceService>()->IsTLASReady())
	{
		Log(Warning, "DX12FrameManagementService::DispatchRays: TLAS not ready, skipping for ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	if (l_PSO->m_RaytracingMissShaderCount == 0 || l_PSO->m_RaytracingHitGroupCount == 0)
	{
		Log(Error, "DispatchRays: PSO for ", renderPass->m_InstanceName,
			" has missShaderCount=", l_PSO->m_RaytracingMissShaderCount,
			" hitGroupCount=", l_PSO->m_RaytracingHitGroupCount,
			" — any TraceRay() in this pass will read past the shader table. Skipping dispatch (TASK-35).");
		return false;
	}

	auto l_shaderIDBufferVirtualAddress = l_PSO->m_RaytracingShaderIDBuffer->GetGPUVirtualAddress();

	// Validate the shader-ID buffer actually holds the number of records the PSO recorded
	// at creation (TASK-35). Layout: [RayGen][Miss × missCount][HitGroup × hitCount].
	D3D12_RESOURCE_DESC l_bufDesc = l_PSO->m_RaytracingShaderIDBuffer->GetDesc();
	const uint32_t l_expectedSlots = 1u + l_PSO->m_RaytracingMissShaderCount + l_PSO->m_RaytracingHitGroupCount;
	const uint64_t l_expectedBytes = static_cast<uint64_t>(l_expectedSlots) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	if (l_bufDesc.Width < l_expectedBytes)
	{
		Log(Error, "DispatchRays: shader-ID buffer too small for ", renderPass->m_InstanceName,
			" expected ", l_expectedBytes, " bytes (", l_expectedSlots, " slots)",
			" got ", l_bufDesc.Width, " bytes. Skipping dispatch (TASK-35).");
		return false;
	}

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	dispatchDesc.RayGenerationShaderRecord.StartAddress = l_shaderIDBufferVirtualAddress;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.MissShaderTable.StartAddress = l_shaderIDBufferVirtualAddress + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.SizeInBytes = static_cast<uint64_t>(l_PSO->m_RaytracingMissShaderCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	const uint64_t hitGroupOffset = static_cast<uint64_t>(1u + l_PSO->m_RaytracingMissShaderCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.HitGroupTable.StartAddress = l_shaderIDBufferVirtualAddress + hitGroupOffset;
	dispatchDesc.HitGroupTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.HitGroupTable.SizeInBytes = static_cast<uint64_t>(l_PSO->m_RaytracingHitGroupCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.Width = dimensionX;
	dispatchDesc.Height = dimensionY;
	dispatchDesc.Depth = dimensionZ;

	l_commandList->DispatchRays(&dispatchDesc);

	return true;
}

bool DX12FrameManagementService::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	if (!renderPass || !commandList || !indirectDrawCommand)
	{
		Log(Error, "Null parameters in ExecuteIndirect");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(indirectDrawCommand->m_DeviceMemories[GetCurrentFrame()]);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

	auto l_modelCount = (uint32_t)g_Engine->Get<DrawCallService>()->GetGPUModelData().size();
	uint32_t l_bufferCapacity = static_cast<uint32_t>(indirectDrawCommand->m_ElementCount);
	UINT maxDrawCommandCount = l_modelCount < l_bufferCapacity ? l_modelCount : l_bufferCapacity;

	if (maxDrawCommandCount == 0)
	{
		Log(Warning, "DX12FrameManagementService::ExecuteIndirect: zero draw commands for ", renderPass->m_InstanceName);
		return false;
	}

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadWrite, Accessibility::ReadOnly);

	l_commandList->ExecuteIndirect(l_PSO->m_IndirectCommandSignature.Get(), maxDrawCommandCount, l_deviceMemory->m_DefaultHeapBuffer.Get(), 0, nullptr, 0);

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadOnly, Accessibility::ReadWrite);

	return true;
}

void DX12FrameManagementService::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
	if (!renderPass || !commandList)
	{
		Log(Warning, "DX12FrameManagementService::PushRootConstants: null ", (!renderPass ? "renderPass" : "commandList"));
		return;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
}
