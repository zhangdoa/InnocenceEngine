#include "DX12RenderingServer.h"
#include <d3d12.h>
#include <dxgiformat.h>

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

#include "../../Engine.h"
#include "../../Services/PhysicsSimulationService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::InitializeImpl(MeshComponent* rhs, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto componentUUID = rhs->m_UUID;

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);

	auto l_defaultHeapBuffer_VB = CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (!l_defaultHeapBuffer_VB)
	{
		Log(Error, "can't create vertex buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_defaultHeapBuffer_VB, "DefaultHeap_VB");
#endif //  INNO_DEBUG
	m_MeshVertexBuffers_Default[componentUUID] = l_defaultHeapBuffer_VB;

	auto l_uploadHeapBuffer_VB = CreateUploadHeapBuffer(&l_verticesResourceDesc);
	if (!l_uploadHeapBuffer_VB)
	{
		Log(Error, "can't create vertex buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_uploadHeapBuffer_VB, "UploadHeap_VB");
#endif //  INNO_DEBUG
	m_MeshVertexBuffers_Upload[componentUUID] = l_uploadHeapBuffer_VB;

	rhs->m_VertexBufferView.m_BufferLocation = l_defaultHeapBuffer_VB->GetGPUVirtualAddress();
	rhs->m_VertexBufferView.m_SizeInBytes = l_verticesDataSize;
	rhs->m_VertexBufferView.m_StrideInBytes = sizeof(Vertex);

	Log(Verbose, rhs->m_InstanceName, " Vertex Buffer is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);

	auto l_defaultHeapBuffer_IB = CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (!l_defaultHeapBuffer_IB)
	{
		Log(Error, "can't create index buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_defaultHeapBuffer_IB, "DefaultHeap_IB");
#endif //  INNO_DEBUG
	m_MeshIndexBuffers_Default[componentUUID] = l_defaultHeapBuffer_IB;

	auto l_uploadHeapBuffer_IB = CreateUploadHeapBuffer(&l_indicesResourceDesc);
	if (!l_uploadHeapBuffer_IB)
	{
		Log(Error, "can't create index buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_uploadHeapBuffer_IB, "UploadHeap_IB");
#endif //  INNO_DEBUG
	m_MeshIndexBuffers_Upload[componentUUID] = l_uploadHeapBuffer_IB;

	rhs->m_IndexBufferView.m_BufferLocation = l_defaultHeapBuffer_IB->GetGPUVirtualAddress();
	rhs->m_IndexBufferView.m_SizeInBytes = l_indicesDataSize;
	rhs->m_IndexBufferView.m_StrideInBytes = sizeof(Index);

	Log(Verbose, rhs->m_InstanceName, " Index Buffer is initialized.");

	// Flip y texture coordinate
	for (auto& i : vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_uploadHeapBuffer_VB->Map(0, &m_readRange, &rhs->m_MappedMemory_VB);
	l_uploadHeapBuffer_IB->Map(0, &m_readRange, &rhs->m_MappedMemory_IB);

	std::memcpy((char*)rhs->m_MappedMemory_VB, &vertices[0], vertices.size() * sizeof(Vertex));
	std::memcpy((char*)rhs->m_MappedMemory_IB, &indices[0], indices.size() * sizeof(Index));

	DX12CommandList l_commandList = {};
	l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

	UploadToGPU(&l_commandList, rhs);

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
	m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
	{
		Log(Error, "Failed to get prebuild info for BLAS!");
		return false;
	}

	auto blasResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_BLAS = CreateDefaultHeapBuffer(&blasResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	if (!l_BLAS)
	{
		Log(Error, "Failed to create BLAS buffer!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_BLAS, "BLAS");
#endif // INNO_DEBUG
	m_MeshBLAS[componentUUID] = l_BLAS;

	auto scratchResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_scratchBuffer = CreateDefaultHeapBuffer(&scratchResourceDesc);
	if (!l_scratchBuffer)
	{
		Log(Error, "Failed to create scratch buffer for BLAS!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(rhs, l_scratchBuffer, "ScratchBuffer_BLAS");
#endif // INNO_DEBUG
	m_MeshScratchBuffers[componentUUID] = l_scratchBuffer;

	// Transition index and vertex buffers to NON_PIXEL_SHADER_RESOURCE state for BLAS build.
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = l_scratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = l_BLAS->GetGPUVirtualAddress();

	l_commandList.m_DirectCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_BLAS.Get());

	// Transition the vertex and index buffers back to their original states after BLAS build.
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_defaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	Log(Verbose, rhs->m_InstanceName, " BLAS is initialized.");

	//g_Engine->Get<PhysicsSimulationService>()->CreateCollisionPrimitive(rhs);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(TextureComponent* rhs, void* textureData)
{
	rhs->m_GPUResourceType = GPUResourceType::Image;
	auto l_textureDesc = GetDX12TextureDesc(rhs->m_TextureDesc);
	auto l_writeState = GetTextureWriteState(rhs->m_TextureDesc);
	auto l_readState = GetTextureReadState(rhs->m_TextureDesc);
	rhs->m_CurrentState = static_cast<uint32_t>(l_readState);

	D3D12_CLEAR_VALUE l_clearValue = {};
	bool useClearValue = false;

	if (rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_clearValue.Format = l_textureDesc.Format;
		l_clearValue.Color[0] = rhs->m_TextureDesc.ClearColor[0];
		l_clearValue.Color[1] = rhs->m_TextureDesc.ClearColor[1];
		l_clearValue.Color[2] = rhs->m_TextureDesc.ClearColor[2];
		l_clearValue.Color[3] = rhs->m_TextureDesc.ClearColor[3];
		useClearValue = true;
	}

	uint32_t frameCount = rhs->m_TextureDesc.IsMultiBuffer ? GetSwapChainImageCount() : 1;
	rhs->m_GPUResources.resize(frameCount);
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		ComPtr<ID3D12Resource> resource;
		if (useClearValue)
			resource = CreateDefaultHeapBuffer(&l_textureDesc, D3D12_RESOURCE_STATE_COMMON, &l_clearValue);
		else
			resource = CreateDefaultHeapBuffer(&l_textureDesc);

		if (!resource)
		{
			Log(Error, "Failed to create texture buffer for frame ", frame);
			return false;
		}

#ifdef INNO_DEBUG
		SetObjectName(rhs, resource, ("DefaultHeap_Texture_Frame" + std::to_string(frame)).c_str());
#endif

		rhs->m_GPUResources[frame] = resource.Get();
		resource.Detach(); // Component now owns the resource
	}

	DX12CommandList l_commandList = {};
	l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
	if (textureData)
	{
		// Upload to first frame's resource
		auto* resource = static_cast<ID3D12Resource*>(rhs->m_GPUResources[0]);

		uint32_t l_subresourcesCount = rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(resource, 0, l_subresourcesCount);

		auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
		auto l_uploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc);

		if (!l_uploadHeapBuffer)
		{
			Log(Error, "can't create texture buffer on Upload Heap!");
			return false;
		}

#ifdef INNO_DEBUG
		SetObjectName(rhs, l_uploadHeapBuffer, "UploadHeap_Texture");
#endif

		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		l_textureSubResourceData.RowPitch = rhs->m_TextureDesc.Width * GetTexturePixelDataSize(rhs->m_TextureDesc);
		l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * rhs->m_TextureDesc.Height;
		l_textureSubResourceData.pData = (unsigned char*)textureData;

		UpdateSubresources(l_commandList.m_DirectCommandList.Get(), resource, l_uploadHeapBuffer.Get(), 0, 0, l_subresourcesCount, &l_textureSubResourceData);
		l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState)));

		// Copy to all other frames
		for (uint32_t frame = 1; frame < frameCount; frame++)
		{
			auto* destResource = static_cast<ID3D12Resource*>(rhs->m_GPUResources[frame]);
			l_commandList.m_DirectCommandList->CopyResource(destResource, resource);
			l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				destResource, D3D12_RESOURCE_STATE_COPY_DEST, static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState)));
		}
	}
	else
	{
		// Transition all resources to initial state
		for (uint32_t frame = 0; frame < frameCount; frame++)
		{
			auto* resource = static_cast<ID3D12Resource*>(rhs->m_GPUResources[frame]);
			l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				resource, D3D12_RESOURCE_STATE_COMMON, static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState)));
		}
	}

	uint32_t mipLevels = l_textureDesc.MipLevels;
	rhs->m_ReadHandles.resize(frameCount * mipLevels);
	rhs->m_WriteHandles.resize(frameCount * mipLevels);
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		for (uint32_t mip = 0; mip < mipLevels; mip++)
		{
			uint32_t handleIndex = rhs->GetHandleIndex(frame, mip);
			if (!CreateSRV(rhs, mip))
			{
				Log(Error, "Failed to create SRV for frame ", frame, " mip ", mip);
				return false;
			}
		}
	}

	if (rhs->m_TextureDesc.Usage != TextureUsage::DepthAttachment
		&& rhs->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment
		&& !rhs->m_TextureDesc.IsSRGB)
	{
		for (uint32_t frame = 0; frame < frameCount; frame++)
		{
			for (uint32_t mip = 0; mip < mipLevels; mip++)
			{
				if (!CreateUAV(rhs, mip))
				{
					Log(Error, "Failed to create UAV for frame ", frame, " mip ", mip);
					return false;
				}
			}
		}
	}

	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	if (rhs->m_TextureDesc.MipLevels > 1)
	{
		GenerateMipmap(rhs, nullptr);
	}

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, "Texture ", rhs->m_InstanceName, " is initialized.");

	return true;
}

bool DX12RenderingServer::InitializeImpl(ShaderProgramComponent* rhs)
{
	// No need to cast - use base component directly
#ifdef USE_DXIL
	if (rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(rhs->m_VSBuffer, rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(rhs->m_HSBuffer, rhs->m_ShaderFilePaths.m_HSPath);
	}
	if (rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(rhs->m_DSBuffer, rhs->m_ShaderFilePaths.m_DSPath);
	}
	if (rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(rhs->m_GSBuffer, rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(rhs->m_PSBuffer, rhs->m_ShaderFilePaths.m_PSPath);
	}
	if (rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(rhs->m_CSBuffer, rhs->m_ShaderFilePaths.m_CSPath);
	}
	if (rhs->m_ShaderFilePaths.m_RayGenPath != "")
	{
		LoadShaderFile(rhs->m_RayGenBuffer, rhs->m_ShaderFilePaths.m_RayGenPath);
	}
	if (rhs->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		LoadShaderFile(rhs->m_AnyHitBuffer, rhs->m_ShaderFilePaths.m_AnyHitPath);
	}
	if (rhs->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		LoadShaderFile(rhs->m_ClosestHitBuffer, rhs->m_ShaderFilePaths.m_ClosestHitPath);
	}
	if (rhs->m_ShaderFilePaths.m_MissPath != "")
	{
		LoadShaderFile(rhs->m_MissBuffer, rhs->m_ShaderFilePaths.m_MissPath);
	}
#else
	// For non-DXIL path, we need temporary ID3DBlob storage
	ComPtr<ID3DBlob> tempBuffer;
	if (rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Vertex, rhs->m_ShaderFilePaths.m_VSPath))
		{
			rhs->m_VSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_VSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Hull, rhs->m_ShaderFilePaths.m_HSPath))
		{
			rhs->m_HSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_HSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Domain, rhs->m_ShaderFilePaths.m_DSPath))
		{
			rhs->m_DSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_DSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Geometry, rhs->m_ShaderFilePaths.m_GSPath))
		{
			rhs->m_GSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_GSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Pixel, rhs->m_ShaderFilePaths.m_PSPath))
		{
			rhs->m_PSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_PSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Compute, rhs->m_ShaderFilePaths.m_CSPath))
		{
			rhs->m_CSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_CSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_RayGenPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::RayGen, rhs->m_ShaderFilePaths.m_RayGenPath))
		{
			rhs->m_RayGenBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_RayGenBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::AnyHit, rhs->m_ShaderFilePaths.m_AnyHitPath))
		{
			rhs->m_AnyHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_AnyHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::ClosestHit, rhs->m_ShaderFilePaths.m_ClosestHitPath))
		{
			rhs->m_ClosestHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_ClosestHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (rhs->m_ShaderFilePaths.m_MissPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, rhs->m_ShaderFilePaths.m_MissPath))
		{
			rhs->m_MissBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(rhs->m_MissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
#endif
	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(SamplerComponent* rhs)
{
	rhs->m_GPUResourceType = GPUResourceType::Sampler;

	D3D12_SAMPLER_DESC l_samplerDesc = {};
	l_samplerDesc.Filter = GetFilterMode(rhs->m_SamplerDesc.m_MinFilterMethod, rhs->m_SamplerDesc.m_MagFilterMethod);
	l_samplerDesc.AddressU = GetWrapMode(rhs->m_SamplerDesc.m_WrapMethodU);
	l_samplerDesc.AddressV = GetWrapMode(rhs->m_SamplerDesc.m_WrapMethodV);
	l_samplerDesc.AddressW = GetWrapMode(rhs->m_SamplerDesc.m_WrapMethodW);
	l_samplerDesc.MipLODBias = 0.0f;
	l_samplerDesc.MaxAnisotropy = rhs->m_SamplerDesc.m_MaxAnisotropy;
	l_samplerDesc.BorderColor[0] = rhs->m_SamplerDesc.m_BorderColor[0];
	l_samplerDesc.BorderColor[1] = rhs->m_SamplerDesc.m_BorderColor[1];
	l_samplerDesc.BorderColor[2] = rhs->m_SamplerDesc.m_BorderColor[2];
	l_samplerDesc.BorderColor[3] = rhs->m_SamplerDesc.m_BorderColor[3];
	l_samplerDesc.MinLOD = rhs->m_SamplerDesc.m_MinLOD;
	l_samplerDesc.MaxLOD = rhs->m_SamplerDesc.m_MaxLOD;

	// @TODO: We don't really need multi-frame samplers
	rhs->m_ReadHandles.resize(GetSwapChainImageCount());
	for (auto& handle : rhs->m_ReadHandles)
	{
		handle = m_SamplerDescHeapAccessor.GetNewHandle();
		m_device->CreateSampler(&l_samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ handle.m_CPUHandle });
	}

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(GPUBufferComponent* rhs)
{
	// @TODO: Make it just like the texture's read/write state
	auto l_initialState = D3D12_RESOURCE_STATE_COMMON;
	auto l_isRaytracingAS = rhs->m_Usage == GPUBufferUsage::TLAS || rhs->m_Usage == GPUBufferUsage::ScratchBuffer;
	if (rhs->m_Usage == GPUBufferUsage::IndirectDraw)
	{
		rhs->m_ElementSize = sizeof(DX12DrawIndirectCommand);
		rhs->m_ElementCount = 256;  // TODO: Don't hardcode
		rhs->m_IndirectDrawCommandList = new DX12IndirectDrawCommandList();
	}
	else if (l_isRaytracingAS)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
		tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		tlasInputs.NumDescs = rhs->m_ElementCount;
		tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfo);

		UINT64 TLAS_SIZE_IN_BYTES = prebuildInfo.ResultDataMaxSizeInBytes;
		UINT64 SCRATCH_SIZE_IN_BYTES = prebuildInfo.ScratchDataSizeInBytes;

		rhs->m_ElementSize = rhs->m_Usage == GPUBufferUsage::TLAS ? TLAS_SIZE_IN_BYTES : SCRATCH_SIZE_IN_BYTES;
		if (rhs->m_Usage == GPUBufferUsage::TLAS)
			l_initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE; // This has to be set when the heap is created.
	}

	auto l_actualElementCount = l_isRaytracingAS ? 1 : rhs->m_ElementCount;
	auto l_swapChainImageCount = GetSwapChainImageCount();
	rhs->m_GPUResourceType = GPUResourceType::Buffer;
	rhs->m_TotalSize = l_actualElementCount * rhs->m_ElementSize;
	rhs->m_MappedMemories.resize(l_swapChainImageCount);
	rhs->m_DeviceMemories.resize(l_swapChainImageCount);

	auto l_uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rhs->m_TotalSize);
	bool l_needDefaultHeap = rhs->m_GPUAccessibility.CanRead() && !rhs->m_CPUAccessibility.CanRead();

	for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
	{
		auto l_mappedMemory = new DX12MappedMemory();
		l_mappedMemory->m_UploadHeapBuffer = CreateUploadHeapBuffer(&l_uploadBufferDesc);

#ifdef INNO_DEBUG
		SetObjectName(rhs, l_mappedMemory->m_UploadHeapBuffer, ("UploadHeap_" + std::to_string(i)).c_str());
#endif
		CD3DX12_RANGE m_readRange(0, 0);
		l_mappedMemory->m_UploadHeapBuffer->Map(0, &m_readRange, &l_mappedMemory->m_Address);
		rhs->m_MappedMemories[i] = l_mappedMemory;

		if (!l_needDefaultHeap)
			continue;

		auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(rhs->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto l_deviceMemory = new DX12DeviceMemory();
		l_deviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, l_initialState);

#ifdef INNO_DEBUG
		SetObjectName(rhs, l_deviceMemory->m_DefaultHeapBuffer, ("DefaultHeap_" + std::to_string(i)).c_str());
#endif
		rhs->m_DeviceMemories[i] = l_deviceMemory;
	}

	if (rhs->m_InitialData)
	{
		DX12CommandList l_commandList = {};
		l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

		for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
		{
			auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(rhs->m_MappedMemories[i]);
			auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(rhs->m_DeviceMemories[i]);
			WriteMappedMemory(rhs, l_mappedMemory, rhs->m_InitialData, 0, rhs->m_TotalSize);
			l_mappedMemory->m_NeedUploadToGPU = false;
			UploadToGPU(&l_commandList, l_mappedMemory, l_deviceMemory, rhs);
		}

		ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	}

	if (rhs->m_Usage != GPUBufferUsage::IndirectDraw
		&& rhs->m_Usage != GPUBufferUsage::ScratchBuffer)
	{
		CreateSRV(rhs);
		if (l_needDefaultHeap)
			CreateUAV(rhs);
	}

	Log(Verbose, "GPU Buffer: ", rhs->m_InstanceName, " (", rhs->m_Usage, ") is initialized.");
	rhs->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool DX12RenderingServer::InitializeImpl(CollisionComponent* rhs)
{
	// auto l_transformMatrix = rhs->m_TransformComponent->m_globalTransformMatrix.m_transformationMat;

	// for (size_t i = 0; i < GetSwapChainImageCount(); i++)
	// {
	// 	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	// 	instanceDesc.Transform[0][0] = l_transformMatrix.m00;
	// 	instanceDesc.Transform[0][1] = l_transformMatrix.m01;
	// 	instanceDesc.Transform[0][2] = l_transformMatrix.m02;
	// 	instanceDesc.Transform[0][3] = l_transformMatrix.m03; // Translation X

	// 	instanceDesc.Transform[1][0] = l_transformMatrix.m10;
	// 	instanceDesc.Transform[1][1] = l_transformMatrix.m11;
	// 	instanceDesc.Transform[1][2] = l_transformMatrix.m12;
	// 	instanceDesc.Transform[1][3] = l_transformMatrix.m13; // Translation Y

	// 	instanceDesc.Transform[2][0] = l_transformMatrix.m20;
	// 	instanceDesc.Transform[2][1] = l_transformMatrix.m21;
	// 	instanceDesc.Transform[2][2] = l_transformMatrix.m22;
	// 	instanceDesc.Transform[2][3] = l_transformMatrix.m23; // Translation Z

	// 	instanceDesc.InstanceID = static_cast<UINT>(rhs->m_RenderableSet->material->m_ShaderModel);
	// 	instanceDesc.InstanceMask = 0xFF;
	// 	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	// 	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

	// 	auto l_mesh = rhs->m_RenderableSet->mesh;
	// 	auto blasIt = m_MeshBLAS.find(l_mesh->m_UUID);
	// 	if (blasIt != m_MeshBLAS.end())
	// 	{
	// 		instanceDesc.AccelerationStructure = blasIt->second->GetGPUVirtualAddress();
	// 		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
	// 		l_descList->m_Descs.emplace_back(instanceDesc);
	// 		l_descList->m_NeedFullUpdate = true;
	// 	}
	// }

	Log(Verbose, rhs->m_InstanceName, " Raytracing instance is registered.");

	m_RegisteredCollisionComponents.emplace(rhs);

	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, MeshComponent* rhs)
{
	auto componentUUID = rhs->m_UUID;
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_DX12CommandList = l_commandList->m_DirectCommandList;

	auto vertexDefault = m_MeshVertexBuffers_Default[componentUUID];
	auto vertexUpload = m_MeshVertexBuffers_Upload[componentUUID];
	auto indexDefault = m_MeshIndexBuffers_Default[componentUUID];
	auto indexUpload = m_MeshIndexBuffers_Upload[componentUUID];

	if (rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexDefault.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexDefault.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	l_DX12CommandList->CopyResource(vertexDefault.Get(), vertexUpload.Get());
	l_DX12CommandList->CopyResource(indexDefault.Get(), indexUpload.Get());
	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, TextureComponent* rhs)
{
	// Texture upload is handled during initialization with centralized resources
	// This function is kept for interface compatibility
	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GPUBufferComponent*>(rhs);
	auto l_currentFrame = GetCurrentFrame();
	auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_rhs->m_MappedMemories[l_currentFrame]);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[l_currentFrame]);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);

	return UploadToGPU(l_commandList, l_mappedMemory, l_deviceMemory, l_rhs);
}

