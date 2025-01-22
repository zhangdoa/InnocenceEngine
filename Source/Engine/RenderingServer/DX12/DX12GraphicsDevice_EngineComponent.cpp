#include "DX12GraphicsDevice.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"

#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsDevice::CreateRTV(DX12RenderPassComponent* rhs)
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
		auto l_descHandle = m_RTVDescHeap->GetNewHandle();
		rhs->m_RTVDescCPUHandles[i] = l_descHandle.CPUHandle;
		auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
		m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &rhs->m_RTVDesc, rhs->m_RTVDescCPUHandles[i]);
	}

	Log(Verbose, rhs->m_InstanceName, " RTV has been created.");
	return true;
}

bool DX12GraphicsDevice::CreateDSV(DX12RenderPassComponent* rhs)
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

	auto l_descHandle = m_DSVDescHeap->GetNewHandle();
	rhs->m_DSVDesc = GetDSVDesc(rhs->m_RenderPassDesc.m_RenderTargetDesc, rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
	rhs->m_DSVDescCPUHandle = l_descHandle.CPUHandle;

	auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
	m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &rhs->m_DSVDesc, rhs->m_DSVDescCPUHandle);

	Log(Verbose, rhs->m_InstanceName, " DSV has been created.");
	return false;
	return false;
}

DX12SRV DX12GraphicsDevice::CreateSRV(DX12TextureComponent* rhs, uint32_t mostDetailedMip)
{
	return CreateSRV(GetSRVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mostDetailedMip), rhs->m_DefaultHeapBuffer);
}

DX12SRV DX12GraphicsDevice::CreateSRV(DX12GPUBufferComponent* rhs)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_isAtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_isAtomicCounter ? (uint32_t)rhs->m_ElementSize : 0;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	return CreateSRV(l_desc, rhs->m_DefaultHeapBuffer);
}

DX12UAV DX12GraphicsDevice::CreateUAV(DX12TextureComponent* rhs, uint32_t mipSlice)
{
	auto l_desc = GetUAVDesc(rhs->m_TextureDesc, rhs->m_DX12TextureDesc, mipSlice);

	return CreateUAV(l_desc, rhs->m_DefaultHeapBuffer, false);
}

DX12UAV DX12GraphicsDevice::CreateUAV(DX12GPUBufferComponent* rhs)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = rhs->m_isAtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = rhs->m_isAtomicCounter ? (uint32_t)rhs->m_ElementSize : 0;

	return CreateUAV(l_desc, rhs->m_DefaultHeapBuffer, rhs->m_isAtomicCounter);
}

DX12CBV DX12GraphicsDevice::CreateCBV(DX12GPUBufferComponent* rhs)
{
	DX12CBV l_result;

	l_result.CBVDesc.BufferLocation = rhs->m_UploadHeapBuffer->GetGPUVirtualAddress();
	l_result.CBVDesc.SizeInBytes = (uint32_t)rhs->m_ElementSize;
	l_result.Handle = m_CSUDescHeap->GetNewHandle();

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.Handle.CPUHandle);

	return l_result;
}

bool DX12GraphicsDevice::CreateRootSignature(DX12RenderPassComponent* DX12RenderPassComp)
{
	if (DX12RenderPassComp->m_ResourceBindingLayoutDescs.empty())
	{
		Log(Verbose, "Skipping creating RootSignature for ", DX12RenderPassComp->m_InstanceName.c_str());
		return true;
	}

	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters;
	l_rootParameters.resize(DX12RenderPassComp->m_ResourceBindingLayoutDescs.size());
	std::vector<CD3DX12_DESCRIPTOR_RANGE1> l_tables;
	l_tables.resize(DX12RenderPassComp->m_ResourceBindingLayoutDescs.size());
	for (size_t i = 0; i < l_rootParameters.size(); i++)
	{
		auto& l_resourceBinderLayoutDesc = DX12RenderPassComp->m_ResourceBindingLayoutDescs[i];
		auto l_type = GetDescriptorRangeType(DX12RenderPassComp, l_resourceBinderLayoutDesc);
		if (l_type == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			if (l_resourceBinderLayoutDesc.m_IsRootConstant)
				l_rootParameters[i].InitAsConstants(l_resourceBinderLayoutDesc.m_SubresourceCount, l_resourceBinderLayoutDesc.m_DescriptorIndex);
			else
				l_rootParameters[i].InitAsConstantBufferView(l_resourceBinderLayoutDesc.m_DescriptorIndex);
		}
		else
		{
			l_tables[i].Init(l_type, l_resourceBinderLayoutDesc.m_SubresourceCount, l_resourceBinderLayoutDesc.m_DescriptorIndex);
			l_rootParameters[i].InitAsDescriptorTable(1, &l_tables[i]);
		}
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

	l_HResult = device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&DX12RenderPassComp->m_RootSignature));

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

