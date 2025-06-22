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
#include "../../Services/ComponentManager.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::InitializeImpl(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto componentUUID = mesh->m_UUID;

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);

	auto l_defaultHeapBuffer_VB = CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (!l_defaultHeapBuffer_VB)
	{
		Log(Error, mesh->m_InstanceName, " can't create vertex buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_defaultHeapBuffer_VB, "DefaultHeap_VB");
#endif //  INNO_DEBUG
	m_MeshVertexBuffers_Default[componentUUID] = l_defaultHeapBuffer_VB;

	auto l_uploadHeapBuffer_VB = CreateUploadHeapBuffer(&l_verticesResourceDesc);
	if (!l_uploadHeapBuffer_VB)
	{
		Log(Error, mesh->m_InstanceName, " can't create vertex buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_uploadHeapBuffer_VB, "UploadHeap_VB");
#endif //  INNO_DEBUG
	m_MeshVertexBuffers_Upload[componentUUID] = l_uploadHeapBuffer_VB;

	mesh->m_VertexBufferView.m_BufferLocation = l_defaultHeapBuffer_VB->GetGPUVirtualAddress();
	mesh->m_VertexBufferView.m_SizeInBytes = l_verticesDataSize;
	mesh->m_VertexBufferView.m_StrideInBytes = sizeof(Vertex);

	Log(Verbose, mesh->m_InstanceName, " Vertex Buffer is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);

	auto l_defaultHeapBuffer_IB = CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (!l_defaultHeapBuffer_IB)
	{
		Log(Error, mesh->m_InstanceName, " can't create index buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_defaultHeapBuffer_IB, "DefaultHeap_IB");
#endif //  INNO_DEBUG
	m_MeshIndexBuffers_Default[componentUUID] = l_defaultHeapBuffer_IB;

	auto l_uploadHeapBuffer_IB = CreateUploadHeapBuffer(&l_indicesResourceDesc);
	if (!l_uploadHeapBuffer_IB)
	{
		Log(Error, mesh->m_InstanceName, " can't create index buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_uploadHeapBuffer_IB, "UploadHeap_IB");
#endif //  INNO_DEBUG
	m_MeshIndexBuffers_Upload[componentUUID] = l_uploadHeapBuffer_IB;

	mesh->m_IndexBufferView.m_BufferLocation = l_defaultHeapBuffer_IB->GetGPUVirtualAddress();
	mesh->m_IndexBufferView.m_SizeInBytes = l_indicesDataSize;
	mesh->m_IndexBufferView.m_StrideInBytes = sizeof(Index);

	Log(Verbose, mesh->m_InstanceName, " Index Buffer is initialized.");

	// Flip y texture coordinate
	for (auto& i : vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_uploadHeapBuffer_VB->Map(0, &m_readRange, &mesh->m_MappedMemory_VB);
	l_uploadHeapBuffer_IB->Map(0, &m_readRange, &mesh->m_MappedMemory_IB);

	std::memcpy((char*)mesh->m_MappedMemory_VB, &vertices[0], vertices.size() * sizeof(Vertex));
	std::memcpy((char*)mesh->m_MappedMemory_IB, &indices[0], indices.size() * sizeof(Index));

	CommandListComponent l_commandList = {};
	l_commandList.m_Type = GPUEngineType::Graphics;
	auto l_dx12CommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), L"MeshInitCommandList");
	l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

	UploadToGPU(&l_commandList, mesh);

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
		Log(Error, mesh->m_InstanceName, " Failed to get prebuild info for BLAS!");
		return false;
	}

	auto blasResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_BLAS = CreateDefaultHeapBuffer(&blasResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	if (!l_BLAS)
	{
		Log(Error, mesh->m_InstanceName, " Failed to create BLAS buffer!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_BLAS, "BLAS");
#endif // INNO_DEBUG
	m_MeshBLAS[componentUUID] = l_BLAS;

	auto scratchResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto l_scratchBuffer = CreateDefaultHeapBuffer(&scratchResourceDesc);
	if (!l_scratchBuffer)
	{
		Log(Error, mesh->m_InstanceName, " Failed to create scratch buffer for BLAS!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(mesh, l_scratchBuffer, "ScratchBuffer_BLAS");
#endif // INNO_DEBUG
	m_MeshScratchBuffers[componentUUID] = l_scratchBuffer;

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

	Close(&l_commandList, GPUEngineType::Graphics);
	Execute(&l_commandList, GPUEngineType::Graphics);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
	auto l_semaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
	WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);

	Log(Verbose, mesh->m_InstanceName, " BLAS is initialized.");

	mesh->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(TextureComponent* texture, void* textureData)
{
	texture->m_GPUResourceType = GPUResourceType::Image;
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	texture->m_WriteState = GetTextureWriteState(texture->m_TextureDesc);
	texture->m_ReadState = GetTextureReadState(texture->m_TextureDesc);

	if (texture->m_TextureDesc.Usage == TextureUsage::Sample)
		texture->m_CurrentState = texture->m_ReadState;
	else
		texture->m_CurrentState = texture->m_WriteState;

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

	uint32_t frameCount = texture->m_TextureDesc.IsMultiBuffer ? GetSwapChainImageCount() : 1;
	texture->m_GPUResources.resize(frameCount);

	D3D12_RESOURCE_STATES l_initialState = static_cast<D3D12_RESOURCE_STATES>(texture->m_CurrentState);
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		ComPtr<ID3D12Resource> defaultHeapBuffer;
		if (useClearValue)
			defaultHeapBuffer = CreateDefaultHeapBuffer(&l_textureDesc, l_initialState, &l_clearValue);
		else
			defaultHeapBuffer = CreateDefaultHeapBuffer(&l_textureDesc, l_initialState);

		if (!defaultHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create default heap buffer for frame ", frame);
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		SetObjectName(texture, defaultHeapBuffer, ("DefaultHeap_Texture_Frame" + std::to_string(frame)).c_str());
#endif

		texture->m_GPUResources[frame] = defaultHeapBuffer.Get();
		m_TextureBuffers_Default[texture->m_UUID] = defaultHeapBuffer;
		defaultHeapBuffer.Detach();
	}

	// Phase 1: Upload texture data with direct command list
	if (textureData)
	{
		CommandListComponent l_uploadCommandList = {};
		l_uploadCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12UploadCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), L"TextureUploadCommandList");
		l_uploadCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12UploadCommandList.Get());

		auto* defaultHeapBuffer_Frame0 = static_cast<ID3D12Resource*>(texture->m_GPUResources[0]);
		uint32_t l_subresourcesCount = texture->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(defaultHeapBuffer_Frame0, 0, l_subresourcesCount);

		auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
		auto l_uploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc);
		if (!l_uploadHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create upload heap buffer for frame 0");
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		SetObjectName(texture, l_uploadHeapBuffer, "UploadHeap_Texture");
#endif

		m_TextureBuffers_Upload[texture->m_UUID] = l_uploadHeapBuffer;

		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		l_textureSubResourceData.RowPitch = texture->m_TextureDesc.Width * GetTexturePixelDataSize(texture->m_TextureDesc);
		l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * texture->m_TextureDesc.Height;
		l_textureSubResourceData.pData = (unsigned char*)textureData;

		// Determine final state after upload
		D3D12_RESOURCE_STATES l_finalState = (texture->m_TextureDesc.MipLevels > 1) ?
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS : static_cast<D3D12_RESOURCE_STATES>(texture->m_ReadState);
		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, l_initialState, D3D12_RESOURCE_STATE_COPY_DEST));

			UpdateSubresources(l_dx12UploadCommandList.Get(), l_defaultHeapBuffer, l_uploadHeapBuffer.Get(), 0, 0, l_subresourcesCount, &l_textureSubResourceData);

			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, D3D12_RESOURCE_STATE_COPY_DEST, l_finalState));
		}

		// Update current state
		texture->m_CurrentState = static_cast<uint32_t>(l_finalState);

		// Execute and wait for upload phase
		Close(&l_uploadCommandList, GPUEngineType::Graphics);
		Execute(&l_uploadCommandList, GPUEngineType::Graphics);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		auto l_uploadSemaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
		WaitOnCPU(l_uploadSemaphoreValue, GPUEngineType::Graphics);
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
		// Texture should already be in UAV state from upload phase
		CommandListComponent l_mipmapCommandList = {};
		l_mipmapCommandList.m_Type = GPUEngineType::Compute;
		auto l_dx12MipmapCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE), L"TextureMipmapCommandList");
		l_mipmapCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12MipmapCommandList.Get());

		// Generate mipmaps (texture must be in UAV state)
		GenerateMipmap(texture, &l_mipmapCommandList);

		// Execute and wait for mipmap generation
		l_dx12MipmapCommandList->Close();
		Execute(&l_mipmapCommandList, GPUEngineType::Compute);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
		auto l_computeSemaphoreValue = GetSemaphoreValue(GPUEngineType::Compute);
		WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);

		// Phase 3: Transition texture back to read state for rendering passes
		CommandListComponent l_transitionCommandList = {};
		l_transitionCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12TransitionCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), L"TextureTransitionCommandList");
		l_transitionCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12TransitionCommandList.Get());

		// Transition all texture resources back to their read state
		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12TransitionCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				static_cast<D3D12_RESOURCE_STATES>(texture->m_ReadState)));
		}

		// Update texture state
		texture->m_CurrentState = texture->m_ReadState;

		// Execute and wait for transition
		Close(&l_transitionCommandList, GPUEngineType::Graphics);
		Execute(&l_transitionCommandList, GPUEngineType::Graphics);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		auto l_transitionSemaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
		WaitOnCPU(l_transitionSemaphoreValue, GPUEngineType::Graphics);
	}

	texture->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, texture->m_InstanceName, " is initialized.");

	return true;
}