bool DX12RenderingServer::UploadToGPU(DX12CommandList* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* GPUBufferComponent)
{
	if (!deviceMemory->m_DefaultHeapBuffer)
		return true;

	auto l_DX12CommandList = commandList->m_DirectCommandList;
	auto l_beforeState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	if (GPUBufferComponent->m_Usage == GPUBufferUsage::IndirectDraw)
		l_beforeState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	else if (GPUBufferComponent->m_Usage == GPUBufferUsage::TLAS)
		l_beforeState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	auto l_afterState = D3D12_RESOURCE_STATE_COPY_DEST;
	if (GPUBufferComponent->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), l_beforeState, l_afterState));
	}

	l_DX12CommandList->CopyResource(deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get());

	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), l_afterState, l_beforeState));

	return true;
}

bool DX12RenderingServer::Clear(ICommandList* commandList, GPUBufferComponent* rhs)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);

	l_commandList->m_DirectCommandList->Reset(GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->m_DirectCommandList->SetDescriptorHeaps(1, l_heaps);

	auto l_currentFrame = GetCurrentFrame();
	const uint32_t zero = 0;
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(rhs->m_DeviceMemories[l_currentFrame]);
	l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
		D3D12_GPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_GPUHandle },
		D3D12_CPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_CPUHandle },
		l_deviceMemory->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	return true;
}

