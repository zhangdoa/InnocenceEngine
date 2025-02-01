#include "DX12RenderingServer.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::InitializeImpl(MeshComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);

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
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_IB, "UploadHeap_IB");
#endif //  INNO_DEBUG

	// Initialize the index buffer view.
	l_rhs->m_IBV.Format = DXGI_FORMAT_R32_UINT;
	l_rhs->m_IBV.BufferLocation = l_rhs->m_DefaultHeapBuffer_IB->GetGPUVirtualAddress();
	l_rhs->m_IBV.SizeInBytes = l_indicesDataSize;

	Log(Verbose, l_rhs->m_InstanceName, " Index Buffer is initialized.");

	// Flip y texture coordinate
	for (auto &i : rhs->m_Vertices)
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
	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	// @TODO: Reset the upload heap buffers.
	return true;
}

bool DX12RenderingServer::InitializeImpl(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_DX12TextureDesc = GetDX12TextureDesc(l_rhs->m_TextureDesc);
	l_rhs->m_PixelDataSize = GetTexturePixelDataSize(l_rhs->m_TextureDesc);
	l_rhs->m_WriteState = GetTextureWriteState(l_rhs->m_TextureDesc);
	l_rhs->m_ReadState = GetTextureReadState(l_rhs->m_TextureDesc);

	l_rhs->m_CurrentState = l_rhs->m_ReadState;

	// Create the empty texture.
	D3D12_CLEAR_VALUE l_clearValue = {};
	if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, &l_clearValue);
	}
	else if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, &l_clearValue);
	}
	else if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_clearValue.Format = l_rhs->m_DX12TextureDesc.Format;
		l_clearValue.Color[0] = l_rhs->m_TextureDesc.ClearColor[0];
		l_clearValue.Color[1] = l_rhs->m_TextureDesc.ClearColor[1];
		l_clearValue.Color[2] = l_rhs->m_TextureDesc.ClearColor[2];
		l_clearValue.Color[3] = l_rhs->m_TextureDesc.ClearColor[3];
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, &l_clearValue);
	}
	// It has to be written like this because:
	// pOptimizedClearValue must be NULL
	// when D3D12_RESOURCE_DESC::Dimension is not D3D12_RESOURCE_DIMENSION_BUFFER
	// and neither D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET nor D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	// are set in D3D12_RESOURCE_DESC::Flags. [ STATE_CREATION ERROR #815: CREATERESOURCE_INVALIDCLEARVALUE]
	else
	{
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc);
	}

	if (l_rhs->m_DefaultHeapBuffer == nullptr)
	{
		Log(Error, "Can't create texture: ", l_rhs->m_InstanceName);
		return false;
	}

#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_Texture");
#endif // INNO_DEBUG

	DX12CommandList l_commandList = {};
	l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
	
	if (l_rhs->m_TextureData)
	{
		uint32_t l_subresourcesCount = l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_DefaultHeapBuffer.Get(), 0, l_subresourcesCount);

		l_rhs->m_UploadHeapBuffers.resize(l_subresourcesCount);
		for (uint32_t i = 0; i < l_subresourcesCount; i++)
		{
			auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
			l_rhs->m_UploadHeapBuffers[i] = CreateUploadHeapBuffer(&l_resourceDesc);

#ifdef INNO_DEBUG
	auto l_name = "UploadHeap_Texture_" + std::to_string(i);
	SetObjectName(l_rhs, l_rhs->m_UploadHeapBuffers[i], l_name.c_str());
#endif // INNO_DEBUG
		}

		UploadToGPU(&l_commandList, l_rhs);
	}
	else
	{
		l_commandList.m_DirectCommandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON,
				l_rhs->m_CurrentState));
	}

	ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	// Create SRV and UAV
	l_rhs->m_SRV = CreateSRV(l_rhs, 0);
	// if (l_rhs->m_TextureDesc.UseMipMap)
	// {
	// 	for (uint32_t TopMip = 1; TopMip < 4; TopMip++)
	// 	{
	// 		auto l_SRV = CreateSRV(l_rhs, TopMip);
	// 	}
	// }

	if (l_rhs->m_TextureDesc.Usage != TextureUsage::DepthAttachment 
	&& l_rhs->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment)
	{
		if (!l_rhs->m_TextureDesc.IsSRGB)
		{
			l_rhs->m_UAV = CreateUAV(l_rhs, 0);
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

bool DX12RenderingServer::InitializeImpl(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent *>(rhs);
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
#endif
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeImpl(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent *>(rhs);
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

bool DX12RenderingServer::InitializeImpl(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize);
	l_rhs->m_UploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc);

#ifdef INNO_DEBUG
	SetObjectName(rhs, l_rhs->m_UploadHeapBuffer, "UploadHeap_General");
#endif // INNO_DEBUG

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc);
#ifdef INNO_DEBUG
			SetObjectName(rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_General");
#endif // INNO_DEBUG

			l_rhs->m_UAV = CreateUAV(l_rhs);
		}
		else
		{
			Log(Warning, "Not support CPU-readable default heap GPU buffer currently.");
		}
	}

	l_rhs->m_SRV = CreateSRV(l_rhs);

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer->Map(0, &m_readRange, &l_rhs->m_MappedMemory);

	if (l_rhs->m_InitialData)
	{
		WriteMappedMemory(l_rhs, l_rhs->m_InitialData, 0, l_rhs->m_TotalSize);
		l_rhs->m_NeedUploadToGPU = false;
		
		DX12CommandList l_commandList = {};
		l_commandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

		UploadToGPU(&l_commandList, l_rhs);
		ExecuteCommandListAndWait(l_commandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

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
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_DX12CommandList = l_commandList->m_DirectCommandList;

	if (!l_rhs->m_TextureData)
		return true;

	if (l_rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	for (uint32_t i = 0; i < l_rhs->m_UploadHeapBuffers.size(); i++)
	{
		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		l_textureSubResourceData.RowPitch = l_rhs->m_TextureDesc.Width * l_rhs->m_PixelDataSize;
		l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height;
		l_textureSubResourceData.pData = (unsigned char*)l_rhs->m_TextureData + l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height * i;

		UpdateSubresources(l_DX12CommandList.Get(), l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffers[i].Get(), 0, i, 1, &l_textureSubResourceData);
	}

	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));

	return true;
}

bool DX12RenderingServer::UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent*>(rhs);
	if (!l_rhs->m_DefaultHeapBuffer)
		return true;

	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_DX12CommandList = l_commandList->m_DirectCommandList;

	if (l_rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	l_DX12CommandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffer.Get());

	l_DX12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	return true;
}