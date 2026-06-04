#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <NRDDescs.h>

namespace Inno
{
	// (Re-)create SRVs for the 5 per-frame engine-side inputs + the
	// IN_MV-as-UAV slot into the CPU staging slots reserved at Initialize.
	// Slot layout: m_InputSRVBaseSlot..+4 are SRVs (ViewZ, NormalRoughness,
	// MotionVector, DiffRadianceHitDist, SpecRadianceHitDist); m_InputMVUAVSlot
	// is a UAV view of IN_MV (REBLUR's mv-reprojection writes here, see
	// Reblur_DiffuseSpecular.hpp:270 PushOutput(IN_MV)).
	void NRDAdapterHelpers::EnsureInputSRVs(NRDIntegrationAdapterImpl* in_Impl, const NRDInputs& in_Inputs)
	{
		TextureComponent* l_inputs[NRDIntegrationAdapterImpl::k_InputCount] = {
			in_Inputs.m_ViewZ,
			in_Inputs.m_NormalRoughness,
			in_Inputs.m_MotionVector,
			in_Inputs.m_DiffRadianceHitDist,
			in_Inputs.m_SpecRadianceHitDist,
		};
		for (uint32_t l_i = 0u; l_i < NRDIntegrationAdapterImpl::k_InputCount; ++l_i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE l_handle = in_Impl->m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			l_handle.ptr += static_cast<SIZE_T>(in_Impl->m_InputSRVBaseSlot + l_i) *
			                static_cast<SIZE_T>(in_Impl->m_CPUDescriptorIncrement);

			ID3D12Resource* l_resource = static_cast<ID3D12Resource*>(l_inputs[l_i]->m_GPUResources[0]);
			if (!l_resource)
				continue;

			D3D12_RESOURCE_DESC l_desc = l_resource->GetDesc();
			D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
			l_SRVDesc.Format                    = l_desc.Format;
			l_SRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
			l_SRVDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			l_SRVDesc.Texture2D.MipLevels       = 1;
			l_SRVDesc.Texture2D.MostDetailedMip = 0;
			in_Impl->m_Device->CreateShaderResourceView(l_resource, &l_SRVDesc, l_handle);
		}

		// IN_MV as UAV (REBLUR mv-reprojection writes here).
		ID3D12Resource* l_mvResource = static_cast<ID3D12Resource*>(in_Inputs.m_MotionVector->m_GPUResources[0]);
		if (l_mvResource)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE l_handle = in_Impl->m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			l_handle.ptr += static_cast<SIZE_T>(in_Impl->m_InputMVUAVSlot) *
			                static_cast<SIZE_T>(in_Impl->m_CPUDescriptorIncrement);

			D3D12_RESOURCE_DESC l_desc = l_mvResource->GetDesc();
			D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
			l_UAVDesc.Format               = l_desc.Format;
			l_UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
			l_UAVDesc.Texture2D.MipSlice   = 0;
			l_UAVDesc.Texture2D.PlaneSlice = 0;
			in_Impl->m_Device->CreateUnorderedAccessView(l_mvResource, nullptr, &l_UAVDesc, l_handle);
		}
	}

	// Map ResourceType -> CPU descriptor handle. For permanent/transient pool
	// entries the descriptor lives in the pre-built section of m_CPUDescriptorHeap;
	// for the 5 engine-side inputs we use the m_InputSRVBaseSlot reservation;
	// for the 2 adapter-owned outputs (OUT_DIFF / OUT_SPEC) we return the
	// pre-built SRV or UAV CPU handle stored on the adapter Impl.
	D3D12_CPU_DESCRIPTOR_HANDLE NRDAdapterHelpers::GetSourceDescriptor(NRDIntegrationAdapterImpl* in_Impl,
	                                                                   const nrd::ResourceDesc&    in_ResourceDesc)
	{
		const bool l_isStorage = (in_ResourceDesc.descriptorType == nrd::DescriptorType::STORAGE_TEXTURE);

		if (in_ResourceDesc.type == nrd::ResourceType::PERMANENT_POOL)
		{
			const NRDPoolTexture& l_slot = in_Impl->m_PoolTextures[in_ResourceDesc.indexInPool];
			return l_isStorage ? l_slot.m_UAV : l_slot.m_SRV;
		}
		if (in_ResourceDesc.type == nrd::ResourceType::TRANSIENT_POOL)
		{
			const uint32_t l_idx = in_Impl->m_PermanentCount + in_ResourceDesc.indexInPool;
			const NRDPoolTexture& l_slot = in_Impl->m_PoolTextures[l_idx];
			return l_isStorage ? l_slot.m_UAV : l_slot.m_SRV;
		}
		// Adapter-owned OUT_DIFF / OUT_SPEC textures (NRD raw API expects
		// these as USER-supplied; mirrors NRDIntegration.hpp's
		// ResourceSnapshot.slots[] for non-pool ResourceTypes).
		if (in_ResourceDesc.type == nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)
			return l_isStorage ? in_Impl->m_OutDiffUAV : in_Impl->m_OutDiffSRV;
		if (in_ResourceDesc.type == nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)
			return l_isStorage ? in_Impl->m_OutSpecUAV : in_Impl->m_OutSpecSRV;

		// IN_MV used as a UAV — REBLUR mv-reprojection
		// (Reblur_DiffuseSpecular.hpp:270 PushOutput(IN_MV)). Returns the
		// pre-built UAV CPU descriptor; the engine state-tracker is fixed up
		// after DispatchDenoise to reflect the temporary UAV state on IN_MV
		// so the format-convert pass's TryToTransitState on frame N+1 emits
		// the correct UAV→UAV barrier (no-op transition).
		if (in_ResourceDesc.type == nrd::ResourceType::IN_MV && l_isStorage)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE l_handle = in_Impl->m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			l_handle.ptr += static_cast<SIZE_T>(in_Impl->m_InputMVUAVSlot) *
			                static_cast<SIZE_T>(in_Impl->m_CPUDescriptorIncrement);
			return l_handle;
		}

		// Engine-side inputs (5 SRVs).
		uint32_t l_offset = UINT32_MAX;
		switch (in_ResourceDesc.type)
		{
			case nrd::ResourceType::IN_VIEWZ:                  l_offset = 0u; break;
			case nrd::ResourceType::IN_NORMAL_ROUGHNESS:       l_offset = 1u; break;
			case nrd::ResourceType::IN_MV:                     l_offset = 2u; break;
			case nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST:  l_offset = 3u; break;
			case nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST:  l_offset = 4u; break;
			default:
				Log(Warning, "NRDAdapter: unhandled ResourceType ", static_cast<int32_t>(in_ResourceDesc.type),
				    " — REBLUR_DIFFUSE_SPECULAR is expected to reference only IN_VIEWZ / IN_NORMAL_ROUGHNESS / IN_MV / IN_DIFF_RADIANCE_HITDIST / IN_SPEC_RADIANCE_HITDIST / OUT_DIFF_RADIANCE_HITDIST / OUT_SPEC_RADIANCE_HITDIST / PERMANENT_POOL / TRANSIENT_POOL.");
				break;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE l_handle = in_Impl->m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		if (l_offset != UINT32_MAX)
		{
			l_handle.ptr += static_cast<SIZE_T>(in_Impl->m_InputSRVBaseSlot + l_offset) *
			                static_cast<SIZE_T>(in_Impl->m_CPUDescriptorIncrement);
		}
		return l_handle;
	}

	// Return the underlying ID3D12Resource* + a pointer to its tracked state
	// for resources that need a barrier. Engine-side inputs return nullptr
	// for the state pointer because their state is already
	// SHADER_RESOURCE on entry (the format-convert pass leaves them
	// ReadOnly == NON_PIXEL_SHADER_RESOURCE) and the dispatch loop should
	// leave them unmodified. Pool textures and adapter-owned outputs track
	// their state internally.
	void NRDAdapterHelpers::GetResourceForBarrier(NRDIntegrationAdapterImpl* in_Impl,
	                                              const NRDInputs&            in_Inputs,
	                                              const nrd::ResourceDesc&    in_RD,
	                                              ID3D12Resource**            out_Resource,
	                                              D3D12_RESOURCE_STATES**     out_State)
	{
		*out_Resource = nullptr;
		*out_State    = nullptr;

		if (in_RD.type == nrd::ResourceType::PERMANENT_POOL)
		{
			NRDPoolTexture& l_slot = in_Impl->m_PoolTextures[in_RD.indexInPool];
			*out_Resource = l_slot.m_Resource;
			*out_State    = &l_slot.m_State;
			return;
		}
		if (in_RD.type == nrd::ResourceType::TRANSIENT_POOL)
		{
			NRDPoolTexture& l_slot = in_Impl->m_PoolTextures[in_Impl->m_PermanentCount + in_RD.indexInPool];
			*out_Resource = l_slot.m_Resource;
			*out_State    = &l_slot.m_State;
			return;
		}
		if (in_RD.type == nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)
		{
			*out_Resource = in_Impl->m_OutDiff;
			*out_State    = &in_Impl->m_OutDiffState;
			return;
		}
		if (in_RD.type == nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)
		{
			*out_Resource = in_Impl->m_OutSpec;
			*out_State    = &in_Impl->m_OutSpecState;
			return;
		}
		// IN_MV is an engine-owned resource that NRD writes during
		// mv-reprojection — track its state in the adapter for the duration
		// of DispatchDenoise. The dispatch tail restores it to
		// NON_PIXEL_SHADER_RESOURCE so the engine state tracker stays
		// consistent (the tracker assumes IN_MV is in that state at end of
		// the format-convert pass).
		if (in_RD.type == nrd::ResourceType::IN_MV)
		{
			*out_Resource = static_cast<ID3D12Resource*>(in_Inputs.m_MotionVector->m_GPUResources[0]);
			*out_State    = &in_Impl->m_InputMVState;
			return;
		}
		// Other engine-side inputs (IN_VIEWZ / IN_NORMAL_ROUGHNESS /
		// IN_DIFF_RADIANCE_HITDIST / IN_SPEC_RADIANCE_HITDIST) are
		// SRV-only for REBLUR_DIFFUSE_SPECULAR — already in
		// NON_PIXEL_SHADER_RESOURCE on entry from the format-convert pass,
		// no barrier needed inside DispatchDenoise.
	}
}

#endif // INNO_BUILD_WITH_NRD
