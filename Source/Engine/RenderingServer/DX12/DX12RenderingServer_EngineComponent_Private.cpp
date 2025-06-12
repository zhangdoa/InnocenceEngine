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

bool DX12RenderingServer::CreateSRV(TextureComponent* rhs, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(rhs->m_TextureDesc);
	auto l_desc = GetSRVDesc(rhs->m_TextureDesc, l_textureDesc, mipSlice);
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);

	uint32_t frameCount = rhs->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(rhs->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, rhs->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = rhs->GetHandleIndex(frame, mipSlice);
		rhs->m_ReadHandles[handleIndex] = l_descHeapAccessor.GetNewHandle();
		m_device->CreateShaderResourceView(resource,
			&l_desc,
			D3D12_CPU_DESCRIPTOR_HANDLE{ rhs->m_ReadHandles[handleIndex].m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", rhs->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(TextureComponent* rhs, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(rhs->m_TextureDesc);
	auto l_desc = GetUAVDesc(rhs->m_TextureDesc, l_textureDesc, mipSlice);

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage, false);

	uint32_t frameCount = rhs->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(rhs->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, rhs->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = rhs->GetHandleIndex(frame, mipSlice);

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		rhs->m_WriteHandles[handleIndex].m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		rhs->m_WriteHandles[handleIndex].m_GPUHandle = l_descHandle.m_GPUHandle;
		rhs->m_WriteHandles[handleIndex].m_Index = l_descHandle.m_Index;

		m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", rhs->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateSRV(GPUBufferComponent* rhs)
{
	bool l_isRaytracingAS = rhs->m_Usage == GPUBufferUsage::TLAS || rhs->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)rhs->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_DX12DeviceMemory->m_SRV.Handle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_DX12DeviceMemory->m_SRV.Handle.m_Index, " for ", rhs->m_InstanceName, " has been created.");			
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(GPUBufferComponent* rhs)
{
	bool l_isRaytracingAS = rhs->m_Usage == GPUBufferUsage::TLAS || rhs->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? (uint32_t)rhs->m_ElementSize : 0;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::Invalid, false);

	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		DX12UAV l_result = {};
		l_result.UAVDesc = l_desc;

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		l_result.Handle.m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		l_result.Handle.m_GPUHandle = l_descHandle.m_GPUHandle;
		l_result.Handle.m_Index = l_descHandle.m_Index;

		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), rhs->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), rhs->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });

		l_DX12DeviceMemory->m_UAV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", rhs->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12RenderingServer::CreateCBV(GPUBufferComponent* rhs)
{
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadOnly);

	for (auto i : rhs->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		DX12CBV l_result;

		l_result.CBVDesc.BufferLocation = l_DX12MappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
		l_result.CBVDesc.SizeInBytes = (uint32_t)rhs->m_ElementSize;
		l_result.Handle = l_descHeapAccessor.GetNewHandle();

		m_device->CreateConstantBufferView(&l_result.CBVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_result.Handle.m_CPUHandle });

		l_DX12MappedMemory->m_CBV = l_result;
		
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", rhs->m_InstanceName, " has been created.");		
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

#ifdef INNO_DEBUG
	SetObjectName(RenderPassComp, l_PSO->m_RootSignature, "RootSignature");
#endif // INNO_DEBUG

	Log(Verbose, RenderPassComp->m_InstanceName, " RootSignature has been created.");

	if (RenderPassComp->m_RenderPassDesc.m_IndirectDraw)
	{
		D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[4] = {};

		// 1. Constant argument.
		argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		argumentDescs[0].Constant.RootParameterIndex = 0; // The root signature slot for the constant.
		argumentDescs[0].Constant.DestOffsetIn32BitValues = 0; // Start at offset 0.
		argumentDescs[0].Constant.Num32BitValuesToSet = 1; // Number of 32-bit values.

		// 2. Vertex buffer view.
		argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;

		// 3. Index buffer view.
		argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

		// 4. Draw indexed arguments.
		argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
		commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
		commandSignatureDesc.pArgumentDescs = argumentDescs;
		commandSignatureDesc.ByteStride = sizeof(DX12DrawIndirectCommand);

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
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment)
			l_range.NumDescriptors = 1;
	}
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
		l_range.NumDescriptors = 1;

	return l_range;
}

