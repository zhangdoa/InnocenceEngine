#pragma once

#include "NRDConstants.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter.h"

#include "../../Engine/Services/DX12/DX12Headers.h"

#include <NRD.h>

#include <vector>
#include <cstdint>

namespace Inno
{
	// Per-NRD-pipeline D3D12 state. NRD generates ~20 of these for
	// ReBLUR_DIFFUSE_SPECULAR; each can have its own descriptor-range cardinality
	// and compute shader bytecode. The adapter walks instanceDesc.pipelines[]
	// at Initialize and creates one entry per pipeline.
	struct NRDPipelineEntry
	{
		ID3D12RootSignature* m_RootSignature = nullptr;
		ID3D12PipelineState* m_PipelineState = nullptr;
		uint32_t             m_TextureCount  = 0;   // SRV count for this pipeline
		uint32_t             m_StorageCount  = 0;   // UAV count for this pipeline
	};

	// One entry per (permanent + transient) pool slot. m_Resource owns the
	// D3D12 allocation; the SRV/UAV CPU descriptor handles are pre-built so
	// every per-frame dispatch can copy them into the shader-visible heap
	// without recreating views.
	struct NRDPoolTexture
	{
		ID3D12Resource*             m_Resource = nullptr;
		D3D12_RESOURCE_STATES       m_State    = D3D12_RESOURCE_STATE_COMMON;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRV      = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_UAV      = {};
		nrd::Format                 m_Format   = nrd::Format::MAX_NUM;
	};

	struct NRDIntegrationAdapterImpl;

	// Helpers split out of NRDIntegrationAdapter_Setup.cpp to keep each TU
	// under the 300-line file-size gate. All static-style functions on a
	// stateless namespace shell — no shared state of their own.
	struct NRDAdapterHelpers
	{
		static DXGI_FORMAT NRDFormatToDXGI(nrd::Format in_Format);
		static D3D12_CPU_DESCRIPTOR_HANDLE AllocCPUDescriptor(NRDIntegrationAdapterImpl* in_Impl);
		static bool CreatePoolTextures(NRDIntegrationAdapterImpl* in_Impl, const nrd::InstanceDesc& in_Desc);
		static bool CreatePipelines(NRDIntegrationAdapterImpl* in_Impl, const nrd::InstanceDesc& in_Desc);
		// Allocate one adapter-owned RGBA16F output texture + SRV/UAV CPU
		// descriptors. Used for OUT_DIFF_RADIANCE_HITDIST and
		// OUT_SPEC_RADIANCE_HITDIST (NRD's raw API treats these as USER-
		// supplied per NRDIntegration.hpp's ResourceSnapshot model).
		static bool AllocateOutputTexture(NRDIntegrationAdapterImpl* in_Impl,
		                                  uint16_t in_ResolutionX, uint16_t in_ResolutionY,
		                                  ID3D12Resource** out_Resource,
		                                  D3D12_CPU_DESCRIPTOR_HANDLE* out_SRV,
		                                  D3D12_CPU_DESCRIPTOR_HANDLE* out_UAV,
		                                  const char* in_DebugName);
		// Wire an engine TextureComponent shell to an adapter-owned
		// resource so the composition pass binds it through the engine's
		// normal BindGPUResource path.
		static void SetupBorrowedShell(NRDIntegrationAdapterImpl* in_Impl,
		                               TextureComponent*& out_Tex,
		                               ID3D12Resource* in_Resource,
		                               uint16_t in_ResolutionX, uint16_t in_ResolutionY,
		                               const char* in_Name);

		// === Per-frame dispatch helpers (NRDIntegrationAdapter_DispatchHelpers.cpp) ===
		// (Re-)create SRVs for the 5 per-frame engine-side inputs + the IN_MV-
		// as-UAV slot into the CPU staging slots reserved at Initialize.
		static void EnsureInputSRVs(NRDIntegrationAdapterImpl* in_Impl, const NRDInputs& in_Inputs);
		// Resolve a NRD ResourceDesc to a pre-built CPU descriptor handle.
		static D3D12_CPU_DESCRIPTOR_HANDLE GetSourceDescriptor(NRDIntegrationAdapterImpl* in_Impl, const nrd::ResourceDesc& in_ResourceDesc);
		// Resolve a NRD ResourceDesc to its (resource, state-tracker) pair
		// for emitting transition barriers. Returns nullptrs for resources
		// that don't need barrier tracking inside DispatchDenoise.
		static void GetResourceForBarrier(NRDIntegrationAdapterImpl* in_Impl,
		                                  const NRDInputs&            in_Inputs,
		                                  const nrd::ResourceDesc&    in_RD,
		                                  ID3D12Resource**            out_Resource,
		                                  D3D12_RESOURCE_STATES**     out_State);
	};

	// pImpl. Visible only to NRDIntegrationAdapter*.cpp; the public header
	// keeps this opaque so a TU that does not need raw-D3D12 types stays
	// independent of DX12 headers.
	struct NRDIntegrationAdapterImpl
	{
		// === NRD instance ===
		nrd::Instance* m_NRDInstance = nullptr;

		// === Engine-borrowed resources (do not own) ===
		ID3D12Device9* m_Device = nullptr;

		// === Per-pipeline state ===
		std::vector<NRDPipelineEntry> m_Pipelines;

		// === Permanent + transient texture pool ===
		std::vector<NRDPoolTexture> m_PoolTextures;
		uint32_t                    m_PermanentCount = 0;
		uint32_t                    m_TransientCount = 0;

