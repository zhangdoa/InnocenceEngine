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
, uint32_t maxDescriptors, uint32_t descriptorSize, const DX12DescriptorHandle& firstHandle, bool shaderVisible, const wchar_t* name)
{
	DX12DescriptorHeapAccessor l_descHeapAccessor = {};
	l_descHeapAccessor.m_Desc.m_HeapDesc = desc;
	l_descHeapAccessor.m_Desc.m_MaxDescriptors = maxDescriptors;
	l_descHeapAccessor.m_Desc.m_DescriptorSize = descriptorSize;
	l_descHeapAccessor.m_Desc.m_ShaderVisible = shaderVisible;
	l_descHeapAccessor.m_Desc.m_Name = name;
	l_descHeapAccessor.m_OffsetFromHeapStart = firstHandle.CPUHandle.ptr - descHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	l_descHeapAccessor.m_OffsetFromHeapStart /= descriptorSize;
	l_descHeapAccessor.m_FirstHandle = firstHandle;
	l_descHeapAccessor.m_CurrentHandle = firstHandle;
	
	l_descHeapAccessor.m_Heap = descHeap;

	Log(Verbose, "Descriptor heap accessor ", name, " has been created.");

	return l_descHeapAccessor;
}

bool DX12RenderingServer::CreateSRV(DX12TextureComponent* rhs, uint32_t mostDetailedMip)
{
	auto l_desc = GetSRVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mostDetailedMip);
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);

	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, l_DX12DeviceMemory->m_SRV.Handle.CPUHandle);
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(DX12TextureComponent* rhs, uint32_t mipSlice)
{
	auto l_desc = GetUAVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mipSlice);

	DX12UAV l_result = {};
	l_result.UAVDesc = l_desc;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage, false);

	auto l_descHandle = l_descHeapAccessor.GetNewHandle();
	auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);		
		DX12UAV l_result = {};
		l_result.UAVDesc = l_desc;

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		l_result.Handle.CPUHandle = l_descHandle_ShaderNonVisible.CPUHandle;
		l_result.Handle.GPUHandle = l_descHandle.GPUHandle;
		l_result.Handle.m_Index = l_descHandle.m_Index;

		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), 0, &l_result.UAVDesc, l_descHandle_ShaderNonVisible.CPUHandle);
		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), 0, &l_result.UAVDesc, l_descHandle.CPUHandle);

		l_DX12DeviceMemory->m_UAV = l_result;		
	}

	return true;
}

bool DX12RenderingServer::CreateSRV(GPUBufferComponent* rhs)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)rhs->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	for (auto i : rhs->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, l_DX12DeviceMemory->m_SRV.Handle.CPUHandle);
	}

	return true;
}

bool DX12RenderingServer::CreateUAV(GPUBufferComponent* rhs)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
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

		l_result.Handle.CPUHandle = l_descHandle_ShaderNonVisible.CPUHandle;
		l_result.Handle.GPUHandle = l_descHandle.GPUHandle;
		l_result.Handle.m_Index = l_descHandle.m_Index;

		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), rhs->m_Usage == GPUBufferUsage::AtomicCounter ? l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, l_descHandle_ShaderNonVisible.CPUHandle);
		m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), rhs->m_Usage == GPUBufferUsage::AtomicCounter ? l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, l_descHandle.CPUHandle);

		l_DX12DeviceMemory->m_UAV = l_result;
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

		m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.Handle.CPUHandle);

		l_DX12MappedMemory->m_CBV = l_result;
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
			switch (l_descriptorRange.RangeType)
			{
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				rangeTypeName = "CBV";
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				rangeTypeName = "SRV";
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				rangeTypeName = "UAV";
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
				rangeTypeName = "Sampler";
				break;
			}

			auto& l_lastRange = l_descriptorRanges.emplace_back(l_descriptorRange);
			Log(Verbose, RenderPassComp->m_InstanceName, " Root Descriptor Table: ", rangeTypeName, " at root parameter ", i,
				" BaseShaderRegister ", l_descriptorRange.BaseShaderRegister, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
			l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
		}

		l_rootParameters.emplace_back(l_rootParameter);
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc((uint32_t)l_rootParameters.size(), l_rootParameters.data());
	l_rootSigDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
	ComPtr<ID3D12GraphicsCommandList> l_commandList;
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
	ComPtr<ID3D12GraphicsCommandList> l_commandList;
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList = commandList->m_DirectCommandList;
	}
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandList = commandList->m_ComputeCommandList;
	}

	if (PSO->m_PSO)
		l_commandList->SetPipelineState(PSO->m_PSO.Get());

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

bool DX12RenderingServer::GenerateMipmapImpl(DX12TextureComponent* DX12TextureComp)
{
	// struct DWParam
	// {
	// 	DWParam(FLOAT f) : Float(f) {}
	// 	DWParam(UINT u) : Uint(u) {}

	// 	void operator=(FLOAT f) { Float = f; }
	// 	void operator=(UINT u) { Uint = u; }

	// 	union
	// 	{
	// 		FLOAT Float;
	// 		UINT Uint;
	// 	};
	// };

	// if (!DX12TextureComp->m_TextureDesc.UseMipMap)
	// {
	// 	Log(Warning, "Attempt to generate mipmaps for texture without mipmaps requirement.");

	// 	return false;
	// }

	// if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	// {
	// 	auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	// 	directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
	// 	directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), DX12TextureComp->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	// 	ExecuteCommandListAndWait(directCommandList, m_directCommandQueue);
	// }

	// auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };

	// auto computeCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_ComputeCommandList;
	// computeCommandList->Reset(m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	// if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	// {
	// 	computeCommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
	// 	computeCommandList->SetPipelineState(m_3DMipmapPSO);
	// }
	// else
	// {
	// 	computeCommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
	// 	computeCommandList->SetPipelineState(m_2DMipmapPSO);
	// }
	// computeCommandList->SetDescriptorHeaps(1, l_heaps);

	// D3D12_GPU_DESCRIPTOR_HANDLE l_SRV = DX12TextureComp->m_SRV.Handle.GPUHandle;
	// D3D12_GPU_DESCRIPTOR_HANDLE l_UAV;
	// l_UAV.ptr = DX12TextureComp->m_UAV.Handle.GPUHandle.ptr + l_CSUDescSize;

	// for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
	// {
	// 	uint32_t dstWidth = std::max(DX12TextureComp->m_TextureDesc.Width >> (TopMip + 1), (uint32_t)1);
	// 	uint32_t dstHeight = std::max(DX12TextureComp->m_TextureDesc.Height >> (TopMip + 1), (uint32_t)1);
	// 	uint32_t dstDepth = 1;

	// 	computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
	// 	computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

	// 	if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	// 	{
	// 		dstDepth = std::max(DX12TextureComp->m_TextureDesc.DepthOrArraySize >> (TopMip + 1), (uint32_t)1);
	// 		computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
	// 	}

	// 	computeCommandList->SetComputeRootDescriptorTable(1, l_SRV);
	// 	computeCommandList->SetComputeRootDescriptorTable(2, l_UAV);

	// 	computeCommandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), std::max(dstDepth / 8, 1u));

	// 	l_SRV.ptr += l_CSUDescSize;
	// 	l_UAV.ptr += l_CSUDescSize;
	// }

	// ExecuteCommandListAndWait(computeCommandList, m_computeCommandQueue);

	// if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	// {
	// 	auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	// 	directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
	// 	directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12TextureComp->m_CurrentState));
	// 	ExecuteCommandListAndWait(directCommandList, m_directCommandQueue);
	// }

	return true;
}
