#include "DX12RenderingServer.h"

#include "DX12GraphicsDevice.h"
#include "DX12RenderingComponentPool.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

using namespace DX12Helper;

#ifdef max
#undef max
#endif

#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"

#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"

#include "../../Engine.h"

using namespace Inno;

bool DX12RenderingServer::Setup(ISystemConfig *systemConfig)
{
	auto l_result = IRenderingServer::Setup(systemConfig);

	m_DX12Device = static_cast<DX12GraphicsDevice *>(m_DX12Device);

	return true;
}

bool DX12RenderingServer::Terminate()
{
	auto l_result = IRenderingServer::Terminate();

	m_DX12Device = nullptr;

	return l_result;
}

bool DX12RenderingServer::InitializeMeshComponent(MeshComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * l_rhs->m_Vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);
	l_rhs->m_DefaultHeapBuffer_VB = m_DX12Device->CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (l_rhs->m_DefaultHeapBuffer_VB == nullptr)
	{
		Log(Error, "can't create vertex buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_VB, "DefaultHeap_VB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_VB = m_DX12Device->CreateUploadHeapBuffer(&l_verticesResourceDesc);
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

	Log(Verbose, "Vertex Buffer ", l_rhs->m_DefaultHeapBuffer_VB, " is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * l_rhs->m_Indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);
	l_rhs->m_DefaultHeapBuffer_IB = m_DX12Device->CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (l_rhs->m_DefaultHeapBuffer_IB == nullptr)
	{
		Log(Error, "can't create index buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_IB, "DefaultHeap_IB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_IB = m_DX12Device->CreateUploadHeapBuffer(&l_indicesResourceDesc);
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

	Log(Verbose, "Index Buffer ", l_rhs->m_DefaultHeapBuffer_IB, " is initialized.");

	UpdateMeshComponent(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	// @TODO: Reset the upload heap buffers.
	return true;
}

bool DX12RenderingServer::InitializeTextureComponent(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	l_rhs->m_DX12TextureDesc = GetDX12TextureDesc(l_rhs->m_TextureDesc);
	l_rhs->m_PixelDataSize = GetTexturePixelDataSize(l_rhs->m_TextureDesc);
	l_rhs->m_WriteState = GetTextureWriteState(l_rhs->m_TextureDesc);
	l_rhs->m_ReadState = GetTextureReadState(l_rhs->m_TextureDesc);

	if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment
	 || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment
	  || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_rhs->m_CurrentState = l_rhs->m_WriteState;
	}
	else
	{
		l_rhs->m_CurrentState = l_rhs->m_ReadState;
	}

	// Create the empty texture.
	D3D12_CLEAR_VALUE l_clearValue;
	if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
	}
	else if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
	}
	else if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_clearValue.Format = l_rhs->m_DX12TextureDesc.Format;
		l_clearValue.Color[0] = l_rhs->m_TextureDesc.ClearColor[0];
		l_clearValue.Color[1] = l_rhs->m_TextureDesc.ClearColor[1];
		l_clearValue.Color[2] = l_rhs->m_TextureDesc.ClearColor[2];
		l_clearValue.Color[3] = l_rhs->m_TextureDesc.ClearColor[3];
	}

	l_rhs->m_DefaultHeapBuffer = m_DX12Device->CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, &l_clearValue);
	if (l_rhs->m_DefaultHeapBuffer == nullptr)
	{
		Log(Error, "Can't create texture: ", l_rhs->m_InstanceName);
		return false;
	}

#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_Texture");
#endif // INNO_DEBUG

	auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);
	
	if (l_rhs->m_TextureData)
	{
		uint32_t l_subresourcesCount = l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_DefaultHeapBuffer.Get(), 0, l_subresourcesCount);

		l_rhs->m_UploadHeapBuffers.resize(l_subresourcesCount);
		for (uint32_t i = 0; i < l_subresourcesCount; i++)
		{
			D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
			l_textureSubResourceData.RowPitch = l_rhs->m_TextureDesc.Width * l_rhs->m_PixelDataSize;
			l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height;
			l_textureSubResourceData.pData = (unsigned char*)l_rhs->m_TextureData + l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height * i;

			auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
			l_rhs->m_UploadHeapBuffers[i] = m_DX12Device->CreateUploadHeapBuffer(&l_resourceDesc);
			UpdateSubresources(l_commandList.Get(), l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffers[i].Get(), 0, i, 1, &l_textureSubResourceData);
		}
	}

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));
	m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	
	// Create SRV and UAV
	l_rhs->m_SRV = m_DX12Device->CreateSRV(l_rhs, 0);
	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		for (uint32_t TopMip = 1; TopMip < 4; TopMip++)
		{
			auto l_SRV = m_DX12Device->CreateSRV(l_rhs, TopMip);
		}
	}

	if (l_rhs->m_TextureDesc.Usage != TextureUsage::DepthAttachment 
	&& l_rhs->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment)
	{
		if (!l_rhs->m_TextureDesc.IsSRGB)
		{
			l_rhs->m_UAV = m_DX12Device->CreateUAV(l_rhs, 0);
			if (l_rhs->m_TextureDesc.UseMipMap)
			{
				for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
				{
					auto l_UAV = m_DX12Device->CreateUAV(l_rhs, TopMip + 1);
				}
			}
		}
	}

	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		GenerateMipmap(l_rhs);
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, "Texture ", l_rhs->m_InstanceName, " is initialized.");

	return true;
}