bool DX12RenderingServer::Copy(ICommandList* commandList, TextureComponent* lhs, TextureComponent* rhs)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	// Get resources directly from components
	auto* srcResource = static_cast<ID3D12Resource*>(lhs->GetGPUResource(frameIndex));
	auto* destResource = static_cast<ID3D12Resource*>(rhs->GetGPUResource(frameIndex));

	if (!srcResource || !destResource)
	{
		Log(Error, "Cannot find texture resources for copy operation");
		return false;
	}

	// Get current states
	auto l_srcReadState = GetTextureReadState(lhs->m_TextureDesc);
	auto l_destReadState = GetTextureReadState(rhs->m_TextureDesc);

	// Transition source to copy source state
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		srcResource,
		static_cast<D3D12_RESOURCE_STATES>(lhs->m_CurrentState),
		D3D12_RESOURCE_STATE_COPY_SOURCE));

	// Transition destination to copy dest state
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		destResource,
		static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState),
		D3D12_RESOURCE_STATE_COPY_DEST));

	// Copy resource
	l_commandList->m_DirectCommandList->CopyResource(destResource, srcResource);

	// Transition back to original states
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		srcResource,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		static_cast<D3D12_RESOURCE_STATES>(lhs->m_CurrentState)));

	l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		destResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState)));

	return true;
}

