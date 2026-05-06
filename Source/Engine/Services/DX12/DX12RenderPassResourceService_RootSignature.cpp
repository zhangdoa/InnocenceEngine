#include "DX12RenderPassResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_BindlessMesh.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderPassResourceService::CreateRootSignature(RenderPassComponent* RenderPassComp)
{
	if (RenderPassComp->m_ResourceBindingLayoutDescs.empty())
		Log(Verbose, "Creating empty RootSignature for ", RenderPassComp->m_InstanceName);

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
	l_HResult = m_ctx->m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&l_PSO->m_RootSignature));

	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_ctx->m_device.Get(), "RootSignature", RenderPassComp->m_InstanceName.c_str(), l_HResult);
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(RenderPassComp, l_PSO->m_RootSignature, "RootSignature");
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

		l_HResult = m_ctx->m_device->CreateCommandSignature(&commandSignatureDesc, l_PSO->m_RootSignature.Get(), IID_PPV_ARGS(&l_PSO->m_IndirectCommandSignature));
		if (FAILED(l_HResult))
		{
			LogD3D12CreateFailure(m_ctx->m_device.Get(), "CommandSignature", RenderPassComp->m_InstanceName.c_str(), l_HResult);
			return false;
		}

		Log(Verbose, RenderPassComp->m_InstanceName, " CommandSignature has been created.");
	}

	return true;
}

D3D12_DESCRIPTOR_RANGE1 DX12RenderPassResourceService::GetDescriptorRange(RenderPassComponent* RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
{
	auto& l_descriptorAccessor = m_ctx->GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
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

	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer && !DX12Helper_BindlessMesh::TryFillDescriptorRange(resourceBinderLayoutDesc.m_GPUBufferUsage, *m_ctx, l_range)) l_range.NumDescriptors = 1;
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::Sample)
			l_range.NumDescriptors = l_descriptorAccessor.GetDesc().m_MaxDescriptors;
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
