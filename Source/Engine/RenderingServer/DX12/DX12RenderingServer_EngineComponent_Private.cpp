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

bool DX12RenderingServer::CreateRTV(DX12RenderPassComponent* rhs)
{
	if (!rhs->m_RenderPassDesc.m_UseOutputMerger)
	{
		Log(Verbose, rhs->m_InstanceName, " doesn't use output merger, so no RTV is created.");
		return false;
	}

	rhs->m_RTVDesc = GetRTVDesc(rhs->m_RenderPassDesc.m_RenderTargetDesc);
	rhs->m_RTVDescCPUHandles.resize(rhs->m_RenderPassDesc.m_RenderTargetCount);

	for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		auto l_descHandle = m_RTVDescHeapAccessor.GetNewHandle();
		rhs->m_RTVDescCPUHandles[i] = l_descHandle.CPUHandle;
		auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
		m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &rhs->m_RTVDesc, rhs->m_RTVDescCPUHandles[i]);
	}

	Log(Verbose, rhs->m_InstanceName, " RTV has been created.");
	return true;
}

bool DX12RenderingServer::CreateDSV(DX12RenderPassComponent* rhs)
{
	if (!rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		Log(Verbose, rhs->m_InstanceName, " doesn't use depth test, so no DSV is created.");
		return false;
	}

	if (rhs->m_DepthStencilRenderTarget.m_Texture == nullptr)
	{
		Log(Error, rhs->m_InstanceName, " depth (and stencil) test is enable, but no depth-stencil render target is bound!");
		return false;
	}

	auto l_descHandle = m_DSVDescHeapAccessor.GetNewHandle();
	rhs->m_DSVDesc = GetDSVDesc(rhs->m_RenderPassDesc.m_RenderTargetDesc, rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
	rhs->m_DSVDescCPUHandle = l_descHandle.CPUHandle;

	auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
	m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &rhs->m_DSVDesc, rhs->m_DSVDescCPUHandle);

	Log(Verbose, rhs->m_InstanceName, " DSV has been created.");
	return false;
	return false;
}

DX12SRV DX12RenderingServer::CreateSRV(DX12TextureComponent* rhs, uint32_t mostDetailedMip)
{
	auto l_desc = GetSRVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mostDetailedMip);
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);

	DX12SRV l_result = {};
	l_result.SRVDesc = l_desc;
	l_result.Handle = l_descHeapAccessor.GetNewHandle();

	m_device->CreateShaderResourceView(rhs->m_DefaultHeapBuffer.Get(), &l_result.SRVDesc, l_result.Handle.CPUHandle);

	return l_result;
}

DX12SRV DX12RenderingServer::CreateSRV(DX12GPUBufferComponent* rhs)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_isAtomicCounter ? DXGI_FORMAT_R32_UINT :  DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_isAtomicCounter ? 0 : (uint32_t)rhs->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	DX12SRV l_result = {};
	l_result.SRVDesc = l_desc;
	l_result.Handle = l_descHeapAccessor.GetNewHandle();

	m_device->CreateShaderResourceView(rhs->m_DefaultHeapBuffer.Get(), &l_result.SRVDesc, l_result.Handle.CPUHandle);

	return l_result;
}

DX12UAV DX12RenderingServer::CreateUAV(DX12TextureComponent* rhs, uint32_t mipSlice)
{
	auto l_desc = GetUAVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mipSlice);

	DX12UAV l_result = {};
	l_result.UAVDesc = l_desc;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, rhs->m_GPUAccessibility, rhs->m_TextureDesc.Usage, false);

	auto l_descHandle = l_descHeapAccessor.GetNewHandle();
	auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

	l_result.Handle.CPUHandle = l_descHandle_ShaderNonVisible.CPUHandle;
	l_result.Handle.GPUHandle = l_descHandle.GPUHandle;
	l_result.Handle.m_Index = l_descHandle.m_Index; // @TODO: Not sure which one is correct

	m_device->CreateUnorderedAccessView(rhs->m_DefaultHeapBuffer.Get(), 0, &l_result.UAVDesc, l_descHandle_ShaderNonVisible.CPUHandle);
	m_device->CreateUnorderedAccessView(rhs->m_DefaultHeapBuffer.Get(), 0, &l_result.UAVDesc, l_descHandle.CPUHandle);

	return l_result;
}