bool DX12RenderingServer::InitializeImpl(ShaderProgramComponent* shaderProgram)
{
	// No need to cast - use base component directly
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
#endif
	shaderProgram->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(SamplerComponent* sampler)
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
	sampler->m_ReadHandles.resize(GetSwapChainImageCount());
	for (auto& handle : sampler->m_ReadHandles)
	{
		handle = m_SamplerDescHeapAccessor.GetNewHandle();
		m_device->CreateSampler(&l_samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ handle.m_CPUHandle });
	}

	sampler->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(GPUBufferComponent* gpuBuffer)
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
		gpuBuffer->m_CurrentState = gpuBuffer->m_WriteState;
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
		m_device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfo);

		UINT64 TLAS_SIZE_IN_BYTES = prebuildInfo.ResultDataMaxSizeInBytes;
		UINT64 SCRATCH_SIZE_IN_BYTES = prebuildInfo.ScratchDataSizeInBytes;

		gpuBuffer->m_ElementSize = gpuBuffer->m_Usage == GPUBufferUsage::TLAS ? TLAS_SIZE_IN_BYTES : SCRATCH_SIZE_IN_BYTES;
		if (gpuBuffer->m_Usage == GPUBufferUsage::TLAS)
			l_initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE; // This has to be set when the heap is created.

		gpuBuffer->m_ReadState = static_cast<uint32_t>(l_initialState);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gpuBuffer->m_CurrentState = gpuBuffer->m_ReadState;
	}
	else
	{
		// Default buffer states for other usage types
		gpuBuffer->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_COMMON);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gpuBuffer->m_CurrentState = gpuBuffer->m_ReadState;
	}

	auto l_actualElementCount = l_isRaytracingAS ? 1 : gpuBuffer->m_ElementCount;
	auto l_swapChainImageCount = GetSwapChainImageCount();
	gpuBuffer->m_GPUResourceType = GPUResourceType::Buffer;
	gpuBuffer->m_TotalSize = l_actualElementCount * gpuBuffer->m_ElementSize;
	gpuBuffer->m_MappedMemories.resize(l_swapChainImageCount);
	gpuBuffer->m_DeviceMemories.resize(l_swapChainImageCount);

	auto l_uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize);
	bool l_needDefaultHeap = gpuBuffer->m_GPUAccessibility.CanRead() && !gpuBuffer->m_CPUAccessibility.CanRead();

	for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
	{
		auto l_mappedMemory = new DX12MappedMemory();
		l_mappedMemory->m_UploadHeapBuffer = CreateUploadHeapBuffer(&l_uploadBufferDesc);

		if (!l_mappedMemory->m_UploadHeapBuffer)
		{
			Log(Error, "Failed to create upload heap buffer for frame ", i);
			delete l_mappedMemory;
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		SetObjectName(gpuBuffer, l_mappedMemory->m_UploadHeapBuffer, ("UploadHeap_" + std::to_string(i)).c_str());
#endif
		CD3DX12_RANGE m_readRange(0, 0);
		l_mappedMemory->m_UploadHeapBuffer->Map(0, &m_readRange, &l_mappedMemory->m_Address);
		gpuBuffer->m_MappedMemories[i] = l_mappedMemory;

		if (!l_needDefaultHeap)
			continue;

		auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto l_deviceMemory = new DX12DeviceMemory();
		l_deviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, l_initialState);

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		SetObjectName(gpuBuffer, l_deviceMemory->m_DefaultHeapBuffer, ("DefaultHeap_" + std::to_string(i)).c_str());
#endif
		gpuBuffer->m_DeviceMemories[i] = l_deviceMemory;
	}

	if (gpuBuffer->m_InitialData)
	{
		CommandListComponent l_commandList = {};
		l_commandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12CommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), L"GPUBufferInitCommandList");
		l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

		for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
		{
			auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(gpuBuffer->m_MappedMemories[i]);
			auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[i]);
			WriteMappedMemory(gpuBuffer, l_mappedMemory, gpuBuffer->m_InitialData, 0, gpuBuffer->m_TotalSize);
			l_mappedMemory->m_NeedUploadToGPU = false;
			UploadToGPU(&l_commandList, l_mappedMemory, l_deviceMemory, gpuBuffer);
		}

		Close(&l_commandList, GPUEngineType::Graphics);
		Execute(&l_commandList, GPUEngineType::Graphics);
		SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		auto l_semaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
		WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);
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