bool DX12GraphicsDevice::CreatePSO(DX12RenderPassComponent* DX12RenderPassComp)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(DX12RenderPassComp->m_PipelineStateObject);
	if (DX12RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
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

		auto l_HResult = device->CreateGraphicsPipelineState(&l_PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));
		if (FAILED(l_HResult))
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Graphics PSO.");
			return false;
		}
	}
	else
	{
		CreateShaderPrograms(DX12RenderPassComp);

		l_PSO->m_ComputePSODesc.pRootSignature = DX12RenderPassComp->m_RootSignature.Get();
		auto l_HResult = device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create Compute PSO.");
			return false;
		}
	}

#ifdef INNO_DEBUG
	SetObjectName(DX12RenderPassComp, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG

	Log(Verbose, "", DX12RenderPassComp->m_InstanceName.c_str(), " PSO has been created.");

	return true;
}

bool DX12GraphicsDevice::CreateCommandList(DX12CommandList* commandList, size_t swapChainImageIndex, const std::wstring& name)
{
	commandList->m_DirectCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCommandAllocators[swapChainImageIndex].Get(), (name + L"_DirectCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	commandList->m_ComputeCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[swapChainImageIndex].Get(), (name + L"_ComputeCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	commandList->m_CopyCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_copyCommandAllocator, (name + L"_CopyCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());

	commandList->m_DirectCommandList->Close();
	commandList->m_ComputeCommandList->Close();
	commandList->m_CopyCommandList->Close();

	return true;
}

bool DX12GraphicsDevice::CreateFenceEvents(DX12RenderPassComponent* DX12RenderPassComp)
{
	bool result = true;
	for (size_t i = 0; i < DX12RenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(DX12RenderPassComp->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for compute CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, DX12RenderPassComp->m_InstanceName.c_str(), " Fence events have been created.");
	}

	return result;
}

DX12DescriptorHeap* DX12GraphicsDevice::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, bool isShaderVisible)
{
	switch (type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return isShaderVisible ? m_CSUDescHeap : m_ShaderNonVisibleCSUDescHeap;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		return m_SamplerDescHeap;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return m_RTVDescHeap;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return m_DSVDescHeap;
	default:
		return nullptr;
	}
}

bool DX12GraphicsDevice::CreateFenceEvents(DX12RenderPassComponent* DX12RenderPassComp)
{
	bool result = true;
	for (size_t i = 0; i < DX12RenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(DX12RenderPassComp->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, "", DX12RenderPassComp->m_InstanceName.c_str(), " Can't create fence event for compute CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, DX12RenderPassComp->m_InstanceName.c_str(), " Fence events have been created.");
	}

	return result;
}

bool DX12GraphicsDevice::GenerateMipmap(DX12TextureComponent* DX12TextureComp)
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

	if (!DX12TextureComp->m_TextureDesc.UseMipMap)
	{
		Log(Warning, "Attempt to generate mipmaps for texture without mipmaps requirement.");

		return false;
	}

	if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	{
		auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
		directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
		directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), DX12TextureComp->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		ExecuteCommandListAndWait(directCommandList, m_directCommandQueue);
	}

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap->GetHeap().Get() };

	auto computeCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_ComputeCommandList;
	computeCommandList->Reset(m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	{
		computeCommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
		computeCommandList->SetPipelineState(m_3DMipmapPSO);
	}
	else
	{
		computeCommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
		computeCommandList->SetPipelineState(m_2DMipmapPSO);
	}
	computeCommandList->SetDescriptorHeaps(1, l_heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE l_SRV = DX12TextureComp->m_SRV.Handle.GPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE l_UAV;
	l_UAV.ptr = DX12TextureComp->m_UAV.Handle.GPUHandle.ptr + l_CSUDescSize;

	for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
	{
		uint32_t dstWidth = std::max(DX12TextureComp->m_TextureDesc.Width >> (TopMip + 1), (uint32_t)1);
		uint32_t dstHeight = std::max(DX12TextureComp->m_TextureDesc.Height >> (TopMip + 1), (uint32_t)1);
		uint32_t dstDepth = 1;

		computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
		computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

		if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			dstDepth = std::max(DX12TextureComp->m_TextureDesc.DepthOrArraySize >> (TopMip + 1), (uint32_t)1);
			computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
		}

		computeCommandList->SetComputeRootDescriptorTable(1, l_SRV);
		computeCommandList->SetComputeRootDescriptorTable(2, l_UAV);

		computeCommandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), std::max(dstDepth / 8, 1u));

		l_SRV.ptr += l_CSUDescSize;
		l_UAV.ptr += l_CSUDescSize;
	}

	ExecuteCommandListAndWait(computeCommandList, m_computeCommandQueue);

	if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	{
		auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
		directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
		directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12TextureComp->m_CurrentState));
		ExecuteCommandListAndWait(directCommandList, m_directCommandQueue);
	}

	return true;
}