bool DX12RenderingServer::SetDescriptorHeaps(RenderPassComponent* renderPass, DX12CommandList* commandList)
{
	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList = commandList->m_DirectCommandList;
	}
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandList = commandList->m_ComputeCommandList;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get(), m_SamplerDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12RenderingServer::SetRenderTargets(RenderPassComponent* renderPass, DX12CommandList* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	auto l_commandList = commandList->m_DirectCommandList;
	auto l_currentFrame = GetCurrentFrame();
	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);

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

bool DX12RenderingServer::PreparePipeline(RenderPassComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO)
{
	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList = commandList->m_DirectCommandList;
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList = commandList->m_ComputeCommandList;

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

bool DX12RenderingServer::Open(ICommandList* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_pipelineStateObject = reinterpret_cast<DX12PipelineStateObject*>(pipelineStateObject);
	auto l_PSO = l_pipelineStateObject ? l_pipelineStateObject->m_PSO.Get() : nullptr;
	auto l_currentFrame = GetCurrentFrame();
	if (GPUEngineType == GPUEngineType::Graphics)
	{
		auto l_DX12CommandList = l_commandList->m_DirectCommandList;
		l_DX12CommandList->Reset(m_directCommandAllocators[l_currentFrame].Get(), l_PSO);
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		auto l_DX12CommandList = l_commandList->m_ComputeCommandList;
		l_DX12CommandList->Reset(m_computeCommandAllocators[l_currentFrame].Get(), l_PSO);
	}
	else if (GPUEngineType == GPUEngineType::Copy)
	{
		auto l_DX12CommandList = l_commandList->m_CopyCommandList;
		l_DX12CommandList->Reset(m_copyCommandAllocators[l_currentFrame].Get(), l_PSO);
	}

	return true;
}

bool DX12RenderingServer::Close(ICommandList* commandList, GPUEngineType GPUEngineType)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	if (GPUEngineType == GPUEngineType::Graphics)
	{
		auto l_DX12CommandList = l_commandList->m_DirectCommandList;
		l_DX12CommandList->Close();
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		auto l_DX12CommandList = l_commandList->m_ComputeCommandList;
		l_DX12CommandList->Close();
	}
	else if (GPUEngineType == GPUEngineType::Copy)
	{
		auto l_DX12CommandList = l_commandList->m_CopyCommandList;
		l_DX12CommandList->Close();
	}

	return true;
}