DX12UAV DX12RenderingServer::CreateUAV(DX12GPUBufferComponent* rhs)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_isAtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_isAtomicCounter ? (uint32_t)rhs->m_ElementSize : 0;

	DX12UAV l_result = {};
	l_result.UAVDesc = l_desc;

	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite);
	auto& l_descHeapAccessor_ShaderNonVisible = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::Invalid, false);

	auto l_descHandle = l_descHeapAccessor.GetNewHandle();
	auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

	l_result.Handle.CPUHandle = l_descHandle_ShaderNonVisible.CPUHandle;
	l_result.Handle.GPUHandle = l_descHandle.GPUHandle;
	l_result.Handle.m_Index = l_descHandle.m_Index; // @TODO: Not sure which one is correct

	m_device->CreateUnorderedAccessView(rhs->m_DefaultHeapBuffer.Get(), rhs->m_isAtomicCounter ? rhs->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, l_descHandle_ShaderNonVisible.CPUHandle);
	m_device->CreateUnorderedAccessView(rhs->m_DefaultHeapBuffer.Get(), rhs->m_isAtomicCounter ? rhs->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, l_descHandle.CPUHandle);

	return l_result;
}

DX12CBV DX12RenderingServer::CreateCBV(DX12GPUBufferComponent* rhs)
{
	auto& l_descHeapAccessor = GetDescriptorHeapAccessor(rhs->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadOnly);
	DX12CBV l_result;

	l_result.CBVDesc.BufferLocation = rhs->m_UploadHeapBuffer->GetGPUVirtualAddress();
	l_result.CBVDesc.SizeInBytes = (uint32_t)rhs->m_ElementSize;
	l_result.Handle = l_descHeapAccessor.GetNewHandle();

	m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.Handle.CPUHandle);

	return l_result;
}

bool DX12RenderingServer::CreateRootSignature(DX12RenderPassComponent* DX12RenderPassComp)
{
	if (DX12RenderPassComp->m_ResourceBindingLayoutDescs.empty())
	{
		Log(Verbose, "Skipping creating RootSignature for ", DX12RenderPassComp->m_InstanceName.c_str());
		return true;
	}

	auto l_maxBindingCount = DX12RenderPassComp->m_ResourceBindingLayoutDescs.size();
	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters;
	l_rootParameters.reserve(l_maxBindingCount);

	std::vector<D3D12_DESCRIPTOR_RANGE1> l_descriptorRanges;
	l_descriptorRanges.reserve(l_maxBindingCount);

	for (size_t i = 0; i < l_maxBindingCount; i++)
	{
		auto& l_resourceBinderLayoutDesc = DX12RenderPassComp->m_ResourceBindingLayoutDescs[i];
		auto l_descriptorRange = GetDescriptorRange(DX12RenderPassComp, l_resourceBinderLayoutDesc);
		CD3DX12_ROOT_PARAMETER1 l_rootParameter = {};
		if (l_descriptorRange.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			if (l_resourceBinderLayoutDesc.m_IsRootConstant)
			{
				Log(Verbose, DX12RenderPassComp->m_InstanceName, " Root Constant: at root parameter ", i,
				 " with ", l_resourceBinderLayoutDesc.m_SubresourceCount, " constants.");
				l_rootParameter.InitAsConstants(l_resourceBinderLayoutDesc.m_SubresourceCount, l_resourceBinderLayoutDesc.m_DescriptorIndex);
			}
			else
			{
				Log(Verbose, DX12RenderPassComp->m_InstanceName, " Root CBV: at root parameter ", i, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
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
			Log(Verbose, DX12RenderPassComp->m_InstanceName, " Root Descriptor Table: ", rangeTypeName, " at root parameter ", i,
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

			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " RootSignature serialization error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't serialize RootSignature.");
		}
		return false;
	}

	l_HResult = m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&DX12RenderPassComp->m_RootSignature));

	if (FAILED(l_HResult))
	{
		Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create RootSignature.");
		return false;
	}

#ifdef INNO_DEBUG
	SetObjectName(DX12RenderPassComp, DX12RenderPassComp->m_RootSignature, "RootSignature");
#endif // INNO_DEBUG

	Log(Verbose, "", DX12RenderPassComp->m_InstanceName.c_str(), " RootSignature has been created.");

	return true;
}

