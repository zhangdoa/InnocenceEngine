#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Services/DX12/DX12GraphicsHardwareService.h"
#include "../../Engine/Services/DX12/DX12Helper_Common.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <NRDDescs.h>

// Pool / pipeline / format helpers live in NRDIntegrationAdapter_SetupHelpers.cpp
// (declared on NRDAdapterHelpers in _Impl.h). Keeping the helpers in a sibling
// TU keeps Initialize + Terminate in this file under the file-size gate.

namespace Inno
{

	bool NRDIntegrationAdapter::Initialize(uint16_t in_ResolutionX, uint16_t in_ResolutionY)
	{
		if (!m_Impl || m_Impl->m_Initialized)
			return m_Impl && m_Impl->m_Initialized;

		auto* l_hwService = static_cast<DX12GraphicsHardwareService*>(g_Engine->Get<GraphicsHardwareService>());
		auto* l_ctx       = l_hwService->GetDX12Context();
		m_Impl->m_Device  = l_ctx->m_device.Get();
		m_Impl->m_ResourceWidth  = in_ResolutionX;
		m_Impl->m_ResourceHeight = in_ResolutionY;

		// CL-4 vendor-force-off — NRD is NV-developed; perf and correctness
		// on AMD/Intel are not guaranteed by NV docs. Read VendorId from the
		// engine's cached DXGI_ADAPTER_DESC (DX12Context.h:17, populated at
		// device-create) and refuse to initialize when the adapter is not
		// NVIDIA. Initialize returning false propagates: PTNRDDenoisePass
		// stays !Activated, PTNRDCompositionPass stays !Activated, and
		// ExampleRenderingClient_PrepareCommands.cpp:155-163 falls through
		// to the raw PT AccumBuffer for the tonemap source. Single binary
		// ships everywhere; AMD/Intel users get raw 1-spp PT.
		// Compile-time-disable via Inno::NRD::FORCE_OFF_ON_NON_NV_GPU=false
		// in NRDConstants.h to test ReBLUR on a non-NV GPU.
		if constexpr (Inno::NRD::FORCE_OFF_ON_NON_NV_GPU)
		{
			const uint32_t l_vendorID = l_ctx->m_adapterDesc.VendorId;
			if (l_vendorID != Inno::NRD::NVIDIA_VENDOR_ID)
			{
				Log(Warning, "NRDAdapter: DXGI VendorId=", l_vendorID,
				    " (NVIDIA=4318=0x10DE); FORCE_OFF_ON_NON_NV_GPU is true. ",
				    "Skipping NRD instance creation; engine falls back to raw 1-spp PT.");
				return false;
			}
		}

		// Create NRD instance with REBLUR_DIFFUSE_SPECULAR.
		nrd::DenoiserDesc l_DenoiserDesc = {};
		l_DenoiserDesc.identifier        = 0;
		l_DenoiserDesc.denoiser          = nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR;

		nrd::InstanceCreationDesc l_InstanceDesc = {};
		l_InstanceDesc.denoisers      = &l_DenoiserDesc;
		l_InstanceDesc.denoisersNum   = 1;

		nrd::Result l_Result = nrd::CreateInstance(l_InstanceDesc, m_Impl->m_NRDInstance);
		if (l_Result != nrd::Result::SUCCESS)
		{
			Log(Error, "NRDAdapter: nrd::CreateInstance failed result=", static_cast<int32_t>(l_Result));
			return false;
		}

		const nrd::InstanceDesc& l_Desc = *nrd::GetInstanceDesc(*m_Impl->m_NRDInstance);

		// Sanity-check NRD's reported register-space layout matches our root-sig assumptions.
		assert(l_Desc.constantBufferAndSamplersSpaceIndex == 1u);
		assert(l_Desc.resourcesSpaceIndex == 0u);
		assert(l_Desc.constantBufferRegisterIndex == 0u);

		// Size the CPU descriptor heap for: pool textures (SRV+UAV each) +
		// 5 per-frame engine inputs (SRV) + 1 IN_MV UAV (REBLUR mv-reproj
		// writes through this slot) + 2 adapter-owned outputs (SRV+UAV each)
		// + 4 slack.
		const uint32_t l_poolSize = l_Desc.permanentPoolSize + l_Desc.transientPoolSize;
		m_Impl->m_CPUDescriptorCapacity  = (l_poolSize * 2u) + NRDIntegrationAdapterImpl::k_InputCount + 1u + 4u + 4u;
		D3D12_DESCRIPTOR_HEAP_DESC l_CPUHeapDesc = {};
		l_CPUHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		l_CPUHeapDesc.NumDescriptors = m_Impl->m_CPUDescriptorCapacity;
		l_CPUHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // CPU-only / staging
		HRESULT l_HR = m_Impl->m_Device->CreateDescriptorHeap(&l_CPUHeapDesc, IID_PPV_ARGS(&m_Impl->m_CPUDescriptorHeap));
		if (FAILED(l_HR))
		{
			Log(Error, "NRDAdapter: CPU descriptor heap CreateDescriptorHeap failed HRESULT=", static_cast<int32_t>(l_HR));
			return false;
		}
		m_Impl->m_CPUDescriptorIncrement = m_Impl->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Shader-visible heap for per-dispatch descriptor sets. ~20 pipelines x
		// up to ~20 descriptors per dispatch x 3 frames-in-flight margin =
		// ~1200 descriptors. Bump to 4096 for safety.
		m_Impl->m_GPUDescriptorCapacity = 4096u;
		D3D12_DESCRIPTOR_HEAP_DESC l_GPUHeapDesc = {};
		l_GPUHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		l_GPUHeapDesc.NumDescriptors = m_Impl->m_GPUDescriptorCapacity;
		l_GPUHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		l_HR = m_Impl->m_Device->CreateDescriptorHeap(&l_GPUHeapDesc, IID_PPV_ARGS(&m_Impl->m_GPUDescriptorHeap));
		if (FAILED(l_HR))
		{
			Log(Error, "NRDAdapter: GPU descriptor heap CreateDescriptorHeap failed HRESULT=", static_cast<int32_t>(l_HR));
			return false;
		}
		m_Impl->m_GPUDescriptorIncrement = m_Impl->m_CPUDescriptorIncrement;

		if (!NRDAdapterHelpers::CreatePoolTextures(m_Impl, l_Desc))
			return false;
		if (!NRDAdapterHelpers::CreatePipelines(m_Impl, l_Desc))
			return false;

		// Constant buffer ring (upload heap, persistently mapped). Aligned to
		// D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256 bytes); ring
		// sized by max-dispatches-per-frame * queuedFrameNum from
		// descriptorPoolDesc.setsMaxNum (NRD reports the upper bound).
		const uint32_t l_alignedView = (l_Desc.constantBufferMaxDataSize + 255u) & ~255u;
		const uint64_t l_cbSize      = static_cast<uint64_t>(l_alignedView) *
		                               static_cast<uint64_t>(l_Desc.descriptorPoolDesc.setsMaxNum) *
		                               static_cast<uint64_t>(3u);  // queuedFrameNum
		m_Impl->m_ConstantBufferViewSize = l_alignedView;
		m_Impl->m_ConstantBufferSize     = l_cbSize;

		D3D12_RESOURCE_DESC l_CBResDesc = CD3DX12_RESOURCE_DESC::Buffer(l_cbSize);
		D3D12_HEAP_PROPERTIES l_CBHeap  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		l_HR = m_Impl->m_Device->CreateCommittedResource(
			&l_CBHeap, D3D12_HEAP_FLAG_NONE, &l_CBResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_Impl->m_ConstantBuffer));
		if (FAILED(l_HR))
		{
			Log(Error, "NRDAdapter: CB CreateCommittedResource failed HRESULT=", static_cast<int32_t>(l_HR));
			return false;
		}
		CD3DX12_RANGE l_NoRead(0, 0);
		l_HR = m_Impl->m_ConstantBuffer->Map(0, &l_NoRead, reinterpret_cast<void**>(&m_Impl->m_ConstantBufferMapped));
		if (FAILED(l_HR))
		{
			Log(Error, "NRDAdapter: CB Map failed HRESULT=", static_cast<int32_t>(l_HR));
			return false;
		}

