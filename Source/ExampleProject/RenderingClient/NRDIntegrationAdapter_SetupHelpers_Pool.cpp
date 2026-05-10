#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <NRDDescs.h>

namespace Inno
{
	// NRD Format -> DXGI_FORMAT translation. Matches the NRI mapping in
	// NRDIntegration.hpp's g_NrdFormatToNri table; we collapse one layer
	// because raw D3D12 takes DXGI_FORMAT directly.
	DXGI_FORMAT NRDAdapterHelpers::NRDFormatToDXGI(nrd::Format in_Format)
	{
		switch (in_Format)
		{
			case nrd::Format::R8_UNORM:           return DXGI_FORMAT_R8_UNORM;
			case nrd::Format::R8_SNORM:           return DXGI_FORMAT_R8_SNORM;
			case nrd::Format::R8_UINT:            return DXGI_FORMAT_R8_UINT;
			case nrd::Format::R8_SINT:            return DXGI_FORMAT_R8_SINT;
			case nrd::Format::RG8_UNORM:          return DXGI_FORMAT_R8G8_UNORM;
			case nrd::Format::RG8_SNORM:          return DXGI_FORMAT_R8G8_SNORM;
			case nrd::Format::RG8_UINT:           return DXGI_FORMAT_R8G8_UINT;
			case nrd::Format::RG8_SINT:           return DXGI_FORMAT_R8G8_SINT;
			case nrd::Format::RGBA8_UNORM:        return DXGI_FORMAT_R8G8B8A8_UNORM;
			case nrd::Format::RGBA8_SNORM:        return DXGI_FORMAT_R8G8B8A8_SNORM;
			case nrd::Format::RGBA8_UINT:         return DXGI_FORMAT_R8G8B8A8_UINT;
			case nrd::Format::RGBA8_SINT:         return DXGI_FORMAT_R8G8B8A8_SINT;
			case nrd::Format::RGBA8_SRGB:         return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			case nrd::Format::R16_UNORM:          return DXGI_FORMAT_R16_UNORM;
			case nrd::Format::R16_SNORM:          return DXGI_FORMAT_R16_SNORM;
			case nrd::Format::R16_UINT:           return DXGI_FORMAT_R16_UINT;
			case nrd::Format::R16_SINT:           return DXGI_FORMAT_R16_SINT;
			case nrd::Format::R16_SFLOAT:         return DXGI_FORMAT_R16_FLOAT;
			case nrd::Format::RG16_UNORM:         return DXGI_FORMAT_R16G16_UNORM;
			case nrd::Format::RG16_SNORM:         return DXGI_FORMAT_R16G16_SNORM;
			case nrd::Format::RG16_UINT:          return DXGI_FORMAT_R16G16_UINT;
			case nrd::Format::RG16_SINT:          return DXGI_FORMAT_R16G16_SINT;
			case nrd::Format::RG16_SFLOAT:        return DXGI_FORMAT_R16G16_FLOAT;
			case nrd::Format::RGBA16_UNORM:       return DXGI_FORMAT_R16G16B16A16_UNORM;
			case nrd::Format::RGBA16_SNORM:       return DXGI_FORMAT_R16G16B16A16_SNORM;
			case nrd::Format::RGBA16_UINT:        return DXGI_FORMAT_R16G16B16A16_UINT;
			case nrd::Format::RGBA16_SINT:        return DXGI_FORMAT_R16G16B16A16_SINT;
			case nrd::Format::RGBA16_SFLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case nrd::Format::R32_UINT:           return DXGI_FORMAT_R32_UINT;
			case nrd::Format::R32_SINT:           return DXGI_FORMAT_R32_SINT;
			case nrd::Format::R32_SFLOAT:         return DXGI_FORMAT_R32_FLOAT;
			case nrd::Format::RG32_UINT:          return DXGI_FORMAT_R32G32_UINT;
			case nrd::Format::RG32_SINT:          return DXGI_FORMAT_R32G32_SINT;
			case nrd::Format::RG32_SFLOAT:        return DXGI_FORMAT_R32G32_FLOAT;
			case nrd::Format::RGB32_UINT:         return DXGI_FORMAT_R32G32B32_UINT;
			case nrd::Format::RGB32_SINT:         return DXGI_FORMAT_R32G32B32_SINT;
			case nrd::Format::RGB32_SFLOAT:       return DXGI_FORMAT_R32G32B32_FLOAT;
			case nrd::Format::RGBA32_UINT:        return DXGI_FORMAT_R32G32B32A32_UINT;
			case nrd::Format::RGBA32_SINT:        return DXGI_FORMAT_R32G32B32A32_SINT;
			case nrd::Format::RGBA32_SFLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case nrd::Format::R10_G10_B10_A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
			case nrd::Format::R10_G10_B10_A2_UINT:  return DXGI_FORMAT_R10G10B10A2_UINT;
			case nrd::Format::R11_G11_B10_UFLOAT:   return DXGI_FORMAT_R11G11B10_FLOAT;
			case nrd::Format::R9_G9_B9_E5_UFLOAT:   return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
			default:                              return DXGI_FORMAT_UNKNOWN;
		}
	}