D3D12_DESCRIPTOR_RANGE1 DX12RenderingServer::GetDescriptorRange(DX12RenderPassComponent* DX12RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
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

bool DX12RenderingServer::CreatePipelineStateObject(RenderPassComponent* rhs)
{	
	bool l_result = true;
	auto DX12RenderPassComp = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	l_result &= CreateRootSignature(DX12RenderPassComp);
	
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(DX12RenderPassComp->m_PipelineStateObject);
	if (DX12RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{		
		l_result &= CreateRTV(DX12RenderPassComp);
		l_result &= CreateDSV(DX12RenderPassComp);

		GenerateDepthStencilStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
		GenerateBlendStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, l_PSO);
		GenerateRasterizerStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
		GenerateViewportStateDesc(DX12RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);

		if (DX12RenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
		{
			l_PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)DX12RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;
			for (size_t i = 0; i < DX12RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				l_PSO->m_GraphicsPSODesc.RTVFormats[i] = DX12RenderPassComp->m_RTVDesc.Format;
			}
		}

		l_PSO->m_GraphicsPSODesc.DSVFormat = DX12RenderPassComp->m_DSVDesc.Format;
		l_PSO->m_GraphicsPSODesc.DepthStencilState = l_PSO->m_DepthStencilDesc;
		l_PSO->m_GraphicsPSODesc.RasterizerState = l_PSO->m_RasterizerDesc;
		l_PSO->m_GraphicsPSODesc.BlendState = l_PSO->m_BlendDesc;
		l_PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
		l_PSO->m_GraphicsPSODesc.PrimitiveTopologyType = l_PSO->m_PrimitiveTopologyType;
		l_PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;

		if (!DX12RenderPassComp->m_RootSignature.Get())
		{
			Log(Verbose, "Skipping creating DX12 PSO for ", DX12RenderPassComp->m_InstanceName.c_str());
			return true;
		}

		l_PSO->m_GraphicsPSODesc.pRootSignature = DX12RenderPassComp->m_RootSignature.Get();

		CreateInputLayout(l_PSO);
		CreateShaderPrograms(DX12RenderPassComp);

		auto l_HResult = m_device->CreateGraphicsPipelineState(&l_PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));
		if (FAILED(l_HResult))
		{
			Log(Error, DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Graphics PSO.");
			return false;
		}
	}
	else
	{
		CreateShaderPrograms(DX12RenderPassComp);

		l_PSO->m_ComputePSODesc.pRootSignature = DX12RenderPassComp->m_RootSignature.Get();
		auto l_HResult = m_device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Compute PSO.");
			return false;
		}
	}

#ifdef INNO_DEBUG
	SetObjectName(DX12RenderPassComp, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG

	Log(Verbose, "", DX12RenderPassComp->m_InstanceName.c_str(), " PSO has been created.");

	return true;
}

bool DX12RenderingServer::CreateCommandList(ICommandList* commandList, size_t swapChainImageIndex, const std::wstring& name)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	l_commandList->m_DirectCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCommandAllocators[swapChainImageIndex].Get(), (name + L"_DirectCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	l_commandList->m_ComputeCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[swapChainImageIndex].Get(), (name + L"_ComputeCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	l_commandList->m_CopyCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_copyCommandAllocators[swapChainImageIndex].Get(), (name + L"_CopyCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());

	l_commandList->m_DirectCommandList->Close();
	l_commandList->m_ComputeCommandList->Close();
	l_commandList->m_CopyCommandList->Close();

	return true;
}

bool DX12RenderingServer::CreateFenceEvents(RenderPassComponent* rhs)
{
	bool result = true;
	auto DX12RenderPassComp = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	for (size_t i = 0; i < DX12RenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(DX12RenderPassComp->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for compute CommandQueue.");
			result = false;
		}

		l_semaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_CopyCommandQueueFenceEvent == NULL)
		{
			Log(Error, DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for copy CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, DX12RenderPassComp->m_InstanceName.c_str(), " Fence events have been created.");
	}

	return result;
}

bool DX12RenderingServer::SetDescriptorHeaps(DX12RenderPassComponent* renderPass, DX12CommandList* commandList)
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

bool DX12RenderingServer::SetRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	auto l_commandList = commandList->m_DirectCommandList;

	D3D12_CPU_DESCRIPTOR_HANDLE* l_DSV = NULL;

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		l_DSV = &renderPass->m_DSVDescCPUHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE* l_RTVs = NULL;
	uint32_t l_RTCount = 0;

	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		if (renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
			{
				l_RTVs = &renderPass->m_RTVDescCPUHandles[renderPass->m_CurrentFrame];
				l_RTCount = 1;
			}
			else
			{
				l_RTVs = &renderPass->m_RTVDescCPUHandles[0];
				l_RTCount = (uint32_t)renderPass->m_RenderPassDesc.m_RenderTargetCount;
			}
		}
	}

	l_commandList->OMSetRenderTargets(l_RTCount, l_RTVs, FALSE, l_DSV);
	return true;
}

