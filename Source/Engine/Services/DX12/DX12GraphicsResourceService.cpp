#include "DX12GraphicsResourceService.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/Randomizer.h"
#include "../../Common/MathHelper.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/PhysicsSimulationService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/AssetService.h"
#include "../../Component/WorldTransformComponent.h"
#include "../../Component/MeshComponent.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

#ifdef max
#undef max
#endif

using namespace Inno;
using namespace DX12Helper;

// ---------------------------------------------------------------------------
// Pool
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializePool()
{
	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(128);

	return true;
}

bool DX12GraphicsResourceService::TerminatePool()
{
	m_DX12MeshResources.clear();
	m_TextureBuffers_Upload.clear();
	m_TextureBuffers_Default.clear();

	delete m_PSOPool;
	delete m_SemaphorePool;
	delete m_OutputMergerTargetPool;

	return true;
}

IPipelineStateObject* DX12GraphicsResourceService::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* DX12GraphicsResourceService::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool DX12GraphicsResourceService::Add(IOutputMergerTarget*& rhs)
{
	rhs = m_OutputMergerTargetPool->Spawn();
	return rhs != nullptr;
}

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::Delete(MeshComponent* mesh)
{
	if (mesh->m_Asset.IsValid())
		ReleaseMeshGPUResourceImpl(mesh->m_Asset);

	return true;
}

void DX12GraphicsResourceService::ReleaseMeshGPUResourceImpl(MeshAssetHandle handle)
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

bool DX12GraphicsResourceService::Delete(TextureComponent* texture)
{
	auto componentUUID = reinterpret_cast<uint64_t>(texture);

	auto uploadIt = m_TextureBuffers_Upload.find(componentUUID);
	if (uploadIt != m_TextureBuffers_Upload.end()) {
		if (uploadIt->second) uploadIt->second.Reset();
		m_TextureBuffers_Upload.erase(uploadIt);
	}

	auto defaultIt = m_TextureBuffers_Default.find(componentUUID);
	if (defaultIt != m_TextureBuffers_Default.end()) {
		for (auto& buf : defaultIt->second) buf.Reset();
		m_TextureBuffers_Default.erase(defaultIt);
	}

	texture->m_GPUResources.clear();
	texture->m_ReadHandles.clear();
	texture->m_WriteHandles.clear();

	return true;
}

bool DX12GraphicsResourceService::Delete(MaterialComponent* material)
{
	return true;
}

bool DX12GraphicsResourceService::Delete(GPUBufferComponent* gpuBuffer)
{
	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		if (l_DX12DeviceMemory->m_DefaultHeapBuffer)
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Reset();
	}

	gpuBuffer->m_DeviceMemories.clear();

	for (auto i : gpuBuffer->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		if (l_DX12MappedMemory->m_UploadHeapBuffer)
			l_DX12MappedMemory->m_UploadHeapBuffer.Reset();
	}

	gpuBuffer->m_MappedMemories.clear();

	return true;
}

bool DX12GraphicsResourceService::Delete(IPipelineStateObject* rhs)
{
	auto l_rhs = reinterpret_cast<DX12PipelineStateObject*>(rhs);
	l_rhs->m_PSO.Reset();
	m_PSOPool->Destroy(l_rhs);

	return true;
}

bool DX12GraphicsResourceService::Delete(CommandListComponent* rhs)
{
	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(rhs->m_CommandList);
	if (l_dx12CommandList)
	{
		l_dx12CommandList->Release();
	}

	return true;
}

bool DX12GraphicsResourceService::Delete(ISemaphore* rhs)
{
	auto l_rhs = reinterpret_cast<DX12Semaphore*>(rhs);

	m_SemaphorePool->Destroy(l_rhs);

	return true;
}

