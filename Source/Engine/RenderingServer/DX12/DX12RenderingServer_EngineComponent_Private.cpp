#include "DX12RenderingServer.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

#ifdef max
#undef max
#endif

#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

DX12DescriptorHeapAccessor DX12RenderingServer::CreateDescriptorHeapAccessor(ComPtr<ID3D12DescriptorHeap> descHeap, D3D12_DESCRIPTOR_HEAP_DESC desc
	, uint32_t maxDescriptors, uint32_t descriptorSize, const DescriptorHandle& firstHandle, bool shaderVisible, const wchar_t* name)
{
	DX12DescriptorHeapAccessor l_descHeapAccessor = {};
	l_descHeapAccessor.m_Desc.m_HeapDesc = desc;
	l_descHeapAccessor.m_Desc.m_MaxDescriptors = maxDescriptors;
	l_descHeapAccessor.m_Desc.m_DescriptorSize = descriptorSize;
	l_descHeapAccessor.m_Desc.m_ShaderVisible = shaderVisible;
	l_descHeapAccessor.m_Desc.m_Name = name;
	l_descHeapAccessor.m_OffsetFromHeapStart = firstHandle.m_CPUHandle - descHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	l_descHeapAccessor.m_OffsetFromHeapStart /= descriptorSize;
	l_descHeapAccessor.m_FirstHandle = firstHandle;
	l_descHeapAccessor.m_CurrentHandle = firstHandle;

	l_descHeapAccessor.m_Heap = descHeap;

	Log(Verbose, "Descriptor heap accessor ", name, " has been created.");

	return l_descHeapAccessor;
}

bool DX12RenderingServer::CreateSRV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetSRVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadOnly, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);

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
		m_device->CreateShaderResourceView(resource,
			&l_desc,
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_ReadHandles[handleIndex].m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetUAVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage, false);

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

		m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateSRV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_DX12DeviceMemory->m_SRV.Handle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_DX12DeviceMemory->m_SRV.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");			
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::Invalid, false);

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

		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });

		l_DX12DeviceMemory->m_UAV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateCBV(GPUBufferComponent* gpuBuffer)
{
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadOnly);

	for (auto i : gpuBuffer->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		DX12CBV l_result;

		l_result.CBVDesc.BufferLocation = l_DX12MappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
		l_result.CBVDesc.SizeInBytes = (uint32_t)gpuBuffer->m_ElementSize;
		l_result.Handle = l_descHeapAccessor.GetNewHandle();

		m_device->CreateConstantBufferView(&l_result.CBVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_result.Handle.m_CPUHandle });

		l_DX12MappedMemory->m_CBV = l_result;
		
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");		
	}

	return true;
}

bool DX12RenderingServer::CreateRootSignature(RenderPassComponent* RenderPassComp)
{
	if (RenderPassComp->m_ResourceBindingLayoutDescs.empty())
	{
		Log(Verbose, "Skipping creating RootSignature for ", RenderPassComp->m_InstanceName);
		return true;
	}

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
	l_HResult = m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&l_PSO->m_RootSignature));

	if (FAILED(l_HResult))
	{
		Log(Error, RenderPassComp->m_InstanceName, " Can't create RootSignature.");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(RenderPassComp, l_PSO->m_RootSignature, "RootSignature");
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

		l_HResult = m_device->CreateCommandSignature(&commandSignatureDesc, l_PSO->m_RootSignature.Get(), IID_PPV_ARGS(&l_PSO->m_IndirectCommandSignature));
		if (FAILED(l_HResult))
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create CommandSignature.");
			return false;
		}

		Log(Verbose, RenderPassComp->m_InstanceName, " CommandSignature has been created.");
	}

	return true;
}

D3D12_DESCRIPTOR_RANGE1 DX12RenderingServer::GetDescriptorRange(RenderPassComponent* RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
{
	auto& l_descriptorAccessor = GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
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
			l_range.NumDescriptors = l_descriptorAccessor.m_Desc.m_MaxDescriptors;
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

bool DX12RenderingServer::SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "Command list is null in SetDescriptorHeaps for render pass ", renderPass->m_InstanceName);
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get(), m_SamplerDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12RenderingServer::SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (!commandList || !commandList->m_CommandList)
	{
		Log(Error, "Command list is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
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

bool DX12RenderingServer::PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

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

bool DX12RenderingServer::Open(CommandListComponent* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_pipelineStateObject = reinterpret_cast<DX12PipelineStateObject*>(pipelineStateObject);
	auto l_PSO = l_pipelineStateObject ? l_pipelineStateObject->m_PSO.Get() : nullptr;
	auto l_currentFrame = GetCurrentFrame();

	// Get the appropriate allocator based on the command list type
	ID3D12CommandAllocator* allocator = nullptr;
	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		allocator = m_directCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Compute:
		allocator = m_computeCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Copy:
		allocator = m_copyCommandAllocators[l_currentFrame].Get();
		break;
	default:
		Log(Error, "Invalid command list type for Open operation");
		return false;
	}

	l_commandList->Reset(allocator, l_PSO);
	return true;
}

bool DX12RenderingServer::Close(CommandListComponent* commandList, GPUEngineType GPUEngineType)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	l_commandList->Close();
	return true;
}