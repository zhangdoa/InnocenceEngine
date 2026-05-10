#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Services/DX12/DX12GraphicsHardwareService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <NRDDescs.h>

namespace Inno
{
	// Build one root signature per NRD pipeline. The cardinality of the SRV
	// and UAV descriptor ranges varies per pipeline (this is the engine-
	// binding-gap #1 from the sub-pivot — the engine's RenderPassComponent
	// can't carry per-pipeline variable ranges, so we own the root sigs here).
	bool NRDAdapterHelpers::CreatePipelines(NRDIntegrationAdapterImpl* in_Impl,
	                                        const nrd::InstanceDesc&    in_Desc)
	{
		in_Impl->m_Pipelines.resize(in_Desc.pipelinesNum);

		// Static samplers — NRD requires NEAREST_CLAMP and LINEAR_CLAMP at
		// register(s0/s1, space1). Engine has no static-sampler hook on
		// RenderPassComponent (engine-binding-gap #3), so we author the
		// D3D12_STATIC_SAMPLER_DESCs directly here.
		D3D12_STATIC_SAMPLER_DESC l_Samplers[2] = {};
		// s0 (space1): NEAREST_CLAMP
		l_Samplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
		l_Samplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[0].MaxLOD           = D3D12_FLOAT32_MAX;
		l_Samplers[0].ShaderRegister   = 0;
		l_Samplers[0].RegisterSpace    = 1;
		l_Samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		// s1 (space1): LINEAR_CLAMP
		l_Samplers[1].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		l_Samplers[1].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[1].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[1].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		l_Samplers[1].MaxLOD           = D3D12_FLOAT32_MAX;
		l_Samplers[1].ShaderRegister   = 1;
		l_Samplers[1].RegisterSpace    = 1;
		l_Samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		for (uint32_t l_i = 0u; l_i < in_Desc.pipelinesNum; ++l_i)
		{
			const nrd::PipelineDesc& l_NRDPipe = in_Desc.pipelines[l_i];
			NRDPipelineEntry&        l_Entry   = in_Impl->m_Pipelines[l_i];

			// Tally per-pipeline texture/storage counts.
			uint32_t l_textureCount = 0u;
			uint32_t l_storageCount = 0u;
			for (uint32_t l_r = 0u; l_r < l_NRDPipe.resourceRangesNum; ++l_r)
			{
				const nrd::ResourceRangeDesc& l_Range = l_NRDPipe.resourceRanges[l_r];
				if (l_Range.descriptorType == nrd::DescriptorType::TEXTURE)
					l_textureCount += l_Range.descriptorsNum;
				else
					l_storageCount += l_Range.descriptorsNum;
			}
			l_Entry.m_TextureCount = l_textureCount;
			l_Entry.m_StorageCount = l_storageCount;

			// Root sig: param0 = root CBV at b0,space1 (engine-binding-gap #2 +
			// #4: register-space-1 override + per-dispatch byte-offset binding);
			// param1 = descriptor table over (t0..tN, space0) + (u0..uM, space0)
			// (engine-binding-gap #1: variable per-pipeline cardinality).
			D3D12_DESCRIPTOR_RANGE1 l_Ranges[2] = {};
			uint32_t                l_RangeNum  = 0u;
			if (l_textureCount > 0u)
			{
				l_Ranges[l_RangeNum].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				l_Ranges[l_RangeNum].NumDescriptors                    = l_textureCount;
				l_Ranges[l_RangeNum].BaseShaderRegister                = 0;
				l_Ranges[l_RangeNum].RegisterSpace                     = 0;
				l_Ranges[l_RangeNum].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				l_Ranges[l_RangeNum].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				++l_RangeNum;
			}
			if (l_storageCount > 0u)
			{
				l_Ranges[l_RangeNum].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				l_Ranges[l_RangeNum].NumDescriptors                    = l_storageCount;
				l_Ranges[l_RangeNum].BaseShaderRegister                = 0;
				l_Ranges[l_RangeNum].RegisterSpace                     = 0;
				l_Ranges[l_RangeNum].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				l_Ranges[l_RangeNum].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				++l_RangeNum;
			}

			D3D12_ROOT_PARAMETER1 l_Params[2] = {};
			// param0: root CBV at (b0, space1)
			l_Params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_CBV;
			l_Params[0].Descriptor.ShaderRegister = 0;
			l_Params[0].Descriptor.RegisterSpace  = 1;
			l_Params[0].Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
			l_Params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
			// param1: descriptor table
			l_Params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			l_Params[1].DescriptorTable.NumDescriptorRanges = l_RangeNum;
			l_Params[1].DescriptorTable.pDescriptorRanges   = l_Ranges;
			l_Params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC l_RSDesc = {};
			l_RSDesc.Version                  = D3D_ROOT_SIGNATURE_VERSION_1_1;
			l_RSDesc.Desc_1_1.NumParameters     = 2;
			l_RSDesc.Desc_1_1.pParameters       = l_Params;
			l_RSDesc.Desc_1_1.NumStaticSamplers = 2;
			l_RSDesc.Desc_1_1.pStaticSamplers   = l_Samplers;
			l_RSDesc.Desc_1_1.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;

			ComPtr<ID3DBlob> l_Signature;
			ComPtr<ID3DBlob> l_Error;
			HRESULT l_HR = D3D12SerializeVersionedRootSignature(&l_RSDesc, &l_Signature, &l_Error);
			if (FAILED(l_HR))
			{
				const char* l_msg = l_Error ? static_cast<const char*>(l_Error->GetBufferPointer()) : "(no error blob)";
				Log(Error, "NRDAdapter: D3D12SerializeVersionedRootSignature failed for pipeline ", l_i, " HRESULT=", static_cast<int32_t>(l_HR), " msg='", l_msg, "'");
				return false;
			}
			l_HR = in_Impl->m_Device->CreateRootSignature(0, l_Signature->GetBufferPointer(), l_Signature->GetBufferSize(), IID_PPV_ARGS(&l_Entry.m_RootSignature));
			if (FAILED(l_HR))
			{
				Log(Error, "NRDAdapter: CreateRootSignature failed for pipeline ", l_i, " HRESULT=", static_cast<int32_t>(l_HR));
				return false;
			}

			D3D12_COMPUTE_PIPELINE_STATE_DESC l_PSODesc = {};
			l_PSODesc.pRootSignature = l_Entry.m_RootSignature;
			l_PSODesc.CS.pShaderBytecode = l_NRDPipe.computeShaderDXIL.bytecode;
			l_PSODesc.CS.BytecodeLength  = static_cast<SIZE_T>(l_NRDPipe.computeShaderDXIL.size);
			if (l_PSODesc.CS.BytecodeLength == 0)
			{
				Log(Error, "NRDAdapter: pipeline ", l_i, " has empty DXIL bytecode (NRD built without DXIL?)");
				return false;
			}
			l_HR = in_Impl->m_Device->CreateComputePipelineState(&l_PSODesc, IID_PPV_ARGS(&l_Entry.m_PipelineState));
			if (FAILED(l_HR))
			{
				Log(Error, "NRDAdapter: CreateComputePipelineState failed for pipeline ", l_i, " HRESULT=", static_cast<int32_t>(l_HR));
				return false;
			}
		}

		return true;
	}