bool DX12GraphicsResourceService::Delete(IOutputMergerTarget* rhs)
{
	auto l_rhs = reinterpret_cast<DX12OutputMergerTarget*>(rhs);

	for (auto& j : l_rhs->m_ColorOutputs)
	{
		if (j)
			Delete(j);
	}

	l_rhs->m_ColorOutputs.clear();

	if (l_rhs->m_DepthStencilOutput)
		Delete(l_rhs->m_DepthStencilOutput);

	l_rhs->m_DepthStencilOutput = nullptr;

	m_OutputMergerTargetPool->Destroy(l_rhs);

	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (Mesh)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto* l_resource = AssetService::GetMeshAsset(handle);
	if (!l_resource)
	{
		Log(Error, "InitializeImpl: invalid MeshAssetHandle");
		return false;
	}

	auto l_name = l_resource->m_Name.c_str();

	// vertices
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

	// indices
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

	// Flip y texture coordinate
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

	// Transition buffers to copy destination state, upload, then back to their read states
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

	// Inline upload: copy from upload heap to default heap
	l_dx12CommandList->CopyResource(l_defaultHeapBuffer_VB.Get(), l_uploadHeapBuffer_VB.Get());
	l_dx12CommandList->CopyResource(l_defaultHeapBuffer_IB.Get(), l_uploadHeapBuffer_IB.Get());

	// Transition back to read states for normal rendering
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	// Create BLAS
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

	// Transition index and vertex buffers to NON_PIXEL_SHADER_RESOURCE state for BLAS build.
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

	// Transition the vertex and index buffers back to their original states after BLAS build.
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	l_hwService->Close(&l_commandList, GPUEngineType::Graphics);
	l_hwService->Execute(&l_commandList, GPUEngineType::Graphics);
	auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();
	l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
	auto l_semaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
	l_hwService->WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);

	// Store all DX12 resources in consolidated struct
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

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (Texture)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(TextureComponent* texture, void* textureData)
{
	texture->m_GPUResourceType = GPUResourceType::Image;
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	texture->m_WriteState = GetTextureWriteState(texture->m_TextureDesc);
	texture->m_ReadState = GetTextureReadState(texture->m_TextureDesc);

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	auto stateFrameCount = texture->m_TextureDesc.IsMultiBuffer ? l_swapChainImageCount : 1;
	auto l_initialState = static_cast<D3D12_RESOURCE_STATES>(texture->m_TextureDesc.Usage == TextureUsage::Sample ? texture->m_ReadState : texture->m_WriteState);
	texture->m_CurrentState.resize(stateFrameCount, l_initialState);

	D3D12_CLEAR_VALUE l_clearValue = {};
	bool useClearValue = false;
	if (texture->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (texture->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (texture->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_clearValue.Format = l_textureDesc.Format;
		l_clearValue.Color[0] = texture->m_TextureDesc.ClearColor[0];
		l_clearValue.Color[1] = texture->m_TextureDesc.ClearColor[1];
		l_clearValue.Color[2] = texture->m_TextureDesc.ClearColor[2];
		l_clearValue.Color[3] = texture->m_TextureDesc.ClearColor[3];
		useClearValue = true;
	}

	uint32_t frameCount = texture->m_TextureDesc.IsMultiBuffer ? l_swapChainImageCount : 1;
	texture->m_GPUResources.resize(frameCount);

	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		ComPtr<ID3D12Resource> defaultHeapBuffer;
		if (useClearValue)
			defaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_textureDesc, l_initialState, &l_clearValue);
		else
			defaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_textureDesc, l_initialState);

		if (!defaultHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create default heap buffer for frame ", frame);
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(texture, defaultHeapBuffer, ("DefaultHeap_Texture_Frame" + std::to_string(frame)).c_str());
#endif

		texture->m_GPUResources[frame] = defaultHeapBuffer.Get();
		m_TextureBuffers_Default[reinterpret_cast<uint64_t>(texture)].push_back(std::move(defaultHeapBuffer));
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();

	// Phase 1: Upload texture data with direct command list
	if (textureData)
	{
		CommandListComponent l_uploadCommandList = {};
		l_uploadCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12UploadCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame), L"TextureUploadCommandList");
		l_uploadCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12UploadCommandList.Get());

		auto* defaultHeapBuffer_Frame0 = static_cast<ID3D12Resource*>(texture->m_GPUResources[0]);
		uint32_t l_subresourcesCount = texture->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(defaultHeapBuffer_Frame0, 0, l_subresourcesCount);

		auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
		auto l_uploadHeapBuffer = m_ctx->CreateUploadHeapBuffer(&l_resourceDesc);
		if (!l_uploadHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create upload heap buffer for frame 0");
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(texture, l_uploadHeapBuffer, "UploadHeap_Texture");
#endif

		m_TextureBuffers_Upload[reinterpret_cast<uint64_t>(texture)] = l_uploadHeapBuffer;

		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		l_textureSubResourceData.RowPitch = texture->m_TextureDesc.Width * GetTexturePixelDataSize(texture->m_TextureDesc);
		l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * texture->m_TextureDesc.Height;
		l_textureSubResourceData.pData = (unsigned char*)textureData;

		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, l_initialState, D3D12_RESOURCE_STATE_COPY_DEST));

			UpdateSubresources(l_dx12UploadCommandList.Get(), l_defaultHeapBuffer, l_uploadHeapBuffer.Get(), 0, 0, l_subresourcesCount, &l_textureSubResourceData);

			auto l_nextState = texture->m_TextureDesc.MipLevels > 1 ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : l_initialState;
			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, D3D12_RESOURCE_STATE_COPY_DEST, l_nextState));
		}

		// Execute and wait for upload phase
		l_hwService->Close(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_uploadSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_uploadSemaphoreValue, GPUEngineType::Graphics);
	}

	// Create descriptor handles
	uint32_t mipLevels = l_textureDesc.MipLevels;
	texture->m_ReadHandles.resize(frameCount * mipLevels);
	texture->m_WriteHandles.resize(frameCount * mipLevels);
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		for (uint32_t mip = 0; mip < mipLevels; mip++)
		{
			uint32_t handleIndex = texture->GetHandleIndex(frame, mip);
			if (!CreateSRV(texture, mip))
			{
				Log(Error, texture->m_InstanceName, " Failed to create SRV for frame ", frame, " mip ", mip);
				return false;
			}
		}
	}

	if (texture->m_TextureDesc.Usage != TextureUsage::DepthAttachment
		&& texture->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment
		&& !texture->m_TextureDesc.IsSRGB)
	{
		for (uint32_t frame = 0; frame < frameCount; frame++)
		{
			for (uint32_t mip = 0; mip < mipLevels; mip++)
			{
				if (!CreateUAV(texture, mip))
				{
					Log(Error, texture->m_InstanceName, " Failed to create UAV for frame ", frame, " mip ", mip);
					return false;
				}
			}
		}
	}

	// Phase 2: Generate mipmaps with compute command list (if needed)
	if (texture->m_TextureDesc.MipLevels > 1 && textureData)
	{
		CommandListComponent l_mipmapCommandList = {};
		l_mipmapCommandList.m_Type = GPUEngineType::Compute;
		auto l_dx12MipmapCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, l_currentFrame), L"TextureMipmapCommandList");
		l_mipmapCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12MipmapCommandList.Get());

		GenerateMipmap(texture, &l_mipmapCommandList);

		l_dx12MipmapCommandList->Close();
		l_hwService->Execute(&l_mipmapCommandList, GPUEngineType::Compute);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Compute);
		auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);

		// Phase 3: Transition texture back to initial state for rendering passes
		CommandListComponent l_transitionCommandList = {};
		l_transitionCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12TransitionCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame), L"TextureTransitionCommandList");
		l_transitionCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12TransitionCommandList.Get());

		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12TransitionCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				l_initialState));
		}

		// Execute and wait for transition
		l_hwService->Close(&l_transitionCommandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_transitionCommandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_transitionSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_transitionSemaphoreValue, GPUEngineType::Graphics);
	}

	texture->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, texture->m_InstanceName, " is initialized.");

	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (ShaderProgram)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(ShaderProgramComponent* shaderProgram)
{
#ifdef USE_DXIL
	if (shaderProgram->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(shaderProgram->m_VSBuffer, shaderProgram->m_ShaderFilePaths.m_VSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(shaderProgram->m_HSBuffer, shaderProgram->m_ShaderFilePaths.m_HSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(shaderProgram->m_DSBuffer, shaderProgram->m_ShaderFilePaths.m_DSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(shaderProgram->m_GSBuffer, shaderProgram->m_ShaderFilePaths.m_GSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(shaderProgram->m_PSBuffer, shaderProgram->m_ShaderFilePaths.m_PSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(shaderProgram->m_CSBuffer, shaderProgram->m_ShaderFilePaths.m_CSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_RayGenPath != "")
	{
		LoadShaderFile(shaderProgram->m_RayGenBuffer, shaderProgram->m_ShaderFilePaths.m_RayGenPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		LoadShaderFile(shaderProgram->m_AnyHitBuffer, shaderProgram->m_ShaderFilePaths.m_AnyHitPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		LoadShaderFile(shaderProgram->m_ClosestHitBuffer, shaderProgram->m_ShaderFilePaths.m_ClosestHitPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_MissPath != "")
	{
		LoadShaderFile(shaderProgram->m_MissBuffer, shaderProgram->m_ShaderFilePaths.m_MissPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
	{
		LoadShaderFile(shaderProgram->m_ShadowMissBuffer, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath);
	}
#else
	// For non-DXIL path, we need temporary ID3DBlob storage
	ComPtr<ID3DBlob> tempBuffer;
	if (shaderProgram->m_ShaderFilePaths.m_VSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Vertex, shaderProgram->m_ShaderFilePaths.m_VSPath))
		{
			shaderProgram->m_VSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_VSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_HSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Hull, shaderProgram->m_ShaderFilePaths.m_HSPath))
		{
			shaderProgram->m_HSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_HSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_DSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Domain, shaderProgram->m_ShaderFilePaths.m_DSPath))
		{
			shaderProgram->m_DSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_DSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_GSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Geometry, shaderProgram->m_ShaderFilePaths.m_GSPath))
		{
			shaderProgram->m_GSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_GSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_PSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Pixel, shaderProgram->m_ShaderFilePaths.m_PSPath))
		{
			shaderProgram->m_PSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_PSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_CSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Compute, shaderProgram->m_ShaderFilePaths.m_CSPath))
		{
			shaderProgram->m_CSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_CSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_RayGenPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::RayGen, shaderProgram->m_ShaderFilePaths.m_RayGenPath))
		{
			shaderProgram->m_RayGenBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_RayGenBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::AnyHit, shaderProgram->m_ShaderFilePaths.m_AnyHitPath))
		{
			shaderProgram->m_AnyHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_AnyHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::ClosestHit, shaderProgram->m_ShaderFilePaths.m_ClosestHitPath))
		{
			shaderProgram->m_ClosestHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_ClosestHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_MissPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, shaderProgram->m_ShaderFilePaths.m_MissPath))
		{
			shaderProgram->m_MissBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_MissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath))
		{
			shaderProgram->m_ShadowMissBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_ShadowMissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
#endif
	shaderProgram->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (Sampler)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(SamplerComponent* sampler)
{
	sampler->m_GPUResourceType = GPUResourceType::Sampler;

	D3D12_SAMPLER_DESC l_samplerDesc = {};
	l_samplerDesc.Filter = GetFilterMode(sampler->m_SamplerDesc.m_MinFilterMethod, sampler->m_SamplerDesc.m_MagFilterMethod);
	l_samplerDesc.AddressU = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodU);
	l_samplerDesc.AddressV = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodV);
	l_samplerDesc.AddressW = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodW);
	l_samplerDesc.MipLODBias = 0.0f;
	l_samplerDesc.MaxAnisotropy = sampler->m_SamplerDesc.m_MaxAnisotropy;
	l_samplerDesc.BorderColor[0] = sampler->m_SamplerDesc.m_BorderColor[0];
	l_samplerDesc.BorderColor[1] = sampler->m_SamplerDesc.m_BorderColor[1];
	l_samplerDesc.BorderColor[2] = sampler->m_SamplerDesc.m_BorderColor[2];
	l_samplerDesc.BorderColor[3] = sampler->m_SamplerDesc.m_BorderColor[3];
	l_samplerDesc.MinLOD = sampler->m_SamplerDesc.m_MinLOD;
	l_samplerDesc.MaxLOD = sampler->m_SamplerDesc.m_MaxLOD;

	// @TODO: We don't really need multi-frame samplers
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	sampler->m_ReadHandles.resize(l_swapChainImageCount);
	for (auto& handle : sampler->m_ReadHandles)
	{
		handle = m_ctx->m_SamplerDescHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateSampler(&l_samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ handle.m_CPUHandle });
	}

	sampler->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (GPUBuffer)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(GPUBufferComponent* gpuBuffer)
{
	auto l_initialState = D3D12_RESOURCE_STATE_COMMON;
	auto l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	if (gpuBuffer->m_Usage == GPUBufferUsage::IndirectDraw)
	{
		// GPU-driven indirect draw - element size padded to 64 bytes
		gpuBuffer->m_ElementSize = 64;
		gpuBuffer->m_CPUAccessibility = Accessibility::Immutable;
		gpuBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		gpuBuffer->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	else if (l_isRaytracingAS)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
		tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		tlasInputs.NumDescs = gpuBuffer->m_ElementCount;
		tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
			| D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_ctx->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfo);

		UINT64 TLAS_SIZE_IN_BYTES = prebuildInfo.ResultDataMaxSizeInBytes;
		UINT64 SCRATCH_SIZE_IN_BYTES = prebuildInfo.ScratchDataSizeInBytes;

		gpuBuffer->m_ElementSize = gpuBuffer->m_Usage == GPUBufferUsage::TLAS ? TLAS_SIZE_IN_BYTES : SCRATCH_SIZE_IN_BYTES;
		if (gpuBuffer->m_Usage == GPUBufferUsage::TLAS)
			l_initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE; // This has to be set when the heap is created.

		gpuBuffer->m_ReadState = static_cast<uint32_t>(l_initialState);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	else
	{
		// UAV-capable buffers (ReadWrite) that also have a default heap for SRV reads must be
		// explicitly transitioned to ALL_SHADER_RESOURCE after CopyResource. They cannot rely on
		// implicit promotion from COMMON because D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		// blocks implicit promotion to SHADER_RESOURCE state.
		bool l_needDefaultHeap = gpuBuffer->m_GPUAccessibility.CanRead() && !gpuBuffer->m_CPUAccessibility.CanRead();
		if (l_needDefaultHeap)
			gpuBuffer->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		else
			gpuBuffer->m_ReadState = static_cast<uint32_t>(l_initialState);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	auto l_actualElementCount = l_isRaytracingAS ? 1 : gpuBuffer->m_ElementCount;
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	gpuBuffer->m_GPUResourceType = GPUResourceType::Buffer;
	gpuBuffer->m_TotalSize = l_actualElementCount * gpuBuffer->m_ElementSize;
	gpuBuffer->m_MappedMemories.resize(l_swapChainImageCount);
	gpuBuffer->m_DeviceMemories.resize(l_swapChainImageCount);

	// Resources are created in COMMON state. m_ReadState may differ (e.g. ALL_SHADER_RESOURCE for
	// UAV-capable buffers), so initialize m_CurrentState from l_initialState, not m_ReadState.
	gpuBuffer->m_CurrentState.resize(l_swapChainImageCount, static_cast<uint32_t>(l_initialState));

	auto l_uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize);
	bool l_needDefaultHeap = gpuBuffer->m_GPUAccessibility.CanRead() && !gpuBuffer->m_CPUAccessibility.CanRead();

	for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
	{
		auto l_mappedMemory = new DX12MappedMemory();
		l_mappedMemory->m_UploadHeapBuffer = m_ctx->CreateUploadHeapBuffer(&l_uploadBufferDesc);

		if (!l_mappedMemory->m_UploadHeapBuffer)
		{
			Log(Error, "Failed to create upload heap buffer for frame ", i);
			delete l_mappedMemory;
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(gpuBuffer, l_mappedMemory->m_UploadHeapBuffer, ("UploadHeap_" + std::to_string(i)).c_str());
#endif
		CD3DX12_RANGE m_readRange(0, 0);
		l_mappedMemory->m_UploadHeapBuffer->Map(0, &m_readRange, &l_mappedMemory->m_Address);
		gpuBuffer->m_MappedMemories[i] = l_mappedMemory;

		if (!l_needDefaultHeap)
			continue;

		auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto l_deviceMemory = new DX12DeviceMemory();
		l_deviceMemory->m_DefaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, l_initialState);

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(gpuBuffer, l_deviceMemory->m_DefaultHeapBuffer, ("DefaultHeap_" + std::to_string(i)).c_str());
#endif
		gpuBuffer->m_DeviceMemories[i] = l_deviceMemory;
	}

	if (gpuBuffer->m_InitialData)
	{
		auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

		CommandListComponent l_commandList = {};
		l_commandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12CommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame), L"GPUBufferInitCommandList");
		l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

		for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
		{
			auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(gpuBuffer->m_MappedMemories[i]);
			auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[i]);
			WriteMappedMemory(gpuBuffer, l_mappedMemory, gpuBuffer->m_InitialData, 0, gpuBuffer->m_TotalSize);
			l_mappedMemory->m_NeedUploadToGPU = false;

			// Transition to copy destination state for upload
			if (l_deviceMemory->m_DefaultHeapBuffer)
			{
				l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), l_initialState, D3D12_RESOURCE_STATE_COPY_DEST));
			}

			UploadToGPU(&l_commandList, l_mappedMemory, l_deviceMemory, gpuBuffer);

			// Transition back to initial state
			if (l_deviceMemory->m_DefaultHeapBuffer)
			{
				l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_initialState));
			}
		}

		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();
		l_hwService->Close(&l_commandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_commandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_semaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);
	}

	if (gpuBuffer->m_Usage != GPUBufferUsage::ScratchBuffer)
	{
		CreateSRV(gpuBuffer);
		if (l_needDefaultHeap)
			CreateUAV(gpuBuffer);
	}

	Log(Verbose, gpuBuffer->m_InstanceName, " (", gpuBuffer->m_Usage, ") is initialized.");
	gpuBuffer->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (Entity)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(EntityID Entity)
{
	auto* l_world = g_Engine->Get<EntityRegistry>()->Get<WorldTransformComponent>(Entity);
	Mat4 transformMatrix = l_world ? l_world->m_WorldMatrix : Mat4{};

	auto* l_mesh = g_Engine->Get<EntityRegistry>()->Get<MeshComponent>(Entity);
	if (!l_mesh)
		return true;

	auto l_handleIndex = l_mesh->m_Asset.m_Index;
	auto blasIt = m_DX12MeshResources.find(l_handleIndex);
	if (blasIt == m_DX12MeshResources.end() || !blasIt->second.m_BLAS)
		return false;

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	for (size_t frameIndex = 0; frameIndex < l_swapChainImageCount; frameIndex++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[frameIndex]);

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

		instanceDesc.Transform[0][0] = transformMatrix.m00;
		instanceDesc.Transform[0][1] = transformMatrix.m01;
		instanceDesc.Transform[0][2] = transformMatrix.m02;
		instanceDesc.Transform[0][3] = transformMatrix.m03;

		instanceDesc.Transform[1][0] = transformMatrix.m10;
		instanceDesc.Transform[1][1] = transformMatrix.m11;
		instanceDesc.Transform[1][2] = transformMatrix.m12;
		instanceDesc.Transform[1][3] = transformMatrix.m13;

		instanceDesc.Transform[2][0] = transformMatrix.m20;
		instanceDesc.Transform[2][1] = transformMatrix.m21;
		instanceDesc.Transform[2][2] = transformMatrix.m22;
		instanceDesc.Transform[2][3] = transformMatrix.m23;

		instanceDesc.InstanceID = static_cast<UINT>(l_descList->m_Descs.size());
		instanceDesc.InstanceMask = 0xFF;
		instanceDesc.InstanceContributionToHitGroupIndex = 0;
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instanceDesc.AccelerationStructure = blasIt->second.m_BLAS->GetGPUVirtualAddress();

		l_descList->m_Descs.emplace_back(instanceDesc);
		l_descList->m_NeedFullUpdate = true;
	}

	Log(Verbose, "Entity ", Entity, " raytracing instance registered.");
	m_initializedEntities.emplace(Entity);

	return true;
}