bool DX12RenderingServer::InitializeImpl(ModelComponent* model)
{
	// Generate raytracing instance descriptors for each mesh in each draw call
	auto transformMatrix = model->m_Transform.GetMatrix();

	for (size_t frameIndex = 0; frameIndex < GetSwapChainImageCount(); frameIndex++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[frameIndex]);

		// Create instance descriptor for each mesh in each draw call component
		for (const auto& drawCallUUID : model->m_DrawCallComponents)
		{
			auto drawCallComp = g_Engine->Get<ComponentManager>()->FindByUUID<DrawCallComponent>(drawCallUUID);
			if (!drawCallComp || !drawCallComp->m_MeshComponent || drawCallComp->m_ObjectStatus != ObjectStatus::Activated)
				return false;

			auto meshComp = g_Engine->Get<ComponentManager>()->FindByUUID<MeshComponent>(drawCallComp->m_MeshComponent);
			if (!meshComp || meshComp->m_ObjectStatus != ObjectStatus::Activated)
				return false;

			auto blasIt = m_MeshBLAS.find(meshComp->m_UUID);
			if (blasIt == m_MeshBLAS.end())
				return false;

			D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

			// Apply model transform matrix
			instanceDesc.Transform[0][0] = transformMatrix.m00;
			instanceDesc.Transform[0][1] = transformMatrix.m01;
			instanceDesc.Transform[0][2] = transformMatrix.m02;
			instanceDesc.Transform[0][3] = transformMatrix.m03; // Translation X

			instanceDesc.Transform[1][0] = transformMatrix.m10;
			instanceDesc.Transform[1][1] = transformMatrix.m11;
			instanceDesc.Transform[1][2] = transformMatrix.m12;
			instanceDesc.Transform[1][3] = transformMatrix.m13; // Translation Y

			instanceDesc.Transform[2][0] = transformMatrix.m20;
			instanceDesc.Transform[2][1] = transformMatrix.m21;
			instanceDesc.Transform[2][2] = transformMatrix.m22;
			instanceDesc.Transform[2][3] = transformMatrix.m23; // Translation Z

			// Use MeshComponent UUID for unique instance identification
			instanceDesc.InstanceID = static_cast<UINT>(meshComp->m_UUID);
			instanceDesc.InstanceMask = 0xFF;
			instanceDesc.InstanceContributionToHitGroupIndex = 0;
			instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			instanceDesc.AccelerationStructure = blasIt->second->GetGPUVirtualAddress();

			l_descList->m_Descs.emplace_back(instanceDesc);
			l_descList->m_NeedFullUpdate = true;
		}
	}

	model->m_ObjectStatus = ObjectStatus::Activated;
	Log(Verbose, model->m_InstanceName, " Raytracing instances registered for ", model->m_DrawCallComponents.size(), " draw calls.");
	m_initializedModels.emplace(model);

	return true;
}