bool DX12RenderingServer::GenerateMipmapImpl(TextureComponent* TextureComp, ICommandList* commandList)
{
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

	if (TextureComp->m_TextureDesc.MipLevels == 1)
	{
		Log(Warning, TextureComp->m_InstanceName, " Attempt to generate mipmaps for texture without mipmaps requirement.");
		return false;
	}

	// Determine if this is a static texture (Sample usage) or render target (attachment usage)
	bool isStaticTexture = (TextureComp->m_TextureDesc.Usage == TextureUsage::Sample);
	bool isRenderTarget = (TextureComp->m_TextureDesc.Usage == TextureUsage::ColorAttachment ||
		TextureComp->m_TextureDesc.Usage == TextureUsage::DepthAttachment ||
		TextureComp->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment);

	// For static textures: generate mipmaps for all device memories
	// For render targets: generate mipmaps only for current frame's device memory
	size_t startIndex = 0;
	size_t endIndex = 1;

	if (isStaticTexture && TextureComp->m_TextureDesc.IsMultiBuffer)
	{
		// Static textures with multi-buffer: generate for all buffers
		endIndex = TextureComp->m_ReadHandles.size();
	}
	else if (isRenderTarget && TextureComp->m_TextureDesc.IsMultiBuffer)
	{
		// Render targets with multi-buffer: generate only for current frame
		startIndex = GetCurrentFrame();
		endIndex = startIndex + 1;
	}
	else
	{
		// Single buffer textures: always use index 0
		startIndex = 0;
		endIndex = 1;
	}

	auto l_DX12CommandList = reinterpret_cast<DX12CommandList*>(commandList);
	if (!l_DX12CommandList)
	{
		Log(Error, TextureComp->m_InstanceName, " Invalid command list");
		return false;
	}

	auto l_computeCommandList = l_DX12CommandList->m_ComputeCommandList;
	auto l_directCommandList = l_DX12CommandList->m_DirectCommandList;
	if (!l_computeCommandList || !l_directCommandList)
	{
		Log(Error, TextureComp->m_InstanceName, " Invalid command lists");
		return false;
	}

	// Set pipeline state based on texture type
	if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	{
		l_computeCommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
		l_computeCommandList->SetPipelineState(m_3DMipmapPSO);
	}
	else
	{
		l_computeCommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
		l_computeCommandList->SetPipelineState(m_2DMipmapPSO);
	}

	// Set descriptor heaps
	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_computeCommandList->SetDescriptorHeaps(1, l_heaps);

	uint32_t l_mipLevels = TextureComp->m_TextureDesc.MipLevels;

	// Process each required device memory
	for (size_t deviceMemoryIndex = startIndex; deviceMemoryIndex < endIndex; deviceMemoryIndex++)
	{
		auto l_defaultHeapBuffer = reinterpret_cast<ID3D12Resource*>(TextureComp->m_GPUResources[deviceMemoryIndex]);
		if (!l_defaultHeapBuffer)
		{
			Log(Error, TextureComp->m_InstanceName, " Invalid device memory at index ", deviceMemoryIndex);
			return false;
		}

		if (TextureComp->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer, 
				static_cast<D3D12_RESOURCE_STATES>(TextureComp->m_CurrentState), 
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			l_directCommandList->ResourceBarrier(1, &l_transition);
		}

		for (uint32_t mipLevel = 0; mipLevel < l_mipLevels - 1; mipLevel++)
		{
			uint32_t dstWidth = std::max(TextureComp->m_TextureDesc.Width >> (mipLevel + 1), 1u);
			uint32_t dstHeight = std::max(TextureComp->m_TextureDesc.Height >> (mipLevel + 1), 1u);
			uint32_t dstDepth = 1;

			// Set texel size constants (1.0 / dstSize)
			l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
			l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

			if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
			{
				dstDepth = std::max(TextureComp->m_TextureDesc.DepthOrArraySize >> (mipLevel + 1), 1u);
				l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
			}

			// Bind source mip (SRV) and destination mip (UAV) using array indices
			// Source: mipLevel (read from current mip)
			// Destination: mipLevel + 1 (write to next smaller mip)
			auto l_readHandleIndex = TextureComp->GetHandleIndex(deviceMemoryIndex, mipLevel);
			auto l_srcSRV = D3D12_GPU_DESCRIPTOR_HANDLE { TextureComp->m_ReadHandles[l_readHandleIndex].m_GPUHandle };

			auto l_writeHandleIndex = TextureComp->GetHandleIndex(deviceMemoryIndex, mipLevel + 1);
			auto l_dstUAV = D3D12_GPU_DESCRIPTOR_HANDLE { TextureComp->m_WriteHandles[l_writeHandleIndex].m_GPUHandle };

			l_computeCommandList->SetComputeRootDescriptorTable(1, l_srcSRV);
			l_computeCommandList->SetComputeRootDescriptorTable(2, l_dstUAV);

			// Dispatch compute shader
			uint32_t dispatchX = std::max(dstWidth / 8, 1u);
			uint32_t dispatchY = std::max(dstHeight / 8, 1u);
			uint32_t dispatchZ = std::max(dstDepth / 8, 1u);

			l_computeCommandList->Dispatch(dispatchX, dispatchY, dispatchZ);

			D3D12_RESOURCE_BARRIER l_uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_defaultHeapBuffer);
			l_directCommandList->ResourceBarrier(1, &l_uavBarrier);
		}

		l_directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			l_defaultHeapBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			static_cast<D3D12_RESOURCE_STATES>(TextureComp->m_CurrentState)));
	}

	Log(Verbose, TextureComp->m_InstanceName, " Successfully generated ", l_mipLevels, " mip levels for ", (endIndex - startIndex), " device memory/memories with proper synchronization");
	return true;
}