// ---------------------------------------------------------------------------
// Protected: InitializeImpl (CommandList)
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::InitializeImpl(CommandListComponent* commandList)
{
	if (!commandList)
	{
		Log(Error, "CommandList parameter is null");
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	HRESULT l_HResult;

	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Compute:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Copy:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	default:
		Log(Error, commandList->m_InstanceName, " Unknown GPU engine type for command list creation");
		return false;
	}

	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to create DX12 command list for command list creation");
		return false;
	}

	// Close the command list immediately after creation (DX12 requirement)
	l_HResult = l_commandList->Close();
	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to close command list after creation");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(commandList, l_commandList, "CommandList");
#endif

	commandList->m_CommandList = reinterpret_cast<uint64_t>(l_commandList.Detach());
	commandList->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, commandList->m_InstanceName, " Command list created successfully");
	return true;
}

// ---------------------------------------------------------------------------
// Upload / Transfer / Clear / Copy
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::UploadToGPU(CommandListComponent* commandList, TextureComponent* texture)
{
	// Texture upload is handled during initialization with centralized resources
	// This function is kept for interface compatibility
	return true;
}

bool DX12GraphicsResourceService::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_mesh = reinterpret_cast<GPUBufferComponent*>(gpuBuffer);
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_mesh->m_MappedMemories[l_currentFrame]);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_mesh->m_DeviceMemories[l_currentFrame]);
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	return UploadToGPU(commandList, l_mappedMemory, l_deviceMemory, l_mesh);
}