bool DX12RenderingServer::InitializeImpl(CommandListComponent* commandList)
{
	if (!commandList)
	{
		Log(Error, "CommandList parameter is null");
		return false;
	}

	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	HRESULT l_HResult;

	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		l_HResult = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Compute:
		l_HResult = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
			GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Copy:
		l_HResult = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
			GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY).Get(), nullptr,
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
	SetObjectName(commandList, l_commandList, "CommandList");
#endif

	commandList->m_CommandList = reinterpret_cast<uint64_t>(l_commandList.Detach());
	commandList->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, commandList->m_InstanceName, " Command list created successfully");
	return true;
}

bool DX12RenderingServer::UploadToGPU(CommandListComponent* commandList, MeshComponent* mesh)
{
	auto componentUUID = mesh->m_UUID;
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	auto vertexDefault = m_MeshVertexBuffers_Default[componentUUID];
	auto vertexUpload = m_MeshVertexBuffers_Upload[componentUUID];
	auto indexDefault = m_MeshIndexBuffers_Default[componentUUID];
	auto indexUpload = m_MeshIndexBuffers_Upload[componentUUID];

	if (mesh->m_ObjectStatus == ObjectStatus::Activated)
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

bool DX12RenderingServer::UploadToGPU(CommandListComponent* commandList, TextureComponent* texture)
{
	// Texture upload is handled during initialization with centralized resources
	// This function is kept for interface compatibility
	return true;
}

bool DX12RenderingServer::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_mesh = reinterpret_cast<GPUBufferComponent*>(gpuBuffer);
	auto l_currentFrame = GetCurrentFrame();
	auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_mesh->m_MappedMemories[l_currentFrame]);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_mesh->m_DeviceMemories[l_currentFrame]);
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	return UploadToGPU(commandList, l_mappedMemory, l_deviceMemory, l_mesh);
}