bool DX12RenderingServer::PreparePipeline(DX12RenderPassComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO)
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
		if (renderPass->m_RootSignature)
			l_commandList->SetGraphicsRootSignature(renderPass->m_RootSignature.Get());

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
		if (renderPass->m_RootSignature)
			l_commandList->SetComputeRootSignature(renderPass->m_RootSignature.Get());
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

bool DX12RenderingServer::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	auto l_currentFrame = GetCurrentFrame();
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(semaphore);
	auto l_isOnGlobalSemaphore = l_semaphore == l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
	{
		l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1);
		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.load();

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_directCommandQueue->Signal(m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1);
		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.load();

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;
		
		m_computeCommandQueue->Signal(m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		l_globalSemaphore->m_CopyCommandQueueSemaphore.fetch_add(1);
		UINT64 l_copyCommandFinishedSemaphore = l_globalSemaphore->m_CopyCommandQueueSemaphore.load();

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_CopyCommandQueueSemaphore = l_copyCommandFinishedSemaphore;

		m_copyCommandQueue->Signal(m_copyCommandQueueFence.Get(), l_copyCommandFinishedSemaphore);
	}

	return true;
}

bool DX12RenderingServer::WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t semaphoreValue = 0;
	auto l_currentFrame = GetCurrentFrame();
	auto l_semaphore = semaphore ? reinterpret_cast<DX12Semaphore*>(semaphore) : reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

	if (queueType == GPUEngineType::Graphics)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	}
	else if (queueType == GPUEngineType::Compute)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	}
	else if (queueType == GPUEngineType::Copy)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY).Get();
	}

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_directCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_computeCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_ComputeCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Copy)
	{
		fence = m_copyCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_CopyCommandQueueSemaphore;
	}

	if (commandQueue && fence)
	{
		commandQueue->Wait(fence, semaphoreValue);
	}

	return true;
}

bool DX12RenderingServer::Execute(ICommandList* commandList, GPUEngineType queueType)
{
	auto l_currentFrame = GetCurrentFrame();
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);

	if (queueType == GPUEngineType::Graphics)
	{
		ID3D12CommandList* l_directCommandLists[] = { l_commandList->m_DirectCommandList.Get() };
		m_directCommandQueue->ExecuteCommandLists(1, l_directCommandLists);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		ID3D12CommandList* l_computeCommandLists[] = { l_commandList->m_ComputeCommandList.Get() };
		m_computeCommandQueue->ExecuteCommandLists(1, l_computeCommandLists);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		ID3D12CommandList* l_copyCommandLists[] = { l_commandList->m_CopyCommandList.Get() };
		m_copyCommandQueue->ExecuteCommandLists(1, l_copyCommandLists);
	}

	return true;
}

uint64_t DX12RenderingServer::GetSemaphoreValue(GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

	if (queueType == GPUEngineType::Graphics)
		return l_semaphore->m_DirectCommandQueueSemaphore;	
	else if (queueType == GPUEngineType::Compute)
		return l_semaphore->m_ComputeCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Copy)
		return l_semaphore->m_CopyCommandQueueSemaphore;

	return 0;
}

bool DX12RenderingServer::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
	UINT64 l_semaphoreValue = 0;
	HANDLE* fenceEvent = nullptr;

	if (queueType == GPUEngineType::Graphics)
	{
		fenceEvent = &l_semaphore->m_DirectCommandQueueFenceEvent;
	}
	else if (queueType == GPUEngineType::Compute)
	{
		fenceEvent = &l_semaphore->m_ComputeCommandQueueFenceEvent;
	}
	else if (queueType == GPUEngineType::Copy)
	{
		fenceEvent = &l_semaphore->m_CopyCommandQueueFenceEvent;
	}

	if (queueType == GPUEngineType::Graphics)
	{
		if (m_directCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_directCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (m_computeCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_computeCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (m_copyCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_copyCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
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
