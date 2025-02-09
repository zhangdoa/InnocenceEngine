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

bool DX12RenderingServer::InitializeImpl(MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent*>(rhs);

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * l_rhs->m_Vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);
	l_rhs->m_DefaultHeapBuffer_VB = CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (l_rhs->m_DefaultHeapBuffer_VB == nullptr)
	{
		Log(Error, "can't create vertex buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_VB, "DefaultHeap_VB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_VB = CreateUploadHeapBuffer(&l_verticesResourceDesc);
	if (l_rhs->m_UploadHeapBuffer_VB == nullptr)
	{
		Log(Error, "can't create vertex buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_UploadHeapBuffer_VB, "UploadHeap_VB");
#endif //  INNO_DEBUG

	// Initialize the vertex buffer view.
	l_rhs->m_VBV.BufferLocation = l_rhs->m_DefaultHeapBuffer_VB->GetGPUVirtualAddress();
	l_rhs->m_VBV.StrideInBytes = sizeof(Vertex);
	l_rhs->m_VBV.SizeInBytes = l_verticesDataSize;

	Log(Verbose, l_rhs->m_InstanceName, " Vertex Buffer is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * l_rhs->m_Indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);
	l_rhs->m_DefaultHeapBuffer_IB = CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (l_rhs->m_DefaultHeapBuffer_IB == nullptr)
	{
		Log(Error, "can't create index buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_IB, "DefaultHeap_IB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_IB = CreateUploadHeapBuffer(&l_indicesResourceDesc);
	if (l_rhs->m_UploadHeapBuffer_IB == nullptr)
	{
		Log(Error, "can't create index buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_UploadHeapBuffer_IB, "UploadHeap_IB");
#endif //  INNO_DEBUG

	// Initialize the index buffer view.
	l_rhs->m_IBV.Format = DXGI_FORMAT_R32_UINT;
	l_rhs->m_IBV.BufferLocation = l_rhs->m_DefaultHeapBuffer_IB->GetGPUVirtualAddress();
	l_rhs->m_IBV.SizeInBytes = l_indicesDataSize;

	Log(Verbose, l_rhs->m_InstanceName, " Index Buffer is initialized.");

	// Flip y texture coordinate
	for (auto& i : rhs->m_Vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer_VB->Map(0, &m_readRange, &l_rhs->m_MappedMemory_VB);
	l_rhs->m_UploadHeapBuffer_IB->Map(0, &m_readRange, &l_rhs->m_MappedMemory_IB);

	WriteMappedMemory(l_rhs);
	l_rhs->m_NeedUploadToGPU = false;

	DX12CommandList l_commandList = {};
	l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

	UploadToGPU(&l_commandList, l_rhs);

	// Create BLAS
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = l_rhs->m_DefaultHeapBuffer_VB->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(l_rhs->m_Vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = l_rhs->m_DefaultHeapBuffer_IB->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(l_rhs->m_Indices.size());
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
	l_rhs->m_BLAS = CreateDefaultHeapBuffer(&blasResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	if (l_rhs->m_BLAS == nullptr)
	{
		Log(Error, "Failed to create BLAS buffer!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_BLAS, "BLAS");
#endif // INNO_DEBUG

	auto scratchResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	l_rhs->m_ScratchBuffer = CreateDefaultHeapBuffer(&scratchResourceDesc);
	if (l_rhs->m_ScratchBuffer == nullptr)
	{
		Log(Error, "Failed to create scratch buffer for BLAS!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_ScratchBuffer, "ScratchBuffer_BLAS");
#endif // INNO_DEBUG

	// Transition index and vertex buffers to NON_PIXEL_SHADER_RESOURCE state for BLAS build.
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = l_rhs->m_ScratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = l_rhs->m_BLAS->GetGPUVirtualAddress();

	l_commandList.m_DirectCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_rhs->m_BLAS.Get());

	// Transition the vertex and index buffers back to their original states after BLAS build.
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	Log(Verbose, l_rhs->m_InstanceName, " BLAS is initialized.");

	g_Engine->Get<PhysicsSimulationService>()->CreateCollisionPrimitive(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_PixelDataSize = GetTexturePixelDataSize(l_rhs->m_TextureDesc);
	l_rhs->m_DX12TextureDesc = GetDX12TextureDesc(l_rhs->m_TextureDesc);
	l_rhs->m_WriteState = GetTextureWriteState(l_rhs->m_TextureDesc);
	l_rhs->m_ReadState = GetTextureReadState(l_rhs->m_TextureDesc);

	l_rhs->m_CurrentState = l_rhs->m_ReadState;

	if (l_rhs->m_TextureDesc.Usage == TextureUsage::Sample)
		l_rhs->m_DeviceMemories.resize(1);
	else
		l_rhs->m_DeviceMemories.resize(GetSwapChainImageCount());

	D3D12_CLEAR_VALUE l_clearValue = {};
	for (size_t i = 0; i < l_rhs->m_DeviceMemories.size(); i++)
	{
		auto l_DX12DeviceMemory = new DX12DeviceMemory();

		if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
		{
			l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
			l_DX12DeviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, D3D12_RESOURCE_STATE_COMMON, &l_clearValue);
		}
		else if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
		{
			l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
			l_DX12DeviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, D3D12_RESOURCE_STATE_COMMON, &l_clearValue);
		}
		else if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
		{
			l_clearValue.Format = l_rhs->m_DX12TextureDesc.Format;
			l_clearValue.Color[0] = l_rhs->m_TextureDesc.ClearColor[0];
			l_clearValue.Color[1] = l_rhs->m_TextureDesc.ClearColor[1];
			l_clearValue.Color[2] = l_rhs->m_TextureDesc.ClearColor[2];
			l_clearValue.Color[3] = l_rhs->m_TextureDesc.ClearColor[3];
			l_DX12DeviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, D3D12_RESOURCE_STATE_COMMON, &l_clearValue);
		}
		// It has to be written like this because:
		// pOptimizedClearValue must be NULL
		// when D3D12_RESOURCE_DESC::Dimension is not D3D12_RESOURCE_DIMENSION_BUFFER
		// and neither D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET nor D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		// are set in D3D12_RESOURCE_DESC::Flags. [ STATE_CREATION ERROR #815: CREATERESOURCE_INVALIDCLEARVALUE]
		else
		{
			l_DX12DeviceMemory->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc);
		}

#ifdef INNO_DEBUG
		SetObjectName(l_rhs, l_DX12DeviceMemory->m_DefaultHeapBuffer, ("DefaultHeap_Texture_" + std::to_string(i)).c_str());
#endif // INNO_DEBUG

		l_rhs->m_DeviceMemories[i] = l_DX12DeviceMemory;
	}

	DX12CommandList l_commandList = {};
	l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

	if (l_rhs->m_InitialData)
	{
		if (l_rhs->m_TextureDesc.Usage == TextureUsage::Sample)
			l_rhs->m_MappedMemories.resize(1);
		else
			l_rhs->m_MappedMemories.resize(GetSwapChainImageCount());

		uint32_t l_subresourcesCount = l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		for (size_t i = 0; i < l_rhs->m_MappedMemories.size(); i++)
		{
			auto l_DX12MappedMemory = new DX12MappedMemory();
			auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[i]);
			UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), 0, l_subresourcesCount);

			auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
			l_DX12MappedMemory->m_UploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc);

#ifdef INNO_DEBUG
			SetObjectName(l_rhs, l_DX12MappedMemory->m_UploadHeapBuffer, ("UploadHeap_Texture_" + std::to_string(i)).c_str());
#endif // INNO_DEBUG

			UploadToGPU(&l_commandList, l_DX12MappedMemory, l_DX12DeviceMemory, l_rhs);

			l_rhs->m_MappedMemories[i] = l_DX12MappedMemory;
		}
	}
	else
	{
		for (size_t i = 0; i < l_rhs->m_DeviceMemories.size(); i++)
		{
			auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[i]);
			l_commandList.m_DirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, l_rhs->m_CurrentState));
		}
	}

	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	CreateSRV(l_rhs, 0);
	// if (l_rhs->m_TextureDesc.UseMipMap)
	// {
	// 	for (uint32_t TopMip = 1; TopMip < 4; TopMip++)
	// 	{
	// 		CreateSRV(l_rhs, TopMip);
	// 	}
	// }

	if (l_rhs->m_TextureDesc.Usage != TextureUsage::DepthAttachment
		&& l_rhs->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment)
	{
		if (!l_rhs->m_TextureDesc.IsSRGB)
		{
			CreateUAV(l_rhs, 0);
			// if (l_rhs->m_TextureDesc.UseMipMap)
			// {
			// 	for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
			// 	{
			// 		auto l_UAV = CreateUAV(l_rhs, TopMip + 1);
			// 	}
			// }
		}
	}

	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		GenerateMipmap(l_rhs);
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, "Texture ", l_rhs->m_InstanceName, " is initialized.");

	return true;
}

bool DX12RenderingServer::InitializeImpl(ShaderProgramComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(rhs);
#ifdef USE_DXIL
	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(l_rhs->m_VSBuffer, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(l_rhs->m_HSBuffer, l_rhs->m_ShaderFilePaths.m_HSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(l_rhs->m_DSBuffer, l_rhs->m_ShaderFilePaths.m_DSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(l_rhs->m_GSBuffer, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(l_rhs->m_PSBuffer, l_rhs->m_ShaderFilePaths.m_PSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(l_rhs->m_CSBuffer, l_rhs->m_ShaderFilePaths.m_CSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_RayGenPath != "")
	{
		LoadShaderFile(l_rhs->m_RayGenBuffer, l_rhs->m_ShaderFilePaths.m_RayGenPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		LoadShaderFile(l_rhs->m_AnyHitBuffer, l_rhs->m_ShaderFilePaths.m_AnyHitPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		LoadShaderFile(l_rhs->m_ClosestHitBuffer, l_rhs->m_ShaderFilePaths.m_ClosestHitPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_MissPath != "")
	{
		LoadShaderFile(l_rhs->m_MissBuffer, l_rhs->m_ShaderFilePaths.m_MissPath);
	}
#else
	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(&l_rhs->m_VSBuffer, ShaderStage::Vertex, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(&l_rhs->m_HSBuffer, ShaderStage::Hull, l_rhs->m_ShaderFilePaths.m_HSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(&l_rhs->m_DSBuffer, ShaderStage::Domain, l_rhs->m_ShaderFilePaths.m_DSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(&l_rhs->m_GSBuffer, ShaderStage::Geometry, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(&l_rhs->m_PSBuffer, ShaderStage::Pixel, l_rhs->m_ShaderFilePaths.m_PSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(&l_rhs->m_CSBuffer, ShaderStage::Compute, l_rhs->m_ShaderFilePaths.m_CSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_RayGenPath != "")
	{
		LoadShaderFile(&l_rhs->m_RayGenBuffer, ShaderStage::RayGen, l_rhs->m_ShaderFilePaths.m_RayGenPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		LoadShaderFile(&l_rhs->m_AnyHitBuffer, ShaderStage::AnyHit, l_rhs->m_ShaderFilePaths.m_AnyHitPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		LoadShaderFile(&l_rhs->m_ClosestHitBuffer, ShaderStage::ClosestHit, l_rhs->m_ShaderFilePaths.m_ClosestHitPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_MissPath != "")
	{
		LoadShaderFile(&l_rhs->m_MissBuffer, ShaderStage::Miss, l_rhs->m_ShaderFilePaths.m_MissPath);
	}
#endif
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(SamplerComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent*>(rhs);
	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;

	l_rhs->m_Sampler.SamplerDesc.Filter = GetFilterMode(l_rhs->m_SamplerDesc.m_MinFilterMethod, l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_rhs->m_Sampler.SamplerDesc.AddressU = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_rhs->m_Sampler.SamplerDesc.AddressV = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_rhs->m_Sampler.SamplerDesc.AddressW = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_rhs->m_Sampler.SamplerDesc.MipLODBias = 0.0f;
	l_rhs->m_Sampler.SamplerDesc.MaxAnisotropy = l_rhs->m_SamplerDesc.m_MaxAnisotropy;
	l_rhs->m_Sampler.SamplerDesc.BorderColor[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_rhs->m_Sampler.SamplerDesc.MinLOD = l_rhs->m_SamplerDesc.m_MinLOD;
	l_rhs->m_Sampler.SamplerDesc.MaxLOD = l_rhs->m_SamplerDesc.m_MaxLOD;

	l_rhs->m_Sampler.Handle = m_SamplerDescHeapAccessor.GetNewHandle();
	m_device->CreateSampler(&l_rhs->m_Sampler.SamplerDesc, l_rhs->m_Sampler.Handle.CPUHandle);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

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
		tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

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
	auto l_transformMatrix = rhs->m_TransformComponent->m_globalTransformMatrix.m_transformationMat;

	for (size_t i = 0; i < GetSwapChainImageCount(); i++)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		instanceDesc.Transform[0][0] = l_transformMatrix.m00;
		instanceDesc.Transform[0][1] = l_transformMatrix.m01;
		instanceDesc.Transform[0][2] = l_transformMatrix.m02;
		instanceDesc.Transform[0][3] = l_transformMatrix.m03; // Translation X

		instanceDesc.Transform[1][0] = l_transformMatrix.m10;
		instanceDesc.Transform[1][1] = l_transformMatrix.m11;
		instanceDesc.Transform[1][2] = l_transformMatrix.m12;
		instanceDesc.Transform[1][3] = l_transformMatrix.m13; // Translation Y

		instanceDesc.Transform[2][0] = l_transformMatrix.m20;
		instanceDesc.Transform[2][1] = l_transformMatrix.m21;
		instanceDesc.Transform[2][2] = l_transformMatrix.m22;
		instanceDesc.Transform[2][3] = l_transformMatrix.m23; // Translation Z

		instanceDesc.InstanceID = static_cast<UINT>(rhs->m_RenderableSet->material->m_ShaderModel);
		instanceDesc.InstanceMask = 0xFF;
		instanceDesc.InstanceContributionToHitGroupIndex = 0;
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		auto l_mesh = reinterpret_cast<DX12MeshComponent*>(rhs->m_RenderableSet->mesh);
		instanceDesc.AccelerationStructure = l_mesh->m_BLAS->GetGPUVirtualAddress();
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
		l_descList->m_Descs.emplace_back(instanceDesc);
	}

	Log(Verbose, rhs->m_InstanceName, " Raytracing instance is registered.");

	m_RegisteredCollisionComponents.emplace(rhs);

	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_DX12CommandList = l_commandList->m_DirectCommandList;

	if (l_rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	l_DX12CommandList->CopyResource(l_rhs->m_DefaultHeapBuffer_VB.Get(), l_rhs->m_UploadHeapBuffer_VB.Get());
	l_DX12CommandList->CopyResource(l_rhs->m_DefaultHeapBuffer_IB.Get(), l_rhs->m_UploadHeapBuffer_IB.Get());
	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, TextureComponent* rhs)
{
	if (!rhs->m_InitialData)
		return true;

	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);

	auto l_currentFrame = GetCurrentFrame();
	auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_rhs->m_MappedMemories[l_currentFrame]);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[l_currentFrame]);

	return UploadToGPU(l_commandList, l_mappedMemory, l_deviceMemory, l_rhs);
}

bool DX12RenderingServer::UploadToGPU(DX12CommandList* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, DX12TextureComponent* TextureComponent)
{
	if (!deviceMemory->m_DefaultHeapBuffer)
		return true;

	auto l_DX12CommandList = commandList->m_DirectCommandList;

	if (TextureComponent->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), TextureComponent->m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
	l_textureSubResourceData.RowPitch = TextureComponent->m_TextureDesc.Width * TextureComponent->m_PixelDataSize;
	l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * TextureComponent->m_TextureDesc.Height;
	l_textureSubResourceData.pData = (unsigned char*)TextureComponent->m_InitialData;
	uint32_t l_subresourcesCount = TextureComponent->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;

	UpdateSubresources(l_DX12CommandList.Get(), deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get(), 0, 0, l_subresourcesCount, &l_textureSubResourceData);
	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(deviceMemory->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, TextureComponent->m_CurrentState));

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