bool DX12RenderingServer::UploadToGPU(CommandListComponent* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* GPUBufferComponent)
{
	if (!deviceMemory->m_DefaultHeapBuffer)
		return true;

	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_beforeState = static_cast<D3D12_RESOURCE_STATES>(GPUBufferComponent->m_CurrentState);
	auto l_afterState = D3D12_RESOURCE_STATE_COPY_DEST;
	if (GPUBufferComponent->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), l_beforeState, l_afterState));
	}

	l_DX12CommandList->CopyResource(deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get());

	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), l_afterState, l_beforeState));

	return true;
}

bool DX12RenderingServer::Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	auto l_currentFrame = GetCurrentFrame();
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

bool DX12RenderingServer::Copy(CommandListComponent* commandList, TextureComponent* sourceTexture, TextureComponent* destinationTexture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = GetCurrentFrame();

	// Get resources directly from components
	auto* srcResource = static_cast<ID3D12Resource*>(sourceTexture->GetGPUResource(frameIndex));
	auto* destResource = static_cast<ID3D12Resource*>(destinationTexture->GetGPUResource(frameIndex));

	if (!srcResource || !destResource)
	{
		Log(Error, "Cannot find texture resources for copy operation");
		return false;
	}

	// Get current states
	auto l_srcReadState = sourceTexture->m_ReadState;
	auto l_destReadState = destinationTexture->m_ReadState;

	// Transition source to copy source state
	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		srcResource,
		static_cast<D3D12_RESOURCE_STATES>(sourceTexture->m_CurrentState),
		D3D12_RESOURCE_STATE_COPY_SOURCE));

	// Transition destination to copy dest state
	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		destResource,
		static_cast<D3D12_RESOURCE_STATES>(destinationTexture->m_CurrentState),
		D3D12_RESOURCE_STATE_COPY_DEST));

	// Copy resource
	l_commandList->CopyResource(destResource, srcResource);

	// Transition back to original states
	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		srcResource,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		static_cast<D3D12_RESOURCE_STATES>(sourceTexture->m_CurrentState)));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		destResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		static_cast<D3D12_RESOURCE_STATES>(destinationTexture->m_CurrentState)));

	return true;
}

bool DX12RenderingServer::Clear(CommandListComponent* commandList, TextureComponent* texture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = GetCurrentFrame();

	// Get resource directly from component
	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));
	if (!resource)
	{
		Log(Error, "Cannot find texture resource for clear operation");
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	auto l_currentState = texture->m_CurrentState;
	if (l_currentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			static_cast<D3D12_RESOURCE_STATES>(texture->m_CurrentState),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		texture->m_CurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

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

	if (l_currentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			static_cast<D3D12_RESOURCE_STATES>(l_currentState)));
	}

	return true;
}