bool DX12GraphicsDevice::PrepareRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
		{
			auto l_rhs = reinterpret_cast<DX12TextureComponent*>(renderPass->m_RenderTargets[renderPass->m_CurrentFrame].m_Texture);

			TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
		}
		else
		{
			for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				auto l_rhs = reinterpret_cast<DX12TextureComponent*>(renderPass->m_RenderTargets[i].m_Texture);

				TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
			}
		}

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_rhs = reinterpret_cast<DX12TextureComponent*>(renderPass->m_DepthStencilRenderTarget.m_Texture);

			TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
		}
	}

	return true;
}

bool DX12GraphicsDevice::SetDescriptorHeaps(DX12RenderPassComponent* renderPass, DX12CommandList* commandList)
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

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap->GetHeap().Get(), m_SamplerDescHeap->GetHeap().Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12GraphicsDevice::SetRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList)
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

bool DX12GraphicsDevice::PreparePipeline(DX12RenderPassComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO)
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

bool DX12GraphicsDevice::ExecuteCommandList(DX12CommandList* commandList, DX12Semaphore* semaphore, GPUEngineType GPUEngineType)
{
	auto l_currentFrame = m_SwapChainRenderPassComp->m_CurrentFrame;
	if (GPUEngineType == GPUEngineType::Graphics)
	{
		UINT64 l_directCommandFinishedSemaphore = ++m_directCommandQueueSemaphore[l_currentFrame];
		m_directCommandQueueFence[l_currentFrame]->SetEventOnCompletion(l_directCommandFinishedSemaphore, semaphore->m_DirectCommandQueueFenceEvent);
		semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		ID3D12CommandList* l_directCommandLists[] = { commandList->m_DirectCommandList.Get() };
		m_directCommandQueue->ExecuteCommandLists(1, l_directCommandLists);

		m_directCommandQueueFenceEvent[l_currentFrame] = semaphore->m_DirectCommandQueueFenceEvent;
		m_directCommandQueue->Signal(m_directCommandQueueFence[l_currentFrame].Get(), l_directCommandFinishedSemaphore);
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		UINT64 l_computeCommandFinishedSemaphore = ++m_computeCommandQueueSemaphore[l_currentFrame];
		m_computeCommandQueueFence[l_currentFrame]->SetEventOnCompletion(l_computeCommandFinishedSemaphore, semaphore->m_ComputeCommandQueueFenceEvent);
		semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		ID3D12CommandList* l_computeCommandLists[] = { commandList->m_ComputeCommandList.Get() };
		m_computeCommandQueue->ExecuteCommandLists(1, l_computeCommandLists);

		m_computeCommandQueueFenceEvent[l_currentFrame] = semaphore->m_ComputeCommandQueueFenceEvent;
		m_computeCommandQueue->Signal(m_computeCommandQueueFence[l_currentFrame].Get(), l_computeCommandFinishedSemaphore);
	}

	return true;
}