		// === Constant buffer ring (per-dispatch byte-offset binding) ===
		ID3D12Resource* m_ConstantBuffer            = nullptr;
		uint8_t*        m_ConstantBufferMapped      = nullptr;  // persistently mapped upload heap
		uint64_t        m_ConstantBufferSize        = 0;
		uint32_t        m_ConstantBufferViewSize    = 0;        // aligned per-dispatch view size
		uint32_t        m_ConstantBufferOffset      = 0;        // current head
		uint32_t        m_ConstantBufferOffsetPrev  = 0;        // previous (for matchesPreviousDispatch)

		// === Descriptor heap (CPU-only, source for per-frame descriptor copies) ===
		// Holds SRV+UAV descriptors for all pool textures (pre-built once);
		// per-frame inputs (5 SRVs from PTNRDFormatConvertPass) are created
		// here as well, refreshed per Initialize cycle.
		ID3D12DescriptorHeap* m_CPUDescriptorHeap     = nullptr;
		uint32_t              m_CPUDescriptorIncrement = 0;
		uint32_t              m_CPUDescriptorCapacity  = 0;
		uint32_t              m_CPUDescriptorHead      = 0;     // next free CPU slot

		// === Shader-visible descriptor heap (per-dispatch suballocation) ===
		// All per-dispatch descriptor sets live here; head wraps each frame.
		// Sized for ~20 pipelines × ~20 descriptors × queuedFrameNum (3) ~= 1200.
		// We bump to 4096 to give margin against ReBLUR variance.
		ID3D12DescriptorHeap* m_GPUDescriptorHeap     = nullptr;
		uint32_t              m_GPUDescriptorIncrement = 0;
		uint32_t              m_GPUDescriptorCapacity  = 0;
		uint32_t              m_GPUDescriptorHead      = 0;     // wraps each Denoise call

		// === Static samplers for NRD (NEAREST_CLAMP, LINEAR_CLAMP) ===
		// Wired into every per-pipeline root signature directly via
		// D3D12_STATIC_SAMPLER_DESC at register(s0/s1, space1).

		// === Per-input descriptor cache ===
		// SRV descriptors for the engine-side inputs (ViewZ, NormalRoughness,
		// MotionVector, DiffRadianceHitDist, SpecRadianceHitDist) plus a UAV
		// descriptor for IN_MV (REBLUR's mv-reprojection PushOutputs IN_MV,
		// see Reblur_DiffuseSpecular.hpp:270). All are created on m_CPUDescriptorHeap
		// at first DispatchDenoise and reused across frames because the
		// input TextureComponent*s are stable for the adapter's lifetime.
		uint32_t m_InputSRVBaseSlot = 0;
		uint32_t m_InputMVUAVSlot   = 0;

		// Per-frame state tracker for IN_MV (engine-owned resource that NRD
		// writes during mv-reprojection). The format-convert pass sets it to
		// NON_PIXEL_SHADER_RESOURCE at end of its dispatch (engine state
		// tracker matches). DispatchDenoise's barrier loop transitions to
		// UAV before NRD writes; the dispatch tail restores the resource
		// back to NON_PIXEL_SHADER_RESOURCE so the engine state tracker
		// stays consistent for the rest of the frame and for next-frame's
		// format-convert pass write.
		D3D12_RESOURCE_STATES m_InputMVState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		// 5 input SRVs in order: ViewZ, NormalRoughness, MotionVector,
		// DiffRadianceHitDist, SpecRadianceHitDist.
		static constexpr uint32_t k_InputCount = 5;

		// === Adapter-owned OUT_DIFF / OUT_SPEC textures + engine shells ===
		// NRD's raw Instance API treats OUT_DIFF_RADIANCE_HITDIST and
		// OUT_SPEC_RADIANCE_HITDIST as USER-supplied resources (matches
		// NRDIntegration.hpp's ResourceSnapshot.slots[] for non-pool types,
		// see NRDIntegration.h:121). The adapter therefore allocates these
		// two textures itself (RGBA16F at screen resolution) — they are NOT
		// part of NRD's permanent / transient pool.
		//   m_OutDiff       : ID3D12Resource* (adapter-owned UAV-RW texture)
		//   m_OutDiffSRV    : pre-built CPU SRV descriptor on m_CPUDescriptorHeap
		//   m_OutDiffUAV    : pre-built CPU UAV descriptor on m_CPUDescriptorHeap
		//   m_OutDiffState  : tracker for barrier emission inside DispatchDenoise
		//   m_OutDiffShell  : engine TextureComponent* whose m_GPUResources[0]
		//                     points at m_OutDiff and whose m_ReadHandles[0] is
		//                     a shader-visible SRV — composition pass binds it
		//                     through the engine's normal BindGPUResource path.
		ID3D12Resource*             m_OutDiff      = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_OutDiffSRV   = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_OutDiffUAV   = {};
		D3D12_RESOURCE_STATES       m_OutDiffState = D3D12_RESOURCE_STATE_COMMON;
		ID3D12Resource*             m_OutSpec      = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_OutSpecSRV   = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_OutSpecUAV   = {};
		D3D12_RESOURCE_STATES       m_OutSpecState = D3D12_RESOURCE_STATE_COMMON;
		TextureComponent*           m_OutDiffShell = nullptr;
		TextureComponent*           m_OutSpecShell = nullptr;

		// === Resolution + frame state ===
		uint16_t m_ResourceWidth      = 0;
		uint16_t m_ResourceHeight     = 0;
		uint32_t m_FrameIndexInternal = 0;        // adapter-internal monotonic counter

		// === Initialization gate ===
		bool m_Initialized = false;
	};
}

#endif // INNO_BUILD_WITH_NRD
