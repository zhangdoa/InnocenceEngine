#include "DX12MeshResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"
#include "../../Services/AssetService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12MeshResourceService::Delete(MeshComponent* mesh)
{
	if (mesh->m_Asset.IsValid())
		ReleaseMeshGPUResourceImpl(mesh->m_Asset);

	return MeshResourceService::Delete(mesh);
}

void DX12MeshResourceService::ReleaseMeshGPUResourceImpl(MeshAssetHandle handle)
{
	auto it = m_DX12MeshResources.find(handle.m_Index);
	if (it != m_DX12MeshResources.end())
	{
		it->second.m_VertexBuffer_Upload.Reset();
		it->second.m_VertexBuffer_Default.Reset();
		it->second.m_IndexBuffer_Upload.Reset();
		it->second.m_IndexBuffer_Default.Reset();
		it->second.m_BLAS.Reset();
		it->second.m_ScratchBuffer.Reset();
		m_DX12MeshResources.erase(it);
	}
}

bool DX12MeshResourceService::InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto* l_resource = AssetService::GetMeshAsset(handle);
	if (!l_resource)
	{
		Log(Error, "InitializeImpl: invalid MeshAssetHandle");
		return false;
	}

	auto l_name = l_resource->m_Name.c_str();

	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);

	auto l_defaultHeapBuffer_VB = m_ctx->CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (!l_defaultHeapBuffer_VB)
	{
		Log(Error, l_name, " can't create vertex buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_defaultHeapBuffer_VB, "DefaultHeap_VB");
#endif

	auto l_uploadHeapBuffer_VB = m_ctx->CreateUploadHeapBuffer(&l_verticesResourceDesc);
	if (!l_uploadHeapBuffer_VB)
	{
		Log(Error, l_name, " can't create vertex buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_uploadHeapBuffer_VB, "UploadHeap_VB");
#endif

	l_resource->m_VertexBufferView.m_BufferLocation = l_defaultHeapBuffer_VB->GetGPUVirtualAddress();
	l_resource->m_VertexBufferView.m_SizeInBytes = l_verticesDataSize;
	l_resource->m_VertexBufferView.m_StrideInBytes = sizeof(Vertex);

	auto l_indicesDataSize = uint32_t(sizeof(Index) * indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);

	auto l_defaultHeapBuffer_IB = m_ctx->CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (!l_defaultHeapBuffer_IB)
	{
		Log(Error, l_name, " can't create index buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_defaultHeapBuffer_IB, "DefaultHeap_IB");
#endif

	auto l_uploadHeapBuffer_IB = m_ctx->CreateUploadHeapBuffer(&l_indicesResourceDesc);
	if (!l_uploadHeapBuffer_IB)
	{
		Log(Error, l_name, " can't create index buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_uploadHeapBuffer_IB, "UploadHeap_IB");
#endif

	l_resource->m_IndexBufferView.m_BufferLocation = l_defaultHeapBuffer_IB->GetGPUVirtualAddress();
	l_resource->m_IndexBufferView.m_SizeInBytes = l_indicesDataSize;
	l_resource->m_IndexBufferView.m_StrideInBytes = sizeof(Index);

	for (auto& i : vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_uploadHeapBuffer_VB->Map(0, &m_readRange, &l_resource->m_MappedMemory_VB);
	l_uploadHeapBuffer_IB->Map(0, &m_readRange, &l_resource->m_MappedMemory_IB);

	std::memcpy((char*)l_resource->m_MappedMemory_VB, &vertices[0], vertices.size() * sizeof(Vertex));
	std::memcpy((char*)l_resource->m_MappedMemory_IB, &indices[0], indices.size() * sizeof(Index));

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	CommandListComponent l_commandList = {};
	l_commandList.m_Type = GPUEngineType::Graphics;
	auto l_dx12CommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame), L"MeshInitCommandList");
	l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

	l_dx12CommandList->CopyResource(l_defaultHeapBuffer_VB.Get(), l_uploadHeapBuffer_VB.Get());
	l_dx12CommandList->CopyResource(l_defaultHeapBuffer_IB.Get(), l_uploadHeapBuffer_IB.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = l_defaultHeapBuffer_VB->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = l_defaultHeapBuffer_IB->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(indices.size());
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geometryDesc;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	m_ctx->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
	{
		Log(Error, l_name, " Failed to get prebuild info for BLAS!");
		return false;
	}

	auto blasResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_BLAS = m_ctx->CreateDefaultHeapBuffer(&blasResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	if (!l_BLAS)
	{
		Log(Error, l_name, " Failed to create BLAS buffer!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_BLAS, "BLAS");
#endif

	auto scratchResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_scratchBuffer = m_ctx->CreateDefaultHeapBuffer(&scratchResourceDesc);
	if (!l_scratchBuffer)
	{
		Log(Error, l_name, " Failed to create scratch buffer for BLAS!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(l_name, l_scratchBuffer, "ScratchBuffer_BLAS");
#endif

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = l_scratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = l_BLAS->GetGPUVirtualAddress();

	l_dx12CommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_BLAS.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	l_fmService->Close(&l_commandList, GPUEngineType::Graphics);
	l_hwService->Execute(&l_commandList, GPUEngineType::Graphics);
	auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();
	l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
	auto l_semaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
	l_hwService->WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);

	DX12MeshGPUResources l_dx12Resources;
	l_dx12Resources.m_VertexBuffer_Upload = l_uploadHeapBuffer_VB;
	l_dx12Resources.m_VertexBuffer_Default = l_defaultHeapBuffer_VB;
	l_dx12Resources.m_IndexBuffer_Upload = l_uploadHeapBuffer_IB;
	l_dx12Resources.m_IndexBuffer_Default = l_defaultHeapBuffer_IB;
	l_dx12Resources.m_BLAS = l_BLAS;
	l_dx12Resources.m_ScratchBuffer = l_scratchBuffer;
	m_DX12MeshResources[handle.m_Index] = std::move(l_dx12Resources);

	Log(Verbose, l_name, " BLAS is initialized.");

	return true;
}