// @TODO: Promote this function to the base class.
bool DX12RenderingServer::InitializeRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);

	bool l_result = true;

	l_result &= m_RenderingComponentPool->ReserveRenderTargets(l_rhs);

	l_result &= CreateRenderTargets(l_rhs);

	if(l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= m_DX12Device->CreateRTV(l_rhs);

		l_result &= m_DX12Device->CreateDSV(l_rhs);
	}

	l_result &= m_DX12Device->CreateRootSignature(l_rhs);

	l_rhs->m_PipelineStateObject = m_RenderingComponentPool->AddPipelineStateObject();

	l_result &= m_DX12Device->CreatePSO(l_rhs);

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_rhs->m_CommandLists.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		l_rhs->m_CommandLists.resize(1);
	}

	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		l_rhs->m_CommandLists[i] = m_RenderingComponentPool->AddCommandList();
	}

    auto l_tempName = std::string(l_rhs->m_InstanceName.c_str());
    auto l_tempNameL = std::wstring(l_tempName.begin(), l_tempName.end());

    for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
    {
        auto l_CommandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[i]);
        m_DX12Device->CreateCommandList(l_CommandList, i, l_tempNameL);
    }

	Log(Verbose, l_rhs->m_InstanceName, " CommandList has been created.");

	l_rhs->m_Semaphores.resize(l_rhs->m_CommandLists.size());
	for (size_t i = 0; i < l_rhs->m_Semaphores.size(); i++)
	{
		l_rhs->m_Semaphores[i] = m_RenderingComponentPool->AddSemaphore();
	}
	
	Log(Verbose, l_rhs->m_InstanceName, " Semaphore has been created.");

	m_DX12Device->CreateFenceEvents(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool DX12RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent *rhs)
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