bool DX12GraphicsResourceService::UploadToGPU(CommandListComponent* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* GPUBufferComponent)
{
	if (!deviceMemory->m_DefaultHeapBuffer)
		return true;

	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	l_DX12CommandList->CopyResource(deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get());

	return true;
}

bool DX12GraphicsResourceService::Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	const uint32_t zero = 0;
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[l_currentFrame]);
	l_commandList->ClearUnorderedAccessViewUint(
		D3D12_GPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_GPUHandle },
		D3D12_CPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_CPUHandle },
		l_deviceMemory->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	return true;
}

bool DX12GraphicsResourceService::Copy(CommandListComponent* commandList, TextureComponent* sourceTexture, TextureComponent* destinationTexture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto* srcResource = static_cast<ID3D12Resource*>(sourceTexture->GetGPUResource(frameIndex));
	auto* destResource = static_cast<ID3D12Resource*>(destinationTexture->GetGPUResource(frameIndex));

	if (!srcResource || !destResource)
	{
		Log(Error, "Cannot find texture resources for copy operation");
		return false;
	}

	l_commandList->CopyResource(destResource, srcResource);

	return true;
}

bool DX12GraphicsResourceService::Clear(CommandListComponent* commandList, TextureComponent* texture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));
	if (!resource)
	{
		Log(Error, "Cannot find texture resource for clear operation");
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	uint32_t handleIndex = texture->GetHandleIndex(frameIndex, 0);
	if (texture->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
	{
		l_commandList->ClearUnorderedAccessViewUint(
			D3D12_GPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_GPUHandle },
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_CPUHandle },
			resource,
			(UINT*)&texture->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}
	else
	{
		l_commandList->ClearUnorderedAccessViewFloat(
			D3D12_GPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_GPUHandle },
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_CPUHandle },
			resource,
			&texture->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Public: Query
// ---------------------------------------------------------------------------

std::optional<uint32_t> DX12GraphicsResourceService::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility)
{
    if (!texture)
        return std::nullopt;

    if (texture->m_ObjectStatus != ObjectStatus::Activated)
        return std::nullopt;

    // Use proper handle index based on IsMultiBuffer flag
    auto l_handleIndex = texture->m_TextureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0;

    if (bindingAccessibility == Accessibility::ReadOnly)
    {
        if (l_handleIndex < texture->m_ReadHandles.size())
            return texture->m_ReadHandles[l_handleIndex].m_Index;
    }
    else if (bindingAccessibility.CanWrite())
    {
        if (l_handleIndex < texture->m_WriteHandles.size())
            return texture->m_WriteHandles[l_handleIndex].m_Index;
    }

    return std::nullopt;
}

Vec4 DX12GraphicsResourceService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
    return Vec4();
}