	bool NRDAdapterHelpers::AllocateOutputTexture(NRDIntegrationAdapterImpl* in_Impl,
	                                              uint16_t in_ResolutionX, uint16_t in_ResolutionY,
	                                              ID3D12Resource** out_Resource,
	                                              D3D12_CPU_DESCRIPTOR_HANDLE* out_SRV,
	                                              D3D12_CPU_DESCRIPTOR_HANDLE* out_UAV,
	                                              const char* in_DebugName)
	{
		D3D12_RESOURCE_DESC l_resDesc = {};
		l_resDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		l_resDesc.Width              = in_ResolutionX;
		l_resDesc.Height             = in_ResolutionY;
		l_resDesc.DepthOrArraySize   = 1;
		l_resDesc.MipLevels          = 1;
		l_resDesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
		l_resDesc.SampleDesc.Count   = 1;
		l_resDesc.SampleDesc.Quality = 0;
		l_resDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		l_resDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES l_heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT l_hr = in_Impl->m_Device->CreateCommittedResource(
			&l_heapProps, D3D12_HEAP_FLAG_NONE, &l_resDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(out_Resource));
		if (FAILED(l_hr))
		{
			Log(Error, "NRDAdapter: ", in_DebugName, " CreateCommittedResource failed HRESULT=", static_cast<int32_t>(l_hr));
			return false;
		}

		*out_SRV = AllocCPUDescriptor(in_Impl);
		D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
		l_SRVDesc.Format                    = l_resDesc.Format;
		l_SRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_SRVDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		l_SRVDesc.Texture2D.MipLevels       = 1;
		l_SRVDesc.Texture2D.MostDetailedMip = 0;
		in_Impl->m_Device->CreateShaderResourceView(*out_Resource, &l_SRVDesc, *out_SRV);

		*out_UAV = AllocCPUDescriptor(in_Impl);
		D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
		l_UAVDesc.Format               = l_resDesc.Format;
		l_UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		l_UAVDesc.Texture2D.MipSlice   = 0;
		l_UAVDesc.Texture2D.PlaneSlice = 0;
		in_Impl->m_Device->CreateUnorderedAccessView(*out_Resource, nullptr, &l_UAVDesc, *out_UAV);

		return true;
	}