bool DX12RenderingServer::InitializeSamplerComponent(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent *>(rhs);

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

	l_rhs->m_Sampler.Handle = m_DX12Device->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->GetNewHandle();
	m_DX12Device->GetDevice()->CreateSampler(&l_rhs->m_Sampler.SamplerDesc, l_rhs->m_Sampler.Handle.CPUHandle);

	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeGPUBufferComponent(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize);
	l_rhs->m_UploadHeapBuffer = m_DX12Device->CreateUploadHeapBuffer(&l_resourceDesc);

#ifdef INNO_DEBUG
	SetObjectName(rhs, l_rhs->m_UploadHeapBuffer, "UploadHeap_General");
#endif // INNO_DEBUG

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			l_rhs->m_DefaultHeapBuffer = m_DX12Device->CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc);
#ifdef INNO_DEBUG
			SetObjectName(rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_General");
#endif // INNO_DEBUG

			auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
			l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);
			l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

			l_rhs->m_UAV = m_DX12Device->CreateUAV(l_rhs);
		}
		else
		{
			Log(Warning, "Not support CPU-readable default heap GPU buffer currently.");
		}
	}

	l_rhs->m_SRV = m_DX12Device->CreateSRV(l_rhs);

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer->Map(0, &m_readRange, &l_rhs->m_MappedMemory);

	if (l_rhs->m_InitialData)
	{
		UploadGPUBufferComponent(l_rhs, l_rhs->m_InitialData);
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::UpdateMeshComponent(MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);
	
	// Flip y texture coordinate
	for (auto &i : rhs->m_Vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer_VB->Map(0, &m_readRange, &l_rhs->m_MappedUploadHeapBuffer_VB);
	std::memcpy((char *)l_rhs->m_MappedUploadHeapBuffer_VB, &l_rhs->m_Vertices[0], l_rhs->m_Vertices.size() * sizeof(Vertex));

	l_rhs->m_UploadHeapBuffer_IB->Map(0, &m_readRange, &l_rhs->m_MappedUploadHeapBuffer_IB);
	std::memcpy((char *)l_rhs->m_MappedUploadHeapBuffer_IB, &l_rhs->m_Indices[0], l_rhs->m_Indices.size() * sizeof(Index));

	auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	if(l_rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
		l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer_VB.Get(), l_rhs->m_UploadHeapBuffer_VB.Get());
	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer_IB.Get(), l_rhs->m_UploadHeapBuffer_IB.Get());
	l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::ClearTextureComponent(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);
	
	ID3D12DescriptorHeap *l_heaps[] = { m_DX12Device->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap().Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);
	
	if(l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	if (l_rhs->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
	{
		l_commandList->ClearUnorderedAccessViewUint(
			l_rhs->m_UAV.Handle.GPUHandle,
			l_rhs->m_UAV.Handle.CPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			(UINT *)&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}
	else
	{
		l_commandList->ClearUnorderedAccessViewFloat(
			l_rhs->m_UAV.Handle.GPUHandle,
			l_rhs->m_UAV.Handle.CPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}

	if(l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, l_rhs->m_CurrentState));

	m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::CopyTextureComponent(TextureComponent *lhs, TextureComponent *rhs)
{
	auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	auto l_src = reinterpret_cast<DX12TextureComponent *>(lhs);
	auto l_dest = reinterpret_cast<DX12TextureComponent *>(rhs);

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), l_src->m_CurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), l_dest->m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST));

	l_commandList->CopyResource(l_dest->m_DefaultHeapBuffer.Get(), l_src->m_DefaultHeapBuffer.Get());

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, l_src->m_CurrentState));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_dest->m_CurrentState));

	m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::ClearGPUBufferComponent(GPUBufferComponent *rhs)
{
	const uint32_t zero = 0;
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	ID3D12DescriptorHeap *l_heaps[] = { m_DX12Device->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap().Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	l_commandList->ClearUnorderedAccessViewUint(
		l_rhs->m_UAV.Handle.GPUHandle,
		l_rhs->m_UAV.Handle.CPUHandle,
		l_rhs->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

void DX12RenderingServer::PushRootConstants(RenderPassComponent* rhs, size_t rootConstants)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList->m_DirectCommandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandList->m_ComputeCommandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
	}
}

uint32_t DX12RenderingServer::GetIndex(GPUResourceComponent* rhs)
{
	return 0;
	//return rhs->m_DescriptorHeapSlot;
}

bool DX12RenderingServer::Bind(RenderPassComponent *renderPass, GPUResourceComponent* resource, ShaderStage shaderStage, uint32_t rootParameterIndex, Accessibility bindingAccessibility)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (resource->m_GPUResourceType == GPUResourceType::Buffer)
	{
		auto l_buffer = reinterpret_cast<DX12GPUBufferComponent *>(resource);
		auto& l_SRV = l_buffer->m_SRV;
		auto& l_UAV = l_buffer->m_UAV;
		if (shaderStage == ShaderStage::Compute)
		{
			if (bindingAccessibility == Accessibility::ReadOnly)
			{
				if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
				{
					auto l_GPUVirtualAddress = l_buffer->m_UploadHeapBuffer->GetGPUVirtualAddress();
					l_commandList->m_ComputeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				}
				else if ((l_buffer->m_GPUAccessibility.CanWrite()))
				{
					l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_SRV.Handle.GPUHandle);
				}
				else
				{
					assert(false);
					return false;
				}
			}
			else if (bindingAccessibility.CanWrite())
			{
				if (l_buffer->m_GPUAccessibility.CanWrite())
				{
					l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_UAV.Handle.GPUHandle);
				}
				else
				{
					assert(false);
					return false;
				}
			}
			else
			{
				assert(false);
				return false;
			}
		}
		else
		{
			if (bindingAccessibility == Accessibility::ReadOnly)
			{
				if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
				{
					auto l_GPUVirtualAddress = l_buffer->m_UploadHeapBuffer->GetGPUVirtualAddress();
					l_commandList->m_DirectCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				}
				else if (l_buffer->m_GPUAccessibility.CanWrite())
				{
					l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_SRV.Handle.GPUHandle);
				}
				else
				{
					assert(false);
					return false;
				}
			}
			else
			{
				l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_UAV.Handle.GPUHandle);
			}
		}
	}
	else if (resource->m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<DX12SamplerComponent*>(resource)->m_Sampler.Handle.GPUHandle;
		if (shaderStage == ShaderStage::Compute)
		{
			l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_handle);
		}
		else
		{
			l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_handle);
		}
	}
	else
	{
		assert(false);
		return false;
	}

	return true;
}