std::vector<Vec4> DX12GraphicsResourceService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
    auto textureDesc = TextureComp->m_TextureDesc;
    auto l_frameIndex = textureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0;

    if (l_frameIndex >= TextureComp->m_GPUResources.size())
    {
        Log(Error, TextureComp, " frame index ", l_frameIndex, " out of bounds (size: ", TextureComp->m_GPUResources.size(), ")");
        return {};
    }

    auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(TextureComp->m_GPUResources[l_frameIndex]);
    if (!l_defaultHeapBuffer)
    {
        Log(Error, TextureComp, " has null GPU resource at frame index ", l_frameIndex);
        return {};
    }

    auto l_srcDesc = l_defaultHeapBuffer->GetDesc();

    uint32_t l_subresourceCount = textureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : textureDesc.DepthOrArraySize;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints(l_subresourceCount);
    m_ctx->m_device->GetCopyableFootprints(&l_srcDesc, 0, l_subresourceCount, 0, l_footprints.data(), NULL, NULL, NULL);

    UINT64 l_bufferSize = 0;
    for (uint32_t i = 0; i < l_subresourceCount; ++i)
        l_bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;

    auto l_readBackHeapBuffer = m_ctx->CreateReadBackHeapBuffer(l_bufferSize);
    if (!l_readBackHeapBuffer)
    {
        Log(Error, TextureComp, " failed to create readback heap buffer");
        return {};
    }

    DXGI_FORMAT l_format = DX12Helper::GetTextureFormat(textureDesc);

    {
        auto l_beforeState = static_cast<D3D12_RESOURCE_STATES>(TextureComp->GetCurrentState(l_frameIndex));
        // Use a dedicated allocator so this temporary CL does not share the global
        // per-frame allocator, which has already been used by PrepareGlobalCommands.
        // Sharing would leave the allocator in a state that makes the next frame's
        // Open() (Reset) fail silently, causing EXECUTECOMMANDLISTS_FAILEDCOMMANDLIST.
        ComPtr<ID3D12CommandAllocator> l_tempAllocator;
        auto l_allocResult = m_ctx->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_tempAllocator));
        if (FAILED(l_allocResult))
        {
            Log(Error, TextureComp, " failed to create temporary command allocator for readback");
            return {};
        }
        auto l_dx12CommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_tempAllocator, L"ReadTextureBackToCPU_Transition");
        l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer, l_beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE));

        for (uint32_t i = 0; i < l_subresourceCount; ++i)
        {
            D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
            l_srcLocation.pResource = l_defaultHeapBuffer;
            l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            l_srcLocation.SubresourceIndex = i;

            D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
            l_destLocation.pResource = l_readBackHeapBuffer.Get();
            l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            l_destLocation.PlacedFootprint = l_footprints[i];
            l_destLocation.PlacedFootprint.Footprint.Format = l_format;

            l_dx12CommandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);
        }

        l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, l_beforeState));
        l_dx12CommandList->Close();

        auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
        auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();

        CommandListComponent l_commandListComp = {};
        l_commandListComp.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());
        l_hwService->Execute(&l_commandListComp, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
        l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    }

    size_t l_pixelCount = 0;
    switch (textureDesc.Sampler)
    {
    case TextureSampler::Sampler1D:
        l_pixelCount = textureDesc.Width;
        break;
    case TextureSampler::Sampler2D:
        l_pixelCount = textureDesc.Width * textureDesc.Height;
        break;
    case TextureSampler::Sampler3D:
        l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::Sampler1DArray:
        l_pixelCount = textureDesc.Width * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::Sampler2DArray:
        l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::SamplerCubemap:
        l_pixelCount = textureDesc.Width * textureDesc.Height * 6;
        break;
    default:
        break;
    }

    if (l_pixelCount == 0)
    {
        Log(Error, TextureComp, " unsupported sampler type for readback: ", (uint32_t)textureDesc.Sampler);
        return {};
    }

    uint32_t l_pixelDataSize = DX12Helper::GetTexturePixelDataSize(textureDesc);
    std::vector<unsigned char> l_rawResult(l_bufferSize);

    CD3DX12_RANGE l_readRange(0, l_rawResult.size());
    void* l_pData = nullptr;
    auto l_hResult = l_readBackHeapBuffer->Map(0, &l_readRange, &l_pData);
    if (FAILED(l_hResult))
    {
        Log(Error, TextureComp, " failed to map readback heap buffer");
        return {};
    }
    std::memcpy(l_rawResult.data(), l_pData, l_rawResult.size());
    l_readBackHeapBuffer->Unmap(0, nullptr);
    std::vector<Vec4> l_result(l_pixelCount);
    size_t l_subresourceOffset = 0;
    for (uint32_t sub = 0; sub < l_subresourceCount; ++sub)
    {
        uint32_t l_rowPitch  = l_footprints[sub].Footprint.RowPitch;
        uint32_t l_subWidth  = l_footprints[sub].Footprint.Width;
        uint32_t l_subHeight = l_footprints[sub].Footprint.Height;
        uint32_t l_subDepth  = l_footprints[sub].Footprint.Depth;

        for (uint32_t z = 0; z < l_subDepth; ++z)
        {
            for (uint32_t row = 0; row < l_subHeight; ++row)
            {
                const unsigned char* l_srcRow = l_rawResult.data() + l_subresourceOffset + (z * l_subHeight + row) * l_rowPitch;
                for (uint32_t col = 0; col < l_subWidth; ++col)
                {
                    const unsigned char* l_PixelData = l_srcRow + col * l_pixelDataSize;

                    size_t l_dstIndex = 0;
                    switch (textureDesc.Sampler)
                    {
                    case TextureSampler::Sampler1D:
                        l_dstIndex = col;
                        break;
                    case TextureSampler::Sampler2D:
                        l_dstIndex = row * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler3D:
                        l_dstIndex = (z * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler1DArray:
                        l_dstIndex = sub * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler2DArray:
                        l_dstIndex = (sub * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    case TextureSampler::SamplerCubemap:
                        l_dstIndex = (sub * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    default:
                        break;
                    }

                    if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
                    {
                        float r, g, b, a;
                        memcpy(&r, l_PixelData + 0,  4);
                        memcpy(&g, l_PixelData + 4,  4);
                        memcpy(&b, l_PixelData + 8,  4);
                        memcpy(&a, l_PixelData + 12, 4);
                        l_result[l_dstIndex] = Vec4(r, g, b, a);
                    }
                    else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
                    {
                        uint32_t channels = l_pixelDataSize / 2;
                        float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                        for (uint32_t ch = 0; ch < channels && ch < 4; ++ch)
                        {
                            uint16_t h;
                            memcpy(&h, l_PixelData + ch * 2, 2);
                            values[ch] = Math::float16ToFloat32(h);
                        }
                        l_result[l_dstIndex] = Vec4(values[0], values[1], values[2], values[3]);
                    }
                    else
                    {
                        l_result[l_dstIndex] = Vec4(l_PixelData[0] / 255.0f,
                                                    l_PixelData[1] / 255.0f,
                                                    l_PixelData[2] / 255.0f,
                                                    l_PixelData[3] / 255.0f);
                    }
                }
            }
        }
        l_subresourceOffset += l_rowPitch * l_subHeight * l_subDepth;
    }

    return l_result;
}

// ---------------------------------------------------------------------------
// Public: GenerateMipmap
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList)
{
    if (!commandList)
    {
        Log(Error, "GenerateMipmap requires a valid command list for proper synchronization");
        return false;
    }

    // Skip SRGB textures for now due to complexity - focus on core functionality
    if(texture->m_TextureDesc.IsSRGB)
    {
        Log(Warning, "SRGB mipmap generation not currently supported, skipping texture");
        return true;
    }

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}

        void operator=(FLOAT f) { Float = f; }
        void operator=(UINT u) { Uint = u; }

        union
        {
            FLOAT Float;
            UINT Uint;
        };
    };

    if (texture->m_TextureDesc.MipLevels == 1)
    {
        Log(Warning, texture->m_InstanceName, " Attempt to generate mipmaps for texture without mipmaps requirement.");
        return false;
    }


    // Determine if this is a static texture (Sample) or render target (attachment or compute usage)
    bool isStaticTexture = (texture->m_TextureDesc.Usage == TextureUsage::Sample);
    bool isRenderTarget = (texture->m_TextureDesc.Usage == TextureUsage::ColorAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::ComputeOnly);

    // For static textures: generate mipmaps for all device memories
    // For render targets: generate mipmaps only for current frame's device memory
    size_t startIndex = 0;
    size_t endIndex = 1;

    auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

    if (isStaticTexture && texture->m_TextureDesc.IsMultiBuffer)
    {
        // Static textures with multi-buffer: generate for all buffers
        endIndex = texture->m_ReadHandles.size();
    }
    else if (isRenderTarget && texture->m_TextureDesc.IsMultiBuffer)
    {
        // Render targets with multi-buffer: generate only for current frame
        startIndex = l_currentFrame;
        endIndex = startIndex + 1;
    }
    else
    {
        // Single buffer textures: always use index 0
        startIndex = 0;
        endIndex = 1;
    }

    auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
    if (!l_DX12CommandList)
    {
        Log(Error, texture->m_InstanceName, " Invalid command list");
        return false;
    }

    // Set pipeline state based on texture type
    if (texture->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
    {
        l_DX12CommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
        l_DX12CommandList->SetPipelineState(m_3DMipmapPSO);
    }
    else
    {
        l_DX12CommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
        l_DX12CommandList->SetPipelineState(m_2DMipmapPSO);
    }

    // Set descriptor heaps
    ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
    l_DX12CommandList->SetDescriptorHeaps(1, l_heaps);

    uint32_t l_mipLevels = texture->m_TextureDesc.MipLevels;

    // Process mipmap generation for each device memory on compute command list
    for (size_t deviceMemoryIndex = startIndex; deviceMemoryIndex < endIndex; deviceMemoryIndex++)
    {
        auto l_defaultHeapBuffer = reinterpret_cast<ID3D12Resource*>(texture->m_GPUResources[deviceMemoryIndex]);
        if (!l_defaultHeapBuffer)
        {
            Log(Error, texture->m_InstanceName, " Invalid device memory at index ", deviceMemoryIndex);
            return false;
        }

        for (uint32_t mipLevel = 0; mipLevel < l_mipLevels - 1; mipLevel++)
        {
            uint32_t dstWidth = std::max(texture->m_TextureDesc.Width >> (mipLevel + 1), 1u);
            uint32_t dstHeight = std::max(texture->m_TextureDesc.Height >> (mipLevel + 1), 1u);
            uint32_t dstDepth = 1;

            // Set texel size constants (1.0 / dstSize)
            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

            if (texture->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
            {
                dstDepth = std::max(texture->m_TextureDesc.DepthOrArraySize >> (mipLevel + 1), 1u);
                l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
            }

            // Bind both UAVs - source and destination
            // Source: mipLevel (read from current mip via UAV)
            // Destination: mipLevel + 1 (write to next smaller mip via UAV)
            auto l_srcHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel);
            auto l_srcUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_srcHandleIndex].m_GPUHandle };

            auto l_dstHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel + 1);
            auto l_dstUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_dstHandleIndex].m_GPUHandle };

            l_DX12CommandList->SetComputeRootDescriptorTable(1, l_srcUAV);
            l_DX12CommandList->SetComputeRootDescriptorTable(2, l_dstUAV);

            // Dispatch compute shader
            uint32_t dispatchX = std::max(dstWidth / 8, 1u);
            uint32_t dispatchY = std::max(dstHeight / 8, 1u);
            uint32_t dispatchZ = std::max(dstDepth / 8, 1u);

            l_DX12CommandList->Dispatch(dispatchX, dispatchY, dispatchZ);

            // UAV barrier between mip levels to ensure writes complete
            auto l_uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_defaultHeapBuffer);
            l_DX12CommandList->ResourceBarrier(1, &l_uavBarrier);
        }
    }

    auto memoryCount = endIndex - startIndex;
    Log(Verbose, texture->m_InstanceName, " Successfully recorded mipmap generation commands for ", l_mipLevels, " mip levels for ", memoryCount, " device memory/memories");
    return true;
}