bool DX12RenderingServer::Clear(ICommandList* commandList, TextureComponent* rhs)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	// Get resource directly from component
	auto* resource = static_cast<ID3D12Resource*>(rhs->GetGPUResource(frameIndex));
	if (!resource)
	{
		Log(Error, "Cannot find texture resource for clear operation");
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->m_DirectCommandList->SetDescriptorHeaps(1, l_heaps);

	// Get write state for the texture
	auto l_writeState = GetTextureWriteState(rhs->m_TextureDesc);

	// Transition to UAV state if needed
	if (rhs->m_CurrentState != static_cast<uint32_t>(l_writeState))
	{
		l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState),
			l_writeState));
	}

	// Clear using the UAV handle for current frame, mip 0
	uint32_t handleIndex = rhs->GetHandleIndex(frameIndex, 0);
	if (handleIndex < rhs->m_WriteHandles.size())
	{
		if (rhs->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
		{
			l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
				D3D12_GPU_DESCRIPTOR_HANDLE{ rhs->m_WriteHandles[handleIndex].m_GPUHandle },
				D3D12_CPU_DESCRIPTOR_HANDLE{ rhs->m_WriteHandles[handleIndex].m_CPUHandle },
				resource,
				(UINT*)&rhs->m_TextureDesc.ClearColor[0],
				0,
				NULL);
		}
		else
		{
			l_commandList->m_DirectCommandList->ClearUnorderedAccessViewFloat(
				D3D12_GPU_DESCRIPTOR_HANDLE{ rhs->m_WriteHandles[handleIndex].m_GPUHandle },
				D3D12_CPU_DESCRIPTOR_HANDLE{ rhs->m_WriteHandles[handleIndex].m_CPUHandle },
				resource,
				&rhs->m_TextureDesc.ClearColor[0],
				0,
				NULL);
		}
	}

	// Transition back if needed
	if (rhs->m_CurrentState != static_cast<uint32_t>(l_writeState))
	{
		l_commandList->m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			l_writeState,
			static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState)));
	}

	return true;
}