bool DX12RenderingServer::CommandListBegin(RenderPassComponent *rhs, size_t frameIndex)
{
	if (rhs->m_RenderPassDesc.m_UseMultiFrames)
		rhs->m_CurrentFrame = m_Device->GetCurrentFrame();
	else
		rhs->m_CurrentFrame = frameIndex;

	auto l_commandList = reinterpret_cast<DX12CommandList *>(rhs->m_CommandLists[rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(rhs->m_PipelineStateObject);

	l_commandList->m_DirectCommandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), l_PSO->m_PSO.Get());
	l_commandList->m_ComputeCommandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get(), l_PSO->m_PSO.Get());

	return true;
}

bool DX12RenderingServer::BindRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	m_DX12Device->PrepareRenderTargets(l_rhs, l_commandList);
	m_DX12Device->SetDescriptorHeaps(l_rhs, l_commandList);
	m_DX12Device->SetRenderTargets(l_rhs, l_commandList);

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_rhs->m_PipelineStateObject);
	m_DX12Device->PreparePipeline(l_rhs, l_commandList, l_PSO);

	for	(uint32_t i = 0; i < l_rhs->m_ResourceBindingLayoutDescs.size(); i++)
	{
		auto& l_desc = l_rhs->m_ResourceBindingLayoutDescs[i];
		if(!l_desc.m_GPUResource)
			continue;

		auto l_GPUBuffer = reinterpret_cast<DX12GPUBufferComponent *>(l_desc.m_GPUResource);
		Bind(rhs, l_GPUBuffer, l_desc.m_ShaderStage, i, l_desc.m_BindingAccessibility);
	}
	
	if (l_rhs->m_CustomCommandsFunc)
		l_rhs->m_CustomCommandsFunc(l_commandList);

	return true;
}

bool DX12RenderingServer::ClearRenderTargets(RenderPassComponent *rhs, size_t index)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics && rhs->m_RenderPassDesc.m_RenderTargetCount)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
		auto f_clearRTAsUAV = [&](DX12TextureComponent* l_RT)
		{
			if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
					l_RT->m_UAV.Handle.GPUHandle,
					l_RT->m_UAV.Handle.CPUHandle,
					l_RT->m_DefaultHeapBuffer.Get(),
					(UINT*)l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0,
					NULL);
			}
			else
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewFloat(
					l_RT->m_UAV.Handle.GPUHandle,
					l_RT->m_UAV.Handle.CPUHandle,
					l_RT->m_DefaultHeapBuffer.Get(),
					l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0,
					NULL);
			}
		};

		if (l_rhs->m_RenderPassDesc.m_UseOutputMerger)
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[l_rhs->m_CurrentFrame], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
			else
			{
				if(index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[index], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
				}
				else
				{
					for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
					{
						l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[i], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
					}
				}
			}
		}
		else
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame].m_Texture));
			}
			else
			{
				if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(index));
				}
				else
				{

					for (auto i : l_rhs->m_RenderTargets)
					{
						f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(i.m_Texture));
					}
				}
			}
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
			if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			{
				l_flag |= D3D12_CLEAR_FLAG_STENCIL;
			}
			l_commandList->m_DirectCommandList->ClearDepthStencilView(l_rhs->m_DSVDescCPUHandle, l_flag, 1.0f, 0x00, 0, nullptr);
		}
	}

	return true;
}