// ---------------------------------------------------------------------------
// Private: SRV / UAV / CBV creation
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::CreateSRV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetSRVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);
	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadOnly, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);

	uint32_t frameCount = texture->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(texture->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, texture->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = texture->GetHandleIndex(frame, mipSlice);
		texture->m_ReadHandles[handleIndex] = l_descHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateShaderResourceView(resource,
			&l_desc,
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_ReadHandles[handleIndex].m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GraphicsResourceService::CreateUAV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetUAVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage, false);

	uint32_t frameCount = texture->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(texture->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, texture->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = texture->GetHandleIndex(frame, mipSlice);

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		texture->m_WriteHandles[handleIndex].m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		texture->m_WriteHandles[handleIndex].m_GPUHandle = l_descHandle.m_GPUHandle;
		texture->m_WriteHandles[handleIndex].m_Index = l_descHandle.m_Index;

		m_ctx->m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_ctx->m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GraphicsResourceService::CreateSRV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_DX12DeviceMemory->m_SRV.Handle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_DX12DeviceMemory->m_SRV.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GraphicsResourceService::CreateUAV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite);
	auto& l_descHeapAccessor_ShaderNonVisible = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::Invalid, false);

	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		DX12UAV l_result = {};
		l_result.UAVDesc = l_desc;

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		l_result.Handle.m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		l_result.Handle.m_GPUHandle = l_descHandle.m_GPUHandle;
		l_result.Handle.m_Index = l_descHandle.m_Index;

		m_ctx->m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_ctx->m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });

		l_DX12DeviceMemory->m_UAV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GraphicsResourceService::CreateCBV(GPUBufferComponent* gpuBuffer)
{
	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadOnly);

	for (auto i : gpuBuffer->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		DX12CBV l_result;

		l_result.CBVDesc.BufferLocation = l_DX12MappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
		l_result.CBVDesc.SizeInBytes = (uint32_t)gpuBuffer->m_ElementSize;
		l_result.Handle = l_descHeapAccessor.GetNewHandle();

		m_ctx->m_device->CreateConstantBufferView(&l_result.CBVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_result.Handle.m_CPUHandle });

		l_DX12MappedMemory->m_CBV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

// ---------------------------------------------------------------------------
// Private: RootSignature / DescriptorRange
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::CreateRootSignature(RenderPassComponent* RenderPassComp)
{
	if (RenderPassComp->m_ResourceBindingLayoutDescs.empty())
		Log(Verbose, "Creating empty RootSignature for ", RenderPassComp->m_InstanceName);

	auto l_maxBindingCount = RenderPassComp->m_ResourceBindingLayoutDescs.size();
	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters;
	l_rootParameters.reserve(l_maxBindingCount);

	std::vector<D3D12_DESCRIPTOR_RANGE1> l_descriptorRanges;
	l_descriptorRanges.reserve(l_maxBindingCount);

	for (size_t i = 0; i < l_maxBindingCount; i++)
	{
		auto& l_resourceBinderLayoutDesc = RenderPassComp->m_ResourceBindingLayoutDescs[i];
		auto l_descriptorRange = GetDescriptorRange(RenderPassComp, l_resourceBinderLayoutDesc);
		CD3DX12_ROOT_PARAMETER1 l_rootParameter = {};
		if (l_descriptorRange.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			if (l_resourceBinderLayoutDesc.m_IsRootConstant)
			{
				Log(Verbose, RenderPassComp->m_InstanceName, " Root Constant: at root parameter ", i,
					" with ", l_resourceBinderLayoutDesc.m_SubresourceCount, " constants.");
				l_rootParameter.InitAsConstants(l_resourceBinderLayoutDesc.m_SubresourceCount, l_resourceBinderLayoutDesc.m_DescriptorIndex);
			}
			else
			{
				Log(Verbose, RenderPassComp->m_InstanceName, " Root CBV: at root parameter ", i, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
				l_rootParameter.InitAsConstantBufferView(l_resourceBinderLayoutDesc.m_DescriptorIndex);
			}
		}
		else
		{
			const char* rangeTypeName = "";
			auto& l_lastRange = l_descriptorRanges.emplace_back(l_descriptorRange);
			switch (l_descriptorRange.RangeType)
			{
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table CBV";
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
			{
				if (l_resourceBinderLayoutDesc.m_GPUBufferUsage == GPUBufferUsage::TLAS)
				{
					l_rootParameter.InitAsShaderResourceView(l_resourceBinderLayoutDesc.m_DescriptorIndex);
					rangeTypeName = "Root Shader Resource View";
				}
				else
				{
					l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
					rangeTypeName = "Root Descriptor Table SRV";
				}
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table UAV";
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table Sampler";
				break;
			}
			}
			Log(Verbose, RenderPassComp->m_InstanceName, ": ", rangeTypeName, " at root parameter ", i,
				" BaseShaderRegister ", l_descriptorRange.BaseShaderRegister, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
		}

		l_rootParameters.emplace_back(l_rootParameter);
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc((uint32_t)l_rootParameters.size(), l_rootParameters.data());

	if (RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics && RenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
	{
		l_rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	ComPtr<ID3DBlob> l_signature = 0;
	ComPtr<ID3DBlob> l_error = 0;

	auto l_HResult = D3D12SerializeVersionedRootSignature(&l_rootSigDesc, &l_signature, &l_error);

	if (FAILED(l_HResult))
	{
		if (l_error)
		{
			auto l_errorMessagePtr = (char*)(l_error->GetBufferPointer());
			auto bufferSize = l_error->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);
			l_error->Release();

			Log(Error, RenderPassComp->m_InstanceName, " RootSignature serialization error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't serialize RootSignature.");
		}
		return false;
	}

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	l_HResult = m_ctx->m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&l_PSO->m_RootSignature));

	if (FAILED(l_HResult))
	{
		Log(Error, RenderPassComp->m_InstanceName, " Can't create RootSignature.");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(RenderPassComp, l_PSO->m_RootSignature, "RootSignature");
#endif // INNO_DEBUG

	Log(Verbose, RenderPassComp->m_InstanceName, " RootSignature has been created.");

	if (RenderPassComp->m_RenderPassDesc.m_IndirectDraw)
	{
		D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[4] = {};

		// 1. Constant argument (must be first to match HLSL structure).
		argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		argumentDescs[0].Constant.RootParameterIndex = 0; // The root signature slot for the constant.
		argumentDescs[0].Constant.DestOffsetIn32BitValues = 0; // Start at offset 0.
		argumentDescs[0].Constant.Num32BitValuesToSet = 2; // Number of 32-bit values. Because of the 8-byte alignment requirement, we put two 32-bit values here.

		// 2. Vertex buffer view.
		argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;

		// 3. Index buffer view.
		argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

		// 4. Draw indexed arguments (must be last).
		argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
		commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
		commandSignatureDesc.pArgumentDescs = argumentDescs;
		commandSignatureDesc.ByteStride = 64;

		l_HResult = m_ctx->m_device->CreateCommandSignature(&commandSignatureDesc, l_PSO->m_RootSignature.Get(), IID_PPV_ARGS(&l_PSO->m_IndirectCommandSignature));
		if (FAILED(l_HResult))
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create CommandSignature.");
			return false;
		}

		Log(Verbose, RenderPassComp->m_InstanceName, " CommandSignature has been created.");
	}

	return true;
}

D3D12_DESCRIPTOR_RANGE1 DX12GraphicsResourceService::GetDescriptorRange(RenderPassComponent* RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
{
	auto& l_descriptorAccessor = m_ctx->GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
		, resourceBinderLayoutDesc.m_ResourceAccessibility, resourceBinderLayoutDesc.m_TextureUsage);

	D3D12_DESCRIPTOR_RANGE1 l_range = {};
	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	}

	if (resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
			{
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			}
			else if (resourceBinderLayoutDesc.m_ResourceAccessibility.CanWrite())
			{
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			}
		}
		else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
	}
	else if (resourceBinderLayoutDesc.m_BindingAccessibility.CanWrite())
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		{
			l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
		else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
		{
			l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
	}

	l_range.BaseShaderRegister = resourceBinderLayoutDesc.m_DescriptorIndex;

	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		l_range.NumDescriptors = 1;
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::Sample)
			l_range.NumDescriptors = l_descriptorAccessor.GetDesc().m_MaxDescriptors;
		else if (resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::ComputeOnly)
			l_range.NumDescriptors = 1;
	}
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
		l_range.NumDescriptors = 1;

	return l_range;
}

// ---------------------------------------------------------------------------
// Protected: Render pass initialization
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::CreatePipelineStateObject(RenderPassComponent* renderPass)
{
	bool l_result = true;
	l_result &= CreateRootSignature(renderPass);

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
		{
			l_result &= CreateGraphicsPipelineStateObject(renderPass, l_PSO);
		}
	}
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute && !renderPass->m_RenderPassDesc.m_UseRaytracing)
	{
		LoadComputeShaders(renderPass);

		l_PSO->m_ComputePSODesc.pRootSignature = l_PSO->m_RootSignature.Get();
		auto l_HResult = m_ctx->m_device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, renderPass->m_InstanceName, " Can't create Compute PSO.");
			return false;
		}
	}

	if (l_PSO->m_PSO)
	{
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(renderPass, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG
		Log(Verbose, renderPass->m_InstanceName, " PSO has been created.");
	}

	if (renderPass->m_RenderPassDesc.m_UseRaytracing)
	{
		l_result &= CreateRaytracingPipelineStateObject(renderPass, l_PSO);
	}

	return l_result;
}

bool DX12GraphicsResourceService::CreateGraphicsPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
	GenerateDepthStencilStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, PSO);
	GenerateBlendStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, PSO);
	GenerateRasterizerStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, PSO);
	GenerateViewportStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, PSO);

	PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

	auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
	auto l_RTV = l_DX12OutputMergerTarget->m_RTVs[0];
	for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		PSO->m_GraphicsPSODesc.RTVFormats[i] = l_RTV.m_Desc.Format;
	}

	if (RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
		auto l_DSV = l_DX12OutputMergerTarget->m_DSVs[0];
		PSO->m_GraphicsPSODesc.DSVFormat = l_DSV.m_Desc.Format;
		PSO->m_GraphicsPSODesc.DepthStencilState = PSO->m_DepthStencilDesc;
	}

	PSO->m_GraphicsPSODesc.RasterizerState = PSO->m_RasterizerDesc;
	PSO->m_GraphicsPSODesc.BlendState = PSO->m_BlendDesc;
	PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
	PSO->m_GraphicsPSODesc.PrimitiveTopologyType = PSO->m_PrimitiveTopologyType;
	PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;
	if (!PSO->m_RootSignature.Get() || !RenderPassComp->m_ShaderProgram)
	{
		Log(Verbose, "Skipping creating Graphics PSO for ", RenderPassComp->m_InstanceName);
		return true;
	}

	PSO->m_GraphicsPSODesc.pRootSignature = PSO->m_RootSignature.Get();

	CreateInputLayout(PSO);
	LoadGraphicsShaders(RenderPassComp);

	auto l_HResult = m_ctx->m_device->CreateGraphicsPipelineState(&PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&PSO->m_PSO));
	if (FAILED(l_HResult))
	{
		Log(Error, RenderPassComp->m_InstanceName, " Can't create Graphics PSO.");
		return false;
	}

	return true;
}

