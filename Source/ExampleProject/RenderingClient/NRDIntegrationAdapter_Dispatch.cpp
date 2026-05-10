#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Services/DX12/DX12GraphicsHardwareService.h"
#include "../../Engine/Services/DX12/DX12Helper_Common.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <NRDDescs.h>
#include <NRDSettings.h>

#include <cstring>

namespace Inno
{
	bool NRDIntegrationAdapter::DispatchDenoise(CommandListComponent* in_CommandList, const NRDInputs& in_Inputs)
	{
		if (!m_Impl || !m_Impl->m_Initialized)
			return false;
		if (!in_CommandList)
		{
			Log(Error, "NRDAdapter::DispatchDenoise: null command list");
			return false;
		}
		if (!in_Inputs.m_ViewZ || !in_Inputs.m_NormalRoughness || !in_Inputs.m_MotionVector ||
		    !in_Inputs.m_DiffRadianceHitDist || !in_Inputs.m_SpecRadianceHitDist)
		{
			Log(Error, "NRDAdapter::DispatchDenoise: one or more input TextureComponent*s is null");
			return false;
		}

		ID3D12GraphicsCommandList7* l_cmd = DX12Helper::AsDX12CommandList(in_CommandList);
		if (!l_cmd)
			return false;

		NRDAdapterHelpers::EnsureInputSRVs(m_Impl, in_Inputs);

		// Bind shader-visible heap (must match the heap we suballocate from).
		ID3D12DescriptorHeap* l_heaps[1] = { m_Impl->m_GPUDescriptorHeap };
		l_cmd->SetDescriptorHeaps(1, l_heaps);

		// Sync IN_MV state from the engine's per-resource tracker. The
		// format-convert pass leaves IN_MV in a multi-bit READ state
		// (NON_PIXEL_SHADER_RESOURCE | COPY_SOURCE) that the adapter must
		// match in the next transition's `before` field. The engine
		// TextureComponent's m_CurrentState[0] is the source of truth.
		m_Impl->m_InputMVState = static_cast<D3D12_RESOURCE_STATES>(in_Inputs.m_MotionVector->GetCurrentState(0u));

		// === SetCommonSettings ===
		// Engine Math::Mat4 is row-major / row-vector (skill shader-standards).
		// NRDSettings.h:86-97 explicitly requires column-major / column-vector
		// for all four CommonSettings matrix slots. Without this transpose,
		// NRD's spatial-filter kernels (e.g. REBLUR_Blur.cs.hlsl:66-68) derive
		// view-space normals against a transposed frame and the anisotropic
		// footprint orients on the wrong axis (visible as vertical streaks
		// perpendicular to texture grain on GITestBox walls).
		nrd::CommonSettings l_settings = {};
		Math::Mat4 l_viewToClip      = in_Inputs.m_ViewToClip.transpose();
		Math::Mat4 l_viewToClipPrev  = in_Inputs.m_ViewToClipPrev.transpose();
		Math::Mat4 l_worldToView     = in_Inputs.m_WorldToView.transpose();
		Math::Mat4 l_worldToViewPrev = in_Inputs.m_WorldToViewPrev.transpose();
		std::memcpy(l_settings.viewToClipMatrix,         &l_viewToClip,      sizeof(float) * 16);
		std::memcpy(l_settings.viewToClipMatrixPrev,     &l_viewToClipPrev,  sizeof(float) * 16);
		std::memcpy(l_settings.worldToViewMatrix,        &l_worldToView,     sizeof(float) * 16);
		std::memcpy(l_settings.worldToViewMatrixPrev,    &l_worldToViewPrev, sizeof(float) * 16);
		l_settings.motionVectorScale[0] = 1.0f;
		l_settings.motionVectorScale[1] = 1.0f;
		l_settings.motionVectorScale[2] = 0.0f;
		l_settings.resourceSize[0]      = m_Impl->m_ResourceWidth;
		l_settings.resourceSize[1]      = m_Impl->m_ResourceHeight;
		l_settings.resourceSizePrev[0]  = m_Impl->m_ResourceWidth;
		l_settings.resourceSizePrev[1]  = m_Impl->m_ResourceHeight;
		l_settings.rectSize[0]          = m_Impl->m_ResourceWidth;
		l_settings.rectSize[1]          = m_Impl->m_ResourceHeight;
		l_settings.rectSizePrev[0]      = m_Impl->m_ResourceWidth;
		l_settings.rectSizePrev[1]      = m_Impl->m_ResourceHeight;
		l_settings.frameIndex           = in_Inputs.m_FrameIndex;
		l_settings.accumulationMode     = in_Inputs.m_ResetAccumulation
		                                  ? nrd::AccumulationMode::RESTART
		                                  : nrd::AccumulationMode::CONTINUE;
		l_settings.isMotionVectorInWorldSpace = false;

		nrd::Result l_result = nrd::SetCommonSettings(*m_Impl->m_NRDInstance, l_settings);
		if (l_result != nrd::Result::SUCCESS)
		{
			Log(Error, "NRDAdapter: SetCommonSettings failed result=", static_cast<int32_t>(l_result));
			return false;
		}

		// === SetDenoiserSettings — defaults are sufficient for first launch ===
		nrd::ReblurSettings l_reblurSettings = {};  // NV defaults
		l_result = nrd::SetDenoiserSettings(*m_Impl->m_NRDInstance, 0, &l_reblurSettings);
		if (l_result != nrd::Result::SUCCESS)
		{
			Log(Error, "NRDAdapter: SetDenoiserSettings failed result=", static_cast<int32_t>(l_result));
			return false;
		}

		// === GetComputeDispatches ===
		const nrd::Identifier l_id = 0u;
		const nrd::DispatchDesc* l_dispatches    = nullptr;
		uint32_t                 l_dispatchesNum = 0u;
		l_result = nrd::GetComputeDispatches(*m_Impl->m_NRDInstance, &l_id, 1u, l_dispatches, l_dispatchesNum);
		if (l_result != nrd::Result::SUCCESS || l_dispatchesNum == 0u)
		{
			Log(Error, "NRDAdapter: GetComputeDispatches returned ", l_dispatchesNum, " dispatches result=", static_cast<int32_t>(l_result));
			return false;
		}

		// Reset GPU descriptor head every frame (single in-flight; this CL
		// runs after the format-convert pass on the same compute queue, then
		// the composition pass waits on it. The shader-visible heap is
		// effectively single-buffered for our purposes because the engine's
		// frame fence stalls before frame N+1 reuses the same heap range.
		// 4096-descriptor capacity is the safety margin against this
		// assumption being wrong; if it overflows the assertion below fires.)
		m_Impl->m_GPUDescriptorHead = 0u;
		m_Impl->m_ConstantBufferOffset     = 0u;
		m_Impl->m_ConstantBufferOffsetPrev = 0u;

		const nrd::InstanceDesc& l_instanceDesc = *nrd::GetInstanceDesc(*m_Impl->m_NRDInstance);

		// Walk dispatches and record into the command list.
		for (uint32_t l_d = 0u; l_d < l_dispatchesNum; ++l_d)
		{
			const nrd::DispatchDesc& l_dd        = l_dispatches[l_d];
			const nrd::PipelineDesc& l_pipeDesc  = l_instanceDesc.pipelines[l_dd.pipelineIndex];
			NRDPipelineEntry&        l_pipeEntry = m_Impl->m_Pipelines[l_dd.pipelineIndex];

			// === Insert barriers for resources whose state changes ===
			D3D12_RESOURCE_BARRIER l_barriers[64];
			uint32_t               l_barrierCount = 0u;
			for (uint32_t l_r = 0u; l_r < l_dd.resourcesNum; ++l_r)
			{
				const nrd::ResourceDesc& l_rd       = l_dd.resources[l_r];
				const D3D12_RESOURCE_STATES l_after = (l_rd.descriptorType == nrd::DescriptorType::STORAGE_TEXTURE)
				                                      ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
				                                      : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

				ID3D12Resource*           l_res    = nullptr;
				D3D12_RESOURCE_STATES*    l_state  = nullptr;
				NRDAdapterHelpers::GetResourceForBarrier(m_Impl, in_Inputs, l_rd, &l_res, &l_state);
				if (!l_res || !l_state)
					continue;
				if (*l_state != l_after)
				{
					assert(l_barrierCount < 64u);
					l_barriers[l_barrierCount++] = CD3DX12_RESOURCE_BARRIER::Transition(l_res, *l_state, l_after);
					*l_state = l_after;
				}
				else if (l_after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					// UAV-after-UAV barrier
					assert(l_barrierCount < 64u);
					l_barriers[l_barrierCount++] = CD3DX12_RESOURCE_BARRIER::UAV(l_res);
				}
			}
			if (l_barrierCount > 0u)
				l_cmd->ResourceBarrier(l_barrierCount, l_barriers);

			// === Suballocate descriptor table on the GPU heap ===
			const uint32_t l_descCount = l_dd.resourcesNum;
			assert(m_Impl->m_GPUDescriptorHead + l_descCount <= m_Impl->m_GPUDescriptorCapacity);
			D3D12_CPU_DESCRIPTOR_HANDLE l_dstStartCPU = m_Impl->m_GPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			l_dstStartCPU.ptr += static_cast<SIZE_T>(m_Impl->m_GPUDescriptorHead) *
			                     static_cast<SIZE_T>(m_Impl->m_GPUDescriptorIncrement);
			D3D12_GPU_DESCRIPTOR_HANDLE l_dstStartGPU = m_Impl->m_GPUDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			l_dstStartGPU.ptr += static_cast<UINT64>(m_Impl->m_GPUDescriptorHead) *
			                     static_cast<UINT64>(m_Impl->m_GPUDescriptorIncrement);

			for (uint32_t l_r = 0u; l_r < l_descCount; ++l_r)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE l_src = NRDAdapterHelpers::GetSourceDescriptor(m_Impl, l_dd.resources[l_r]);
				D3D12_CPU_DESCRIPTOR_HANDLE l_dst = l_dstStartCPU;
				l_dst.ptr += static_cast<SIZE_T>(l_r) * static_cast<SIZE_T>(m_Impl->m_GPUDescriptorIncrement);
				m_Impl->m_Device->CopyDescriptorsSimple(1, l_dst, l_src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
			m_Impl->m_GPUDescriptorHead += l_descCount;

			// === Upload constants if needed ===
			uint32_t l_cbDispatchOffset = m_Impl->m_ConstantBufferOffsetPrev;
			if (l_dd.constantBufferDataSize && !l_dd.constantBufferDataMatchesPreviousDispatch)
			{
				if (m_Impl->m_ConstantBufferOffset + m_Impl->m_ConstantBufferViewSize > m_Impl->m_ConstantBufferSize)
					m_Impl->m_ConstantBufferOffset = 0u;
				l_cbDispatchOffset = m_Impl->m_ConstantBufferOffset;
				std::memcpy(m_Impl->m_ConstantBufferMapped + l_cbDispatchOffset,
				            l_dd.constantBufferData,
				            l_dd.constantBufferDataSize);
				m_Impl->m_ConstantBufferOffset       += m_Impl->m_ConstantBufferViewSize;
				m_Impl->m_ConstantBufferOffsetPrev    = l_cbDispatchOffset;
			}

			// === Set root sig + PSO + bindings + dispatch ===
			l_cmd->SetComputeRootSignature(l_pipeEntry.m_RootSignature);
			l_cmd->SetPipelineState(l_pipeEntry.m_PipelineState);

			D3D12_GPU_VIRTUAL_ADDRESS l_cbGPUVA = m_Impl->m_ConstantBuffer->GetGPUVirtualAddress() + l_cbDispatchOffset;
			l_cmd->SetComputeRootConstantBufferView(0, l_cbGPUVA);
			l_cmd->SetComputeRootDescriptorTable(1, l_dstStartGPU);

			l_cmd->Dispatch(l_dd.gridWidth, l_dd.gridHeight, 1u);
		}

		// Transition the adapter-owned OUT_DIFF / OUT_SPEC textures back to
		// NON_PIXEL_SHADER_RESOURCE so the composition pass binds them as
		// ReadOnly SRVs on its own compute dispatch. The dispatch loop's
		// per-resource barrier emission left them in UAV state (last write).
		auto l_transitionOutput = [&](ID3D12Resource* in_Res, D3D12_RESOURCE_STATES& in_State, TextureComponent* in_Shell)
		{
			if (!in_Res) return;
			if (in_State != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			{
				D3D12_RESOURCE_BARRIER l_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					in_Res, in_State, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				l_cmd->ResourceBarrier(1, &l_barrier);
				in_State = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
			// Sync the engine-side state tracker on the shell so the
			// composition pass's TryToTransitState ReadOnly->ReadOnly is a no-op.
			if (in_Shell)
				in_Shell->SetCurrentState(0u, static_cast<uint32_t>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
		};
		l_transitionOutput(m_Impl->m_OutDiff, m_Impl->m_OutDiffState, m_Impl->m_OutDiffShell);
		l_transitionOutput(m_Impl->m_OutSpec, m_Impl->m_OutSpecState, m_Impl->m_OutSpecShell);

		// Restore IN_MV (engine-owned, NRD-mutated) to the engine-tracked
		// state recorded at DispatchDenoise entry. The engine's per-resource
		// tracker was set by the format-convert pass's exit transition (a
		// multi-bit READ mask: NON_PIXEL_SHADER_RESOURCE | COPY_SOURCE on
		// DX12). Restoring to that mask keeps the tracker valid for the
		// rest of the frame and the next frame's format-convert pass write.
		const D3D12_RESOURCE_STATES l_mvEntryState =
			static_cast<D3D12_RESOURCE_STATES>(in_Inputs.m_MotionVector->GetCurrentState(0u));
		if (in_Inputs.m_MotionVector->m_GPUResources[0]
		    && m_Impl->m_InputMVState != l_mvEntryState)
		{
			ID3D12Resource* l_mv = static_cast<ID3D12Resource*>(in_Inputs.m_MotionVector->m_GPUResources[0]);
			D3D12_RESOURCE_BARRIER l_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				l_mv, m_Impl->m_InputMVState, l_mvEntryState);
			l_cmd->ResourceBarrier(1, &l_barrier);
			m_Impl->m_InputMVState = l_mvEntryState;
		}

		++m_Impl->m_FrameIndexInternal;
		return true;
	}
}
#endif // INNO_BUILD_WITH_NRD