bool DX12RenderingServer::TryToTransitState(TextureComponent *rhs, ICommandList *commandList, Accessibility accessibility)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);
	auto l_newState = accessibility == Accessibility::ReadOnly ? l_rhs->m_ReadState : l_rhs->m_WriteState;
	if (l_rhs->m_CurrentState == l_newState)
		return false;

	auto l_commandList = reinterpret_cast<DX12CommandList *>(commandList);
	auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, l_newState);
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &l_transition);
	l_rhs->m_CurrentState = l_newState;
	return true;
}

bool DX12RenderingServer::DrawIndexedInstanced(RenderPassComponent *renderPass, MeshComponent *mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<DX12MeshComponent *>(mesh);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, &l_mesh->m_VBV);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(&l_mesh->m_IBV);
	l_commandList->m_DirectCommandList->DrawIndexedInstanced((uint32_t)l_mesh->m_IndexCount, (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DrawInstanced(RenderPassComponent *renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_renderPass->m_PipelineStateObject);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, nullptr);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(nullptr);
	l_commandList->m_DirectCommandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	for	(size_t i = 0; i < l_rhs->m_ResourceBindingLayoutDescs.size(); i++)
	{
		auto& l_desc = l_rhs->m_ResourceBindingLayoutDescs[i];
		if(l_desc.m_GPUResource)
		{
			UnbindGPUResource(rhs, l_desc.m_ShaderStage, l_desc.m_GPUResource, i);
		}
	}

	l_commandList->m_DirectCommandList->Close();
	l_commandList->m_ComputeCommandList->Close();

	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return m_DX12Device->ExecuteCommandList(l_commandList, l_semaphore, GPUEngineType);
}

bool DX12RenderingServer::WaitCommandQueue(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return m_DX12Device->WaitCommandQueue(l_semaphore, queueType, semaphoreType);
}

bool DX12RenderingServer::WaitFence(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return m_DX12Device->WaitFence(l_semaphore, GPUEngineType);
}