	void NRDAdapterHelpers::SetupBorrowedShell(NRDIntegrationAdapterImpl* in_Impl,
	                                           TextureComponent*& out_Tex,
	                                           ID3D12Resource* in_Resource,
	                                           uint16_t in_ResolutionX, uint16_t in_ResolutionY,
	                                           const char* in_Name)
	{
		auto* l_texService = g_Engine->Get<TextureResourceService>();
		auto* l_hwService  = static_cast<DX12GraphicsHardwareService*>(g_Engine->Get<GraphicsHardwareService>());

		out_Tex = l_texService->Add(in_Name);
		out_Tex->m_GPUResourceType                 = GPUResourceType::Image;
		out_Tex->m_TextureDesc.Sampler             = TextureSampler::Sampler2D;
		out_Tex->m_TextureDesc.Usage               = TextureUsage::ComputeOnly;
		out_Tex->m_TextureDesc.PixelDataFormat     = TexturePixelDataFormat::RGBA;
		out_Tex->m_TextureDesc.PixelDataType       = TexturePixelDataType::Float16;
		out_Tex->m_TextureDesc.Width               = in_ResolutionX;
		out_Tex->m_TextureDesc.Height              = in_ResolutionY;
		out_Tex->m_TextureDesc.DepthOrArraySize    = 1;
		out_Tex->m_TextureDesc.MipLevels           = 1;
		out_Tex->m_TextureDesc.IsMultiBuffer       = false;
		out_Tex->m_CPUAccessibility                = Accessibility::Immutable;
		out_Tex->m_GPUAccessibility                = Accessibility::ReadWrite;
		out_Tex->m_GPUResources.resize(1);
		out_Tex->m_ReadHandles.resize(1);
		out_Tex->m_WriteHandles.resize(1);
		out_Tex->m_CurrentState.resize(1, static_cast<uint32_t>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
		out_Tex->m_ReadState                       = static_cast<uint32_t>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		out_Tex->m_WriteState                      = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		out_Tex->m_GPUResources[0] = in_Resource;

		// Allocate one shader-visible SRV slot from the engine's RT-SRV
		// heap and create an SRV pointing at the adapter-owned resource
		// there. The composition pass's BindGPUResource path reads
		// m_ReadHandles[0].m_GPUHandle and binds it as a descriptor table.
		auto& l_accessor = l_hwService->GetDescriptorHeapAccessor(GPUResourceType::Image,
		                                                          Accessibility::ReadOnly,
		                                                          Accessibility::ReadWrite,
		                                                          TextureUsage::ColorAttachment);
		out_Tex->m_ReadHandles[0] = l_accessor.GetNewHandle();
		D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
		l_SRVDesc.Format                    = DXGI_FORMAT_R16G16B16A16_FLOAT;
		l_SRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_SRVDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		l_SRVDesc.Texture2D.MipLevels       = 1;
		l_SRVDesc.Texture2D.MostDetailedMip = 0;
		in_Impl->m_Device->CreateShaderResourceView(in_Resource, &l_SRVDesc,
			D3D12_CPU_DESCRIPTOR_HANDLE{ out_Tex->m_ReadHandles[0].m_CPUHandle });

		out_Tex->m_ObjectStatus = ObjectStatus::Activated;
	}
}

#endif // INNO_BUILD_WITH_NRD