		// Reserve CPU descriptor slots for the 5 per-frame inputs (SRV)
		// + one extra slot for IN_MV used as a STORAGE_TEXTURE — REBLUR
		// internally writes to IN_MV during mv-reprojection
		// (Reblur_DiffuseSpecular.hpp:270 PushOutput(IN_MV)). Bodies are
		// populated in DispatchDenoise on first call against the actual
		// TextureComponent*s (the inputs are stable for the adapter's
		// lifetime, so one creation each suffices).
		m_Impl->m_InputSRVBaseSlot = m_Impl->m_CPUDescriptorHead;
		for (uint32_t l_i = 0u; l_i < NRDIntegrationAdapterImpl::k_InputCount; ++l_i)
			(void)NRDAdapterHelpers::AllocCPUDescriptor(m_Impl);  // bump head; descriptor body filled lazily
		m_Impl->m_InputMVUAVSlot = m_Impl->m_CPUDescriptorHead;
		(void)NRDAdapterHelpers::AllocCPUDescriptor(m_Impl);

		// Allocate the adapter-owned OUT_DIFF and OUT_SPEC textures.
		// NRD's raw Instance API does NOT allocate these (they are listed
		// as USER-supplied in NRDIntegration.hpp's ResourceSnapshot model);
		// the adapter must provide them. RGBA16F at screen resolution
		// matches REBLUR_DIFFUSE_SPECULAR's documented OUT_*_RADIANCE_HITDIST
		// format band (NRDDescs.h:110, "R11G11B10f+"). The composition pass
		// reads the .rgb (linear radiance after REBLUR_BackEnd unpack).
		if (!NRDAdapterHelpers::AllocateOutputTexture(m_Impl, in_ResolutionX, in_ResolutionY,
				&m_Impl->m_OutDiff, &m_Impl->m_OutDiffSRV, &m_Impl->m_OutDiffUAV, "OutDiffRadianceHitDist"))
			return false;
		if (!NRDAdapterHelpers::AllocateOutputTexture(m_Impl, in_ResolutionX, in_ResolutionY,
				&m_Impl->m_OutSpec, &m_Impl->m_OutSpecSRV, &m_Impl->m_OutSpecUAV, "OutSpecRadianceHitDist"))
			return false;
		m_Impl->m_OutDiffState = D3D12_RESOURCE_STATE_COMMON;
		m_Impl->m_OutSpecState = D3D12_RESOURCE_STATE_COMMON;