bool DX12GraphicsDevice::WaitCommandQueue(DX12Semaphore* rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t semaphore = 0;
	auto l_currentFrame = m_SwapChainRenderPassComp->m_CurrentFrame;

	if (queueType == GPUEngineType::Graphics)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	}
	else if (queueType == GPUEngineType::Compute)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	}

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_directCommandQueueFence[l_currentFrame].Get();
		semaphore = rhs->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_computeCommandQueueFence[l_currentFrame].Get();
		semaphore = rhs->m_ComputeCommandQueueSemaphore;
	}

	if (commandQueue && fence)
	{
		commandQueue->Wait(fence, semaphore);
	}

	return true;
}

bool DX12GraphicsDevice::WaitFence(DX12Semaphore* rhs, GPUEngineType GPUEngineType)
{
	UINT64 semaphore = 0;
	HANDLE fenceEvent = 0;
	auto l_currentFrame = m_SwapChainRenderPassComp->m_CurrentFrame;

	if (rhs == nullptr)
	{
		if (GPUEngineType == GPUEngineType::Graphics)
		{
			semaphore = m_directCommandQueueSemaphore[l_currentFrame];
			fenceEvent = m_directCommandQueueFenceEvent[l_currentFrame];
		}
		else if (GPUEngineType == GPUEngineType::Compute)
		{
			semaphore = m_computeCommandQueueSemaphore[l_currentFrame];
			fenceEvent = m_computeCommandQueueFenceEvent[l_currentFrame];
		}
	}
	else
	{
		if (GPUEngineType == GPUEngineType::Graphics)
		{
			semaphore = rhs->m_DirectCommandQueueSemaphore;
			fenceEvent = rhs->m_DirectCommandQueueFenceEvent;
		}
		else if (GPUEngineType == GPUEngineType::Compute)
		{
			semaphore = rhs->m_ComputeCommandQueueSemaphore;
			fenceEvent = rhs->m_ComputeCommandQueueFenceEvent;
		}
	}

	if (GPUEngineType == GPUEngineType::Graphics)
	{
		if (m_directCommandQueueFence[l_currentFrame]->GetCompletedValue() < semaphore)
		{
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		ResetEvent(fenceEvent);
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		if (m_computeCommandQueueFence[l_currentFrame]->GetCompletedValue() < semaphore)
		{
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		ResetEvent(fenceEvent);
	}

	return true;
}

bool DX12GraphicsDevice::PostResize(DX12RenderPassComponent* rhs, const TVec2<uint32_t>& screenResolution)
{
	if (!rhs->m_RenderPassDesc.m_Resizable)
		return true;

	rhs->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	rhs->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	m_RenderingComponentPool->ReserveRenderTargets(rhs);
	m_RenderingServer->CreateRenderTargets(rhs);

	if (rhs->m_RenderPassDesc.m_UseOutputMerger)
	{
		for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &rhs->m_RTVDesc, rhs->m_RTVDescCPUHandles[i]);
		}
	}

	if (rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		if (rhs->m_DepthStencilRenderTarget.m_Texture != nullptr)
		{
			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &rhs->m_DSVDesc, rhs->m_DSVDescCPUHandle);
		}
	}

	rhs->m_PipelineStateObject = m_RenderingComponentPool->AddPipelineStateObject();

	CreatePSO(rhs, m_device);

	if (rhs->m_OnResize)
		rhs->m_OnResize();

	return true;
}