	// Allocate a CPU-only descriptor heap slot. The CPU heap holds SRV/UAV
	// descriptors for every pool texture (built once at Initialize); per-frame
	// dispatches CopyDescriptorsSimple from this CPU heap into the
	// shader-visible heap. Mirrors the staging-heap pattern that
	// imgui_impl_dx12 + many AMD SDK samples use.
	D3D12_CPU_DESCRIPTOR_HANDLE NRDAdapterHelpers::AllocCPUDescriptor(NRDIntegrationAdapterImpl* in_Impl)
	{
		assert(in_Impl->m_CPUDescriptorHead < in_Impl->m_CPUDescriptorCapacity);
		D3D12_CPU_DESCRIPTOR_HANDLE l_Handle =
			in_Impl->m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		l_Handle.ptr += static_cast<SIZE_T>(in_Impl->m_CPUDescriptorHead) *
		                static_cast<SIZE_T>(in_Impl->m_CPUDescriptorIncrement);
		++in_Impl->m_CPUDescriptorHead;
		return l_Handle;
	}

	bool NRDAdapterHelpers::CreatePoolTextures(NRDIntegrationAdapterImpl* in_Impl,
	                                           const nrd::InstanceDesc&    in_Desc)
	{
		const uint32_t l_total = in_Desc.permanentPoolSize + in_Desc.transientPoolSize;
		in_Impl->m_PoolTextures.resize(l_total);
		in_Impl->m_PermanentCount = in_Desc.permanentPoolSize;
		in_Impl->m_TransientCount = in_Desc.transientPoolSize;

		for (uint32_t l_i = 0u; l_i < l_total; ++l_i)
		{
			const nrd::TextureDesc& l_NRDTexDesc = (l_i < in_Desc.permanentPoolSize)
				? in_Desc.permanentPool[l_i]
				: in_Desc.transientPool[l_i - in_Desc.permanentPoolSize];

			NRDPoolTexture& l_Slot = in_Impl->m_PoolTextures[l_i];
			l_Slot.m_Format = l_NRDTexDesc.format;

			const uint16_t l_W = static_cast<uint16_t>((in_Impl->m_ResourceWidth  + l_NRDTexDesc.downsampleFactor - 1u) / l_NRDTexDesc.downsampleFactor);
			const uint16_t l_H = static_cast<uint16_t>((in_Impl->m_ResourceHeight + l_NRDTexDesc.downsampleFactor - 1u) / l_NRDTexDesc.downsampleFactor);

			D3D12_RESOURCE_DESC l_ResourceDesc = {};
			l_ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			l_ResourceDesc.Alignment          = 0;
			l_ResourceDesc.Width              = l_W;
			l_ResourceDesc.Height             = l_H;
			l_ResourceDesc.DepthOrArraySize   = 1;
			l_ResourceDesc.MipLevels          = 1;
			l_ResourceDesc.Format             = NRDFormatToDXGI(l_NRDTexDesc.format);
			l_ResourceDesc.SampleDesc.Count   = 1;
			l_ResourceDesc.SampleDesc.Quality = 0;
			l_ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			l_ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			D3D12_HEAP_PROPERTIES l_HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			HRESULT l_HR = in_Impl->m_Device->CreateCommittedResource(
				&l_HeapProps, D3D12_HEAP_FLAG_NONE, &l_ResourceDesc,
				D3D12_RESOURCE_STATE_COMMON, nullptr,
				IID_PPV_ARGS(&l_Slot.m_Resource));
			if (FAILED(l_HR))
			{
				Log(Error, "NRDAdapter: pool texture[", l_i, "] CreateCommittedResource failed HRESULT=", static_cast<int32_t>(l_HR));
				return false;
			}
			l_Slot.m_State = D3D12_RESOURCE_STATE_COMMON;

			// Pre-build SRV + UAV descriptors on the CPU-only heap.
			l_Slot.m_SRV = AllocCPUDescriptor(in_Impl);
			D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
			l_SRVDesc.Format                    = l_ResourceDesc.Format;
			l_SRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
			l_SRVDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			l_SRVDesc.Texture2D.MipLevels       = 1;
			l_SRVDesc.Texture2D.MostDetailedMip = 0;
			in_Impl->m_Device->CreateShaderResourceView(l_Slot.m_Resource, &l_SRVDesc, l_Slot.m_SRV);

			l_Slot.m_UAV = AllocCPUDescriptor(in_Impl);
			D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
			l_UAVDesc.Format               = l_ResourceDesc.Format;
			l_UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
			l_UAVDesc.Texture2D.MipSlice   = 0;
			l_UAVDesc.Texture2D.PlaneSlice = 0;
			in_Impl->m_Device->CreateUnorderedAccessView(l_Slot.m_Resource, nullptr, &l_UAVDesc, l_Slot.m_UAV);
		}

		return true;
	}
}

#endif // INNO_BUILD_WITH_NRD