		// Wire engine TextureComponent shells so the composition pass
		// binds the adapter-owned outputs through the normal BindGPUResource
		// path (m_GPUResources[0] borrows the ID3D12Resource*; m_ReadHandles[0]
		// is an SRV on the engine's shader-visible RT-SRV heap).
		NRDAdapterHelpers::SetupBorrowedShell(m_Impl, m_Impl->m_OutDiffShell, m_Impl->m_OutDiff,
			in_ResolutionX, in_ResolutionY, "NRD_OutDiffRadianceHitDist");
		NRDAdapterHelpers::SetupBorrowedShell(m_Impl, m_Impl->m_OutSpecShell, m_Impl->m_OutSpec,
			in_ResolutionX, in_ResolutionY, "NRD_OutSpecRadianceHitDist");

		m_Impl->m_Initialized = true;
		Log(Success, "NRDAdapter: Initialized at ", in_ResolutionX, "x", in_ResolutionY,
		    " with ", l_Desc.pipelinesNum, " pipelines, ",
		    l_Desc.permanentPoolSize, "+", l_Desc.transientPoolSize, " pool textures.");
		return true;
	}

	void NRDIntegrationAdapter::Terminate()
	{
		if (!m_Impl || !m_Impl->m_Initialized)
			return;

		// Drop the engine TextureComponent shells before releasing the
		// adapter-owned ID3D12Resource*s they reference.
		// TextureResourceService::Delete on a shell is safe IFF we first
		// clear m_GPUResources[0] — otherwise the engine's destruction path
		// would try to release a resource we own.
		auto* l_texService = g_Engine->Get<TextureResourceService>();
		auto l_dropShell = [&](TextureComponent*& tex)
		{
			if (tex)
			{
				if (!tex->m_GPUResources.empty())
					tex->m_GPUResources[0] = nullptr;
				l_texService->Delete(tex);
				tex = nullptr;
			}
		};
		l_dropShell(m_Impl->m_OutDiffShell);
		l_dropShell(m_Impl->m_OutSpecShell);

		if (m_Impl->m_OutDiff) { m_Impl->m_OutDiff->Release(); m_Impl->m_OutDiff = nullptr; }
		if (m_Impl->m_OutSpec) { m_Impl->m_OutSpec->Release(); m_Impl->m_OutSpec = nullptr; }

		if (m_Impl->m_ConstantBufferMapped)
		{
			m_Impl->m_ConstantBuffer->Unmap(0, nullptr);
			m_Impl->m_ConstantBufferMapped = nullptr;
		}
		if (m_Impl->m_ConstantBuffer) { m_Impl->m_ConstantBuffer->Release(); m_Impl->m_ConstantBuffer = nullptr; }

		for (auto& l_entry : m_Impl->m_Pipelines)
		{
			if (l_entry.m_PipelineState) { l_entry.m_PipelineState->Release(); l_entry.m_PipelineState = nullptr; }
			if (l_entry.m_RootSignature) { l_entry.m_RootSignature->Release(); l_entry.m_RootSignature = nullptr; }
		}
		m_Impl->m_Pipelines.clear();

		for (auto& l_slot : m_Impl->m_PoolTextures)
		{
			if (l_slot.m_Resource) { l_slot.m_Resource->Release(); l_slot.m_Resource = nullptr; }
		}
		m_Impl->m_PoolTextures.clear();

		if (m_Impl->m_GPUDescriptorHeap) { m_Impl->m_GPUDescriptorHeap->Release(); m_Impl->m_GPUDescriptorHeap = nullptr; }
		if (m_Impl->m_CPUDescriptorHeap) { m_Impl->m_CPUDescriptorHeap->Release(); m_Impl->m_CPUDescriptorHeap = nullptr; }

		if (m_Impl->m_NRDInstance)
		{
			nrd::DestroyInstance(*m_Impl->m_NRDInstance);
			m_Impl->m_NRDInstance = nullptr;
		}

		m_Impl->m_Initialized = false;
	}
}
#endif // INNO_BUILD_WITH_NRD