bool DX12GraphicsResourceService::CreateRaytracingPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
	auto l_SPC = RenderPassComp->m_ShaderProgram;

	if (!PSO->m_RootSignature)
	{
		Log(Error, RenderPassComp->m_InstanceName, " Global root signature is null!");
		return false;
	}

	LoadRaytracingShaders(RenderPassComp);

	const bool hasShadowMiss = !l_SPC->m_ShadowMissBuffer.empty();

	D3D12_DXIL_LIBRARY_DESC rayGenLib = {};
	rayGenLib.DXILLibrary.pShaderBytecode = &l_SPC->m_RayGenBuffer[0];
	rayGenLib.DXILLibrary.BytecodeLength = l_SPC->m_RayGenBuffer.size();

	D3D12_DXIL_LIBRARY_DESC closestHitLib = {};
	closestHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ClosestHitBuffer[0];
	closestHitLib.DXILLibrary.BytecodeLength = l_SPC->m_ClosestHitBuffer.size();

	D3D12_DXIL_LIBRARY_DESC anyHitLib = {};
	anyHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_AnyHitBuffer[0];
	anyHitLib.DXILLibrary.BytecodeLength = l_SPC->m_AnyHitBuffer.size();

	D3D12_DXIL_LIBRARY_DESC missLib = {};
	missLib.DXILLibrary.pShaderBytecode = &l_SPC->m_MissBuffer[0];
	missLib.DXILLibrary.BytecodeLength = l_SPC->m_MissBuffer.size();

	D3D12_DXIL_LIBRARY_DESC shadowMissLib = {};
	if (hasShadowMiss)
	{
		shadowMissLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ShadowMissBuffer[0];
		shadowMissLib.DXILLibrary.BytecodeLength = l_SPC->m_ShadowMissBuffer.size();
	}

	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
	hitGroupDesc.AnyHitShaderImport = L"AnyHitShader";
	hitGroupDesc.IntersectionShaderImport = nullptr;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes = 48;  // PathTracerPayload: 44B; ShadowPayload: 4B
	shaderConfig.MaxAttributeSizeInBytes = 8; // barycentrics

	D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { PSO->m_RootSignature.Get() };

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
	pipelineCfg.MaxTraceRecursionDepth = 1;

	// Up to 9 subobjects: RayGen + ClosestHit + AnyHit + Miss + (opt ShadowMiss) + HitGroup + ShaderConfig + GlobalRS + PipelineCfg
	D3D12_STATE_SUBOBJECT subobjects[9] = {};
	uint32_t subIdx = 0;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &rayGenLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &closestHitLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &anyHitLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &missLib;

	if (hasShadowMiss)
	{
		subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		subobjects[subIdx++].pDesc = &shadowMissLib;
	}

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[subIdx++].pDesc = &hitGroupDesc;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobjects[subIdx++].pDesc = &shaderConfig;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[subIdx++].pDesc = &globalSig;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[subIdx++].pDesc = &pipelineCfg;

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = subIdx;
	stateObjectDesc.pSubobjects = subobjects;

	HRESULT l_HResult = m_ctx->m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&PSO->m_RaytracingPSO));
	if (FAILED(l_HResult))
	{
		Log(Error, RenderPassComp->m_InstanceName, " Can't create Raytracing PSO.");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(RenderPassComp, PSO->m_RaytracingPSO, "RaytracingPSO");
#endif

	Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing PSO has been created.");

	// Shader table layout:
	//   hasShadowMiss == false: [RayGen][Miss][HitGroup]             (3 slots)
	//   hasShadowMiss == true:  [RayGen][Miss][ShadowMiss][HitGroup] (4 slots)
	const uint32_t numSlots = hasShadowMiss ? 4 : 3;
	auto l_shaderIDBufferSize = numSlots * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	auto l_shaderIDBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(l_shaderIDBufferSize);
	PSO->m_RaytracingShaderIDBuffer = m_ctx->CreateUploadHeapBuffer(&l_shaderIDBufferDesc);

	ID3D12StateObjectProperties* props;
	PSO->m_RaytracingPSO->QueryInterface(&props);

	void* data;
	auto writeId = [&](const wchar_t* name) {
		void* id = props->GetShaderIdentifier(name);
		memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data = static_cast<char*>(data) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	};

	PSO->m_RaytracingShaderIDBuffer->Map(0, nullptr, &data);
	writeId(L"RayGenShader");
	writeId(L"MissShader");
	if (hasShadowMiss)
		writeId(L"ShadowMissShader");
	writeId(L"HitGroup");
	PSO->m_RaytracingShaderIDBuffer->Unmap(0, nullptr);
	props->Release();

	Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing shader IDs have been written.");

	return true;
}

bool DX12GraphicsResourceService::CreateFenceEvents(RenderPassComponent* renderPass)
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

bool DX12GraphicsResourceService::OnOutputMergerTargetsCreated(RenderPassComponent* renderPass)
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

bool DX12GraphicsResourceService::OnSceneLoadingStart()
{
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
		l_descList->m_Descs.clear();
	}

	m_initializedEntities.clear();
	m_TLASReady = false;

	Log(Verbose, "Raytracing instance descriptions have been cleared.");

	return true;
}

// ---------------------------------------------------------------------------
// DX12-specific hardware-resource init
// ---------------------------------------------------------------------------

bool DX12GraphicsResourceService::CreateMipmapGenerator()
{
	{
		CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
		uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
		rootParameters[0].InitAsConstants(3, 0);
		rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

		ID3DBlob* signature;
		ID3DBlob* error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_ctx->m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_3DMipmapRootSignature));

		ShaderFilePath l_3DPath = "mipmapGenerator3D.comp/";

		D3D12_COMPUTE_PIPELINE_STATE_DESC l_3DPSODesc = {};
		l_3DPSODesc.pRootSignature = m_3DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<uint8_t> l_3DmipmapComputeShader;
		LoadShaderFile(l_3DmipmapComputeShader, l_3DPath);
		l_3DPSODesc.CS = { l_3DmipmapComputeShader.data(), l_3DmipmapComputeShader.size() };
#else
		ID3DBlob* l_3DmipmapComputeShader;
		LoadShaderFile(&l_3DmipmapComputeShader, ShaderStage::Compute, l_3DPath);
		l_3DPSODesc.CS = { reinterpret_cast<UINT8*>(l_3DmipmapComputeShader->GetBufferPointer()), l_3DmipmapComputeShader->GetBufferSize() };
#endif
		m_ctx->m_device->CreateComputePipelineState(&l_3DPSODesc, IID_PPV_ARGS(&m_3DMipmapPSO));

		Log(Success, "Mipmap generator for 3D texture has been created.");
	}
	{
		CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
		uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
		rootParameters[0].InitAsConstants(2, 0);
		rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

		ID3DBlob* signature;
		ID3DBlob* error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_ctx->m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_2DMipmapRootSignature));

		ShaderFilePath l_2DPath = "mipmapGenerator2D.comp/";
		D3D12_COMPUTE_PIPELINE_STATE_DESC l_2DPSODesc = {};
		l_2DPSODesc.pRootSignature = m_2DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<uint8_t> l_2DmipmapComputeShader;
		LoadShaderFile(l_2DmipmapComputeShader, l_2DPath);
		l_2DPSODesc.CS = { l_2DmipmapComputeShader.data(), l_2DmipmapComputeShader.size() };
#else
		ID3DBlob* l_2DmipmapComputeShader;
		LoadShaderFile(&l_2DmipmapComputeShader, ShaderStage::Compute, l_2DPath);
		l_2DPSODesc.CS = { reinterpret_cast<UINT8*>(l_2DmipmapComputeShader->GetBufferPointer()), l_2DmipmapComputeShader->GetBufferSize() };
#endif
		m_ctx->m_device->CreateComputePipelineState(&l_2DPSODesc, IID_PPV_ARGS(&m_2DMipmapPSO));

		Log(Success, "Mipmap generator for 2D texture has been created.");
	}

	return true;
}

bool DX12GraphicsResourceService::CreateRaytracingResources()
{
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	m_TLASBufferComponent = AddGPUBufferComponent("TLASBuffer/");
	m_TLASBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	// @TODO: The number of elements should be calculated based on the number of ray tracing instances.
	m_TLASBufferComponent->m_Usage = GPUBufferUsage::TLAS;
	m_TLASBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_TLASBufferComponent);

	m_ScratchBufferComponent = AddGPUBufferComponent("ScratchBuffer/");
	m_ScratchBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_ScratchBufferComponent->m_Usage = GPUBufferUsage::ScratchBuffer;
	m_ScratchBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_ScratchBufferComponent);

	m_RaytracingInstanceBufferComponent = AddGPUBufferComponent("RaytracingInstanceBuffer/");
	m_RaytracingInstanceBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RaytracingInstanceBufferComponent->m_ElementCount = 256;
	m_RaytracingInstanceBufferComponent->m_ElementSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

	InitializeImpl(m_RaytracingInstanceBufferComponent);

	m_RaytracingInstanceDescs.resize(l_swapChainImageCount);
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = new DX12RaytracingInstanceDescList();
		l_descList->m_Descs.reserve(m_RaytracingInstanceBufferComponent->m_ElementCount);
		m_RaytracingInstanceDescs[i] = l_descList;
	}

	return true;
}

bool DX12GraphicsResourceService::ReleaseRaytracingResources()
{
	Delete(m_RaytracingInstanceBufferComponent);
	Delete(m_ScratchBufferComponent);
	Delete(m_TLASBufferComponent);

	return true;
}

bool DX12GraphicsResourceService::ReleaseMipmapGenerator()
{
	m_3DMipmapPSO->Release();
	m_2DMipmapPSO->Release();
	m_3DMipmapRootSignature->Release();
	m_2DMipmapRootSignature->Release();

	return true;
}