bool DX12RenderingServer::Dispatch(RenderPassComponent *renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	l_commandList->m_ComputeCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassComponent *rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassComponent *canvas, TextureComponent *TextureComp)
{
	// @TODO: Support different pixel data type
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(TextureComp);
	auto l_srcDesc = l_rhs->m_DefaultHeapBuffer->GetDesc();

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints;
	l_footprints.resize(l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : l_rhs->m_TextureDesc.DepthOrArraySize);

	m_DX12Device->GetDevice()->GetCopyableFootprints(&l_srcDesc, 0, (UINT)l_footprints.size(), 0, l_footprints.data(), NULL, NULL, NULL);

	if (!l_rhs->m_ReadBackHeapBuffer)
	{
		UINT64 bufferSize = 0;
		for (size_t i = 0; i < l_footprints.size(); ++i)
		{
			bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;
		}

		l_rhs->m_ReadBackHeapBuffer = m_DX12Device->CreateReadBackHeapBuffer(bufferSize);
#ifdef INNO_DEBUG
		SetObjectName(l_rhs, l_rhs->m_ReadBackHeapBuffer, "ReadBackHeap_Texture");
#endif // INNO_DEBUG
	}

	auto f_DefaultToReadbackHeap = [this](ComPtr<ID3D12Resource> l_defaultHeapBuffer, ComPtr<ID3D12Resource> l_readbackHeapBuffer, const std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, DXGI_FORMAT l_format, D3D12_RESOURCE_STATES currentState)
	{
		{
			auto l_commandList = m_DX12Device->CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					currentState,
					D3D12_RESOURCE_STATE_COMMON));
			m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
		}

		for (size_t i = 0; i < footprints.size(); i++)
		{
			D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
			l_srcLocation.pResource = l_defaultHeapBuffer.Get();
			l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			l_srcLocation.SubresourceIndex = (UINT)i;

			D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
			l_destLocation.pResource = l_readbackHeapBuffer.Get();
			l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			l_destLocation.PlacedFootprint = footprints[i];
			l_destLocation.PlacedFootprint.Footprint.Format = l_format;

			auto l_commandList = m_DX12Device->CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY));

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_COPY_SOURCE));

			l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COPY_SOURCE,
					D3D12_RESOURCE_STATE_COMMON));

			m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY));
		}

		{
			auto l_commandList = m_DX12Device->CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COMMON,
					currentState));
			m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
		}
	};

	size_t l_pixelCount = 0;
	
	auto textureDesc = l_rhs->m_TextureDesc;
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

	auto f_ReadbackToHostHeap = [](ComPtr<ID3D12Resource> l_readbackHeapBuffer, uint32_t l_pixelDataSize, size_t l_pixelCount) -> std::vector<unsigned char>
	{
		std::vector<unsigned char> l_result;
		l_result.resize(l_pixelCount * l_pixelDataSize);

		CD3DX12_RANGE m_ReadRange(0, l_result.size());
		void *l_pData;
		auto l_HResult = l_readbackHeapBuffer->Map(0, &m_ReadRange, &l_pData);

		if (FAILED(l_HResult))
		{
			Log(Error, "Can't map texture for CPU to read!");
		}

		std::memcpy(l_result.data(), l_pData, l_result.size());
		l_readbackHeapBuffer->Unmap(0, 0);

		return l_result;
	};

	// Copy from default heap to readback heap, then copy from readback heap to application's heap region
	DXGI_FORMAT l_format;
	std::vector<Vec4> l_result;

	if (textureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R8_TYPELESS for the stencil
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);
		auto l_pixelCount = l_rawResult.size() / 4;
		
		std::vector<uint32_t> l_resultUint32;
		l_resultUint32.resize(l_pixelCount);
		l_result.resize(l_pixelCount);

		std::memcpy(l_resultUint32.data(), l_rawResult.data(), l_rawResult.size());
		for (size_t i = 0; i < l_pixelCount; i++)
		{
			auto l_value = l_resultUint32[i];
			auto l_depth = l_value & 0x00FFFFFF;
			auto l_stencil = l_value & 0xFF000000;
			l_result[i].x = float(l_depth) / float(0x00FFFFFF);
			l_result[i].y = float(l_stencil);
		}
	}
	else
	{
		l_format = l_rhs->m_DX12TextureDesc.Format;
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

		l_result.resize(l_pixelCount);
		if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			for (int i = 0; i < l_pixelCount; ++i)
			{
				const unsigned char *pixelData = &l_rawResult[i * 8];

				// Convert the RGBA FLOAT16 data to float values
				float floatData[4];
				for (int j = 0; j < 4; ++j)
				{
					const unsigned char *floatBytes = &pixelData[j * 2];
					unsigned short float16;
					memcpy(&float16, floatBytes, sizeof(unsigned short));
					floatData[j] = Math::float16ToFloat32(float16);
				}

				// Construct a Vec4 from the float values
				l_result[i] = Vec4(floatData[0], floatData[1], floatData[2], floatData[3]);
			}
		}
		else	
		{
			std::memcpy(l_result.data(), l_rawResult.data(), l_rawResult.size());
		}
	}

	return l_result;
}

// @TODO: This is expensive, it should be running entirely on the GPU
bool DX12RenderingServer::GenerateMipmap(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	if(l_rhs->m_TextureDesc.IsSRGB)
	{
		auto l_copy = reinterpret_cast<DX12TextureComponent*>(m_RenderingComponentPool->AddTextureComponent((l_rhs->m_InstanceName.c_str() + std::string("_MipCopy/")).c_str()));
		l_copy->m_TextureDesc = l_rhs->m_TextureDesc;
		l_copy->m_TextureData = l_rhs->m_TextureData;
		l_copy->m_TextureDesc.IsSRGB = false;
		InitializeTextureComponent(l_copy);

		D3D12_RESOURCE_BARRIER barrier[2] = {};
		barrier[0].Type = barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier[0].Transition.Subresource = barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier[0].Transition.pResource = l_copy->m_DefaultHeapBuffer.Get();
		barrier[0].Transition.StateBefore = l_copy->m_CurrentState;
		barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		barrier[1].Transition.pResource = l_rhs->m_DefaultHeapBuffer.Get();
		barrier[1].Transition.StateBefore = l_rhs->m_CurrentState;
		barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		auto l_commandList = m_DX12Device->GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
		l_commandList->Reset(m_DX12Device->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

		l_commandList->ResourceBarrier(2, barrier);

		// Copy the entire resource back
		l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_copy->m_DefaultHeapBuffer.Get());
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));

		m_DX12Device->ExecuteCommandListAndWait(l_commandList, m_DX12Device->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

		return true;
	}

	return m_DX12Device->GenerateMipmap(l_rhs